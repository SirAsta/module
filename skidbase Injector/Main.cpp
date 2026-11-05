#include <iostream>
#include <Windows.h>
#include <filesystem>
#include <string>
#include <vector>
#include <cstring>
#include <cstdio>
#include <winternl.h>

#include "Dependencies/Logging/Logging.h"
#include "Dependencies/Thread/Thread.h"
#include "Dependencies/Utilities/Utilities.h"
#include "Dependencies/Filesystem/Filesystem.h"
#include "Dependencies/Security/Security.h"
#include "Dependencies/Oxorany/Oxorany.h"

constexpr uintptr_t ShellcodePlaceholder = 0x1000000000ULL;

struct InjectContext {
	char DllPath[256];
	decltype(&LoadLibraryA) LoadLibraryA;
	decltype(&SetUnhandledExceptionFilter) SetUnhandledExceptionFilter;
	uintptr_t ExceptionFilter;
};

struct NotifyEntry {
	uint64_t Address;
	uint64_t Flink;
	uint64_t Blink;
	uint64_t Callback;
};

const std::vector<uint8_t> kNtCreateSectionBytes = {
	0x49, 0x89, 0xCA,
	0xB8, 0x4A, 0x00, 0x00, 0x00,
	0x0F, 0x05,
	0xC3
};

int ScoreListHead(HANDLE process, uintptr_t head) {
	// Read Flink+Blink in a single RPM so we never score a torn head
	// (two separate 8-byte reads can straddle a concurrent insert/remove).
	uint64_t headLinks[2] = {};
	if (!Utilities->ReadMem(process, head, headLinks, sizeof(headLinks)))
		return -1;

	const uint64_t flink = headLinks[0];
	const uint64_t blink = headLinks[1];

	if (!Utilities->LooksLikePointer(flink) || !Utilities->LooksLikePointer(blink))
		return -1;

	// Empty list: head points to itself.
	if (flink == head && blink == head)
		return 100;

	// Full forward walk with bidirectional validation. This catches torn
	// reads (concurrent LdrRegister/Unregister) and rejects them with -1
	// so the caller can retry, instead of mis-scoring a partial list.
	uint64_t prev = head;
	uint64_t cur = flink;
	for (int count = 0; count < 1024; ++count) {
		if (!Utilities->LooksLikePointer(cur))
			return -1;

		// Single 16-byte read per entry: Flink+Blink together.
		uint64_t entryLinks[2] = {};
		if (!Utilities->ReadMem(process, static_cast<uintptr_t>(cur), entryLinks, sizeof(entryLinks)))
			return -1;

		const uint64_t entryFlink = entryLinks[0];
		const uint64_t entryBlink = entryLinks[1];

		if (!Utilities->LooksLikePointer(entryFlink))
			return -1;
		// Blink must point back to where we came from.
		if (entryBlink != prev)
			return -1;

		prev = cur;
		cur = entryFlink;

		if (cur == head) {
			// Closed the loop: head's Blink must be the tail we ended on.
			if (blink != prev)
				return -1;
			if (count == 0)
				return 95; // single entry
			return 85; // multi entry, fully consistent
		}
	}

	return -1; // loop never closed: cycle / corrupt / way too long
}

void CALLBACK NullDllNotify(ULONG, PVOID, PVOID) {}

bool ResolveNotifyListHead(HMODULE ntdll, uintptr_t& outHead) {
	using RegisterNotifyFn = NTSTATUS(NTAPI*)(ULONG, PVOID, PVOID, PVOID*);
	using UnregisterNotifyFn = NTSTATUS(NTAPI*)(PVOID);

	auto registerNotify = reinterpret_cast<RegisterNotifyFn>(GetProcAddress(ntdll, oxorany("LdrRegisterDllNotification")));
	auto unregisterNotify = reinterpret_cast<UnregisterNotifyFn>(GetProcAddress(ntdll, oxorany("LdrUnregisterDllNotification")));
	if (!registerNotify || !unregisterNotify) {
		Logging->Warn(Debug, oxorany("LdrRegisterDllNotification Exports Not Found"));
		return false;
	}

	PVOID cookie = nullptr;
	const NTSTATUS status = registerNotify(0, reinterpret_cast<PVOID>(NullDllNotify), nullptr, &cookie);
	if (status < 0 || !cookie) {
		Logging->Warn(Debug, oxorany("LdrRegisterDllNotification Failed, Status = 0x%08X"), status);
		return false;
	}

	LIST_ENTRY links{};
	if (!Utilities->ReadMem(GetCurrentProcess(), reinterpret_cast<uintptr_t>(cookie), &links, sizeof(links))) {
		unregisterNotify(cookie);
		return false;
	}

	outHead = reinterpret_cast<uintptr_t>(links.Flink);
	const uintptr_t altHeadCandidate = reinterpret_cast<uintptr_t>(links.Blink);
	int score = -1;

	// Local list can tear too if another thread registers concurrently.
	// Retry briefly instead of failing on a single torn sample.
	for (int attempt = 0; attempt < 10; ++attempt) {
		score = ScoreListHead(GetCurrentProcess(), outHead);
		if (score >= 80)
			break;

		// LdrRegisterDllNotification currently uses InsertTailList, so
		// entry.Flink == head. If Windows ever switches to InsertHead, the
		// head would be in Blink instead, so try that before giving up.
		if (altHeadCandidate != outHead) {
			const int altScore = ScoreListHead(GetCurrentProcess(), altHeadCandidate);
			if (altScore >= 80) {
				outHead = altHeadCandidate;
				score = altScore;
				break;
			}
		}

		Sleep(10);
	}

	const NTSTATUS unregStatus = unregisterNotify(cookie);
	if (unregStatus < 0)
		Logging->Warn(Debug, oxorany("LdrUnregisterDllNotification Failed, Status = 0x%08X"), unregStatus);

	if (score < 80) {
		Logging->Warn(Debug, oxorany("Notify List Head Validation Failed, Value = %d"), score);
		return false;
	}

	return true;
}

bool ResolveNotifyList(HANDLE process, uintptr_t ntdllBase, uintptr_t& outList) {
	HMODULE localNtdll = GetModuleHandleA(oxorany("ntdll.dll"));
	if (!localNtdll) {
		Logging->Error(Debug, oxorany("Local ntdll.dll Not Loaded"));
		return false;
	}

	uintptr_t localHead = 0;
	if (!ResolveNotifyListHead(localNtdll, localHead))
		return false;

	const uintptr_t rva = localHead - reinterpret_cast<uintptr_t>(localNtdll);

	// Sanity-check the RVA against the remote ntdll image size. If local and
	// remote ntdll versions differ, the RVA can point outside the image and
	// verification will fail on garbage. Catch that here with a clear message.
	{
		IMAGE_DOS_HEADER dos{};
		IMAGE_NT_HEADERS nt{};
		if (!Utilities->ReadMem(process, ntdllBase, &dos, sizeof(dos)) ||
			dos.e_magic != IMAGE_DOS_SIGNATURE ||
			!Utilities->ReadMem(process, ntdllBase + dos.e_lfanew, &nt, sizeof(nt)) ||
			nt.Signature != IMAGE_NT_SIGNATURE) {
			Logging->Error(Debug, oxorany("Failed Reading Remote ntdll Headers"));
			return false;
		}
		const DWORD imageSize = nt.OptionalHeader.SizeOfImage;
		if (rva == 0 || rva >= imageSize) {
			Logging->Error(Debug, oxorany("Notify List RVA Out Of Range (rva=0x%X imageSize=0x%X)"), rva, imageSize);
			return false;
		}
	}

	outList = ntdllBase + rva;
	Logging->Detail(Debug, oxorany("LdrpDllNotificationList @ 0x%p"), reinterpret_cast<void*>(outList));
	return true;
}

bool VerifyNotifyList(HANDLE process, uintptr_t listHead) {
	// The target mutates this list on another thread (DLL loads/unloads).
	// A single RPM can straddle an insert/remove and score -1 on a healthy
	// list, so retry with a small delay instead of failing immediately.
	constexpr int kAttempts = 100;
	constexpr DWORD kDelayMs = 20;

	int score = -1;
	uint64_t flink = 0;
	uint64_t blink = 0;

	for (int attempt = 0; attempt < kAttempts; ++attempt) {
		uint64_t headLinks[2] = {};
		if (!Utilities->ReadMem(process, listHead, headLinks, sizeof(headLinks))) {
			Logging->Error(Debug, oxorany("Failed Reading Notify List Head"));
			Sleep(kDelayMs);
			continue;
		}

		// Stability check: re-read and require identical values. If the
		// list is being mutated right now, back off briefly and retry.
		uint64_t recheck[2] = {};
		if (!Utilities->ReadMem(process, listHead, recheck, sizeof(recheck))) {
			Sleep(kDelayMs);
			continue;
		}
		if (headLinks[0] != recheck[0] || headLinks[1] != recheck[1]) {
			Sleep(5);
			continue;
		}

		flink = headLinks[0];
		blink = headLinks[1];
		score = ScoreListHead(process, listHead);

		if (score >= 80) {
			if (attempt > 0)
				Logging->Detail(Debug, oxorany("Notify List Stable After %d Retries"), attempt);
			Logging->Detail(Debug, oxorany("Flink=0x%llX Blink=0x%llX Value = %d"), flink, blink, score);
			return true;
		}

		Sleep(kDelayMs);
	}

	Logging->Detail(Debug, oxorany("Flink=0x%llX Blink=0x%llX Value = %d"), flink, blink, score);
	// Extra diagnostics for the single-entry case (Flink == Blink): dump what
	// the entry points to so a persistent -1 can be distinguished from a race.
	if (flink != 0 && flink == blink && Utilities->IsUserPointer(flink) && !(flink & 7)) {
		uint64_t entryLinks[2] = {};
		if (Utilities->ReadMem(process, static_cast<uintptr_t>(flink), entryLinks, sizeof(entryLinks))) {
			Logging->Detail(Debug, oxorany("Single Entry 0x%llX -> Flink=0x%llX Blink=0x%llX (head=0x%p)"),
				flink, entryLinks[0], entryLinks[1], reinterpret_cast<void*>(listHead));
		}
	}
	Logging->Error(Debug, oxorany("Notify List Verification Failed"));
	return false;
}

LONG FilterStub(EXCEPTION_POINTERS* info) {
	EXCEPTION_RECORD* record = info->ExceptionRecord;
	if (!record || !record->ExceptionAddress)
		return EXCEPTION_CONTINUE_SEARCH;

	const DWORD ignoredCodes[] = {
		0x40010006, 0x4001000A, 0x406D1388, 0xE06D7363, 0x80000003, 0xC000001D, 0x80000004
	};

	for (DWORD code : ignoredCodes) {
		if (record->ExceptionCode == code)
			return EXCEPTION_CONTINUE_SEARCH;
	}

	return EXCEPTION_CONTINUE_EXECUTION;
}

void LoadModuleStub() {
	auto* ctx = reinterpret_cast<InjectContext*>(ShellcodePlaceholder);
	ctx->LoadLibraryA(ctx->DllPath);
	ctx->SetUnhandledExceptionFilter(reinterpret_cast<LPTOP_LEVEL_EXCEPTION_FILTER>(ctx->ExceptionFilter));
}

int main() {
	SetConsoleTitleA(oxorany("Zynova injector"));

	const std::filesystem::path binDir = Filesystem->GetExecutableDirectory();
	const std::filesystem::path modulePath = binDir / oxorany("Module.dll");

	Logging->Info(Debug, oxorany("Current Directory: %s"), binDir.string().c_str());

	if (!Filesystem->Exists(modulePath)) {
		Logging->Error(Debug, oxorany("Module Not Found: %s"), modulePath.string().c_str());
		return Logging->FailAttach();
	}

	const DWORD pid = Utilities->FindProcessByName(oxorany("RobloxPlayerBeta.exe"));
	if (!pid) {
		Logging->Error(Debug, oxorany("%s Not Found"), oxorany("RobloxPlayerBeta.exe"));
		return Logging->FailAttach();
	}
	Logging->Ok(Debug, oxorany("%s - PID %u"), oxorany("RobloxPlayerBeta.exe"), pid);

	HANDLE process = OpenProcess(PROCESS_ALL_ACCESS - 0x1, FALSE, pid);
	if (process == INVALID_HANDLE_VALUE) {
		Logging->LastError(Debug, oxorany("OpenProcess"));
		return Logging->FailAttach();
	}
	if (uintptr_t existing = Utilities->GetModuleBase(process, oxorany("Module.dll"))) {
		Logging->Ok(Release, oxorany("Already Attached"));
		Logging->Detail(Debug, oxorany("Already Attached, Module @ 0x%p"), reinterpret_cast<void*>(existing));
		CloseHandle(process);
		return 0;
	}

	Filesystem->RestrictFileAccess(modulePath.string().c_str(), false);
	const uintptr_t ntdllBase = Utilities->GetModuleBase(process, oxorany("ntdll.dll"));
	if (!ntdllBase) {
		Logging->Error(Debug, oxorany("Remote ntdll.dll Not Found"));
		return Logging->FailAttach();
	}

	uintptr_t notifyList = 0;
	if (!ResolveNotifyList(process, ntdllBase, notifyList)) {
		Logging->Error(Debug, oxorany("Failed To Resolve LdrpDllNotificationList"));
		return Logging->FailAttach();
	}

	if (!VerifyNotifyList(process, notifyList)) {
		Logging->Error(Debug, oxorany("Notify List Verification Failed"));
		return Logging->FailAttach();
	}

	const uintptr_t ntCreateSection = Utilities->GetExportAddress(process, ntdllBase, oxorany("NtCreateSection"));
	if (!ntCreateSection) {
		Logging->Error(Debug, oxorany("NtCreateSection Export Not Found"));
		return Logging->FailAttach();
	}

	std::vector<uint8_t> sectionPrologue(5);
	if (!ReadProcessMemory(process, reinterpret_cast<PVOID>(ntCreateSection), sectionPrologue.data(), sectionPrologue.size(), nullptr)) {
		Logging->LastError(Debug, oxorany("ReadProcessMemory(NtCreateSection Prologue)"));
		return Logging->FailAttach();
	}

	const uintptr_t sectionHook = Utilities->ResolveJumpTarget(sectionPrologue, ntCreateSection);
	if (!sectionHook) {
		Logging->Error(Debug, oxorany("NtCreateSection Hook Target Not Found"));
		return Logging->FailAttach();
	}

	std::vector<uint8_t> savedHookBytes(15);
	DWORD hookProtect = 0;

	if (!VirtualProtectEx(process, reinterpret_cast<PVOID>(sectionHook), savedHookBytes.size(), PAGE_EXECUTE_READWRITE, &hookProtect)) {
		Logging->LastError(Debug, oxorany("VirtualProtectEx(NtCreateSection Hook)"));
		return Logging->FailAttach();
	}

	if (!ReadProcessMemory(process, reinterpret_cast<PVOID>(sectionHook), savedHookBytes.data(), savedHookBytes.size(), nullptr)) {
		Logging->LastError(Debug, oxorany("ReadProcessMemory(NtCreateSection Hook Bytes)"));
		return Logging->FailAttach();
	}

	if (!WriteProcessMemory(process, reinterpret_cast<PVOID>(sectionHook), kNtCreateSectionBytes.data(), kNtCreateSectionBytes.size(), nullptr)) {
		Logging->LastError(Debug, oxorany("WriteProcessMemory(NtCreateSection UnHook)"));
		return Logging->FailAttach();
	}

	VirtualProtectEx(process, reinterpret_cast<PVOID>(sectionHook), savedHookBytes.size(), hookProtect, &hookProtect);
	Logging->Ok(Debug, oxorany("NtCreateSection UnHooked"));

	std::vector<NotifyEntry> savedEntries;

	uint64_t cursor = 0;
	if (!ReadProcessMemory(process, reinterpret_cast<PVOID>(notifyList), &cursor, sizeof(cursor), nullptr)) {
		Logging->LastError(Debug, oxorany("ReadProcessMemory(Notify List Head)"));
		Utilities->RestoreBytes(process, sectionHook, savedHookBytes, hookProtect);
		return Logging->FailAttach();
	}

	while (cursor != notifyList) {
		NotifyEntry entry{};
		entry.Address = cursor;

		ReadProcessMemory(process, reinterpret_cast<PVOID>(cursor), &entry.Flink, sizeof(uintptr_t), nullptr);
		ReadProcessMemory(process, reinterpret_cast<PVOID>(cursor + 8), &entry.Blink, sizeof(uintptr_t), nullptr);

		if (!entry.Flink || !entry.Blink) {
			Logging->Warn(Debug, oxorany("Notify List Walk Stopped At 0x%llX"), cursor);
			break;
		}

		ReadProcessMemory(process, reinterpret_cast<PVOID>(cursor + sizeof(LIST_ENTRY)), &entry.Callback, sizeof(uintptr_t), nullptr);
		Logging->Detail(Debug, oxorany("Entry 0x%llX, Callback 0x%llX"), entry.Address, entry.Callback);

		savedEntries.push_back(entry);
		cursor = entry.Flink;
	}

	for (const NotifyEntry& entry : savedEntries) {
		WriteProcessMemory(process, reinterpret_cast<PVOID>(entry.Blink), &entry.Flink, sizeof(uintptr_t), nullptr);
		WriteProcessMemory(process, reinterpret_cast<PVOID>(entry.Flink + 8), &entry.Blink, sizeof(uintptr_t), nullptr);
	}

	WriteProcessMemory(process, reinterpret_cast<PVOID>(notifyList), &notifyList, sizeof(uintptr_t), nullptr);
	WriteProcessMemory(process, reinterpret_cast<PVOID>(notifyList + 8), &notifyList, sizeof(uintptr_t), nullptr);
	if (!savedEntries.empty())
		Logging->Ok(Debug, oxorany("Cleared %zu Notify Entries"), savedEntries.size());

	void* sharedMemory = VirtualAllocEx(process, nullptr, sizeof(InjectContext), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (!sharedMemory) {
		Logging->LastError(Debug, oxorany("VirtualAllocEx(Shared Context)"));
		Utilities->RestoreBytes(process, sectionHook, savedHookBytes, hookProtect);
		return Logging->FailAttach();
	}

	const uintptr_t kernel32Base = Utilities->GetModuleBase(process, oxorany("kernel32.dll"));
	if (!kernel32Base) {
		Logging->Error(Debug, oxorany("Remote kernel32.dll Not Found"));
		Utilities->RestoreBytes(process, sectionHook, savedHookBytes, hookProtect);
		return Logging->FailAttach();
	}

	InjectContext context{};
	sprintf_s(context.DllPath, sizeof(context.DllPath), oxorany("%s"), modulePath.string().c_str());
	context.LoadLibraryA = reinterpret_cast<decltype(&::LoadLibraryA)>(Utilities->GetExportAddress(process, kernel32Base, oxorany("LoadLibraryA")));
	context.SetUnhandledExceptionFilter = reinterpret_cast<decltype(&::SetUnhandledExceptionFilter)>(Utilities->GetExportAddress(process, kernel32Base, oxorany("SetUnhandledExceptionFilter")));
	Logging->Detail(Debug, oxorany("LoadLibraryA @ 0x%p"), reinterpret_cast<void*>(context.LoadLibraryA));
	Logging->Detail(Debug, oxorany("SetUnhandledExceptionFilter @ 0x%p"), reinterpret_cast<void*>(context.SetUnhandledExceptionFilter));

	const uintptr_t winstaBase = Utilities->GetModuleBase(process, oxorany("winsta.dll"));
	if (!winstaBase) {
		Logging->Error(Debug, oxorany("Remote winsta.dll Not Found"));
		Utilities->RestoreBytes(process, sectionHook, savedHookBytes, hookProtect);
		return Logging->FailAttach();
	}

	const uintptr_t loaderCave = winstaBase + 0x1000;
	const uintptr_t filterCave = winstaBase + 0x2000;
	Logging->Detail(Debug, oxorany("Loader Cave @ 0x%p"), reinterpret_cast<void*>(loaderCave));
	Logging->Detail(Debug, oxorany("Filter Cave @ 0x%p"), reinterpret_cast<void*>(filterCave));

	auto loaderShellcode = Utilities->CopyFunctionBytes(reinterpret_cast<uintptr_t>(LoadModuleStub));
	Utilities->PatchShellcodeImmediate(loaderShellcode, ShellcodePlaceholder, reinterpret_cast<uintptr_t>(sharedMemory));
	Logging->Detail(Debug, oxorany("Loader Shellcode 0x%zX Bytes"), loaderShellcode.size());

	auto filterShellcode = Utilities->CopyFunctionBytes(reinterpret_cast<uintptr_t>(FilterStub));
	Logging->Detail(Debug, oxorany("Filter Shellcode 0x%zX Bytes"), filterShellcode.size());

	DWORD caveProtect = 0;
	VirtualProtectEx(process, reinterpret_cast<PVOID>(loaderCave), 0x1000, PAGE_EXECUTE_READWRITE, &caveProtect);
	VirtualProtectEx(process, reinterpret_cast<PVOID>(filterCave), 0x1000, PAGE_EXECUTE_READWRITE, &caveProtect);

	std::vector<uint8_t> zeroPage(0x1000, 0);
	WriteProcessMemory(process, reinterpret_cast<PVOID>(loaderCave), zeroPage.data(), zeroPage.size(), nullptr);
	WriteProcessMemory(process, reinterpret_cast<PVOID>(filterCave), zeroPage.data(), zeroPage.size(), nullptr);
	WriteProcessMemory(process, reinterpret_cast<PVOID>(loaderCave), loaderShellcode.data(), loaderShellcode.size(), nullptr);
	WriteProcessMemory(process, reinterpret_cast<PVOID>(filterCave), filterShellcode.data(), filterShellcode.size(), nullptr);

	context.ExceptionFilter = filterCave;
	WriteProcessMemory(process, sharedMemory, &context, sizeof(context), nullptr);

	if (!CThread::CreateThread(process, reinterpret_cast<PVOID>(loaderCave))) {
		Logging->Error(Debug, oxorany("Remote Thread Creation Failed"));
		Utilities->RestoreBytes(process, sectionHook, savedHookBytes, hookProtect);
		return Logging->FailAttach();
	}

	uintptr_t moduleBase = 0;
	while (!(moduleBase = Utilities->GetModuleBase(process, oxorany("Module.dll")))) {
		Sleep(50);
	}

	Utilities->RestoreBytes(process, sectionHook, savedHookBytes, hookProtect);

	for (const NotifyEntry& entry : savedEntries) {
		WriteProcessMemory(process, reinterpret_cast<PVOID>(entry.Address), &entry.Flink, sizeof(uintptr_t), nullptr);
		WriteProcessMemory(process, reinterpret_cast<PVOID>(entry.Address + 8), &entry.Blink, sizeof(uintptr_t), nullptr);
	}

	if (!savedEntries.empty()) {
		const uint64_t first = savedEntries.front().Address;
		const uint64_t last = savedEntries.back().Address;
		WriteProcessMemory(process, reinterpret_cast<PVOID>(notifyList), &first, sizeof(uintptr_t), nullptr);
		WriteProcessMemory(process, reinterpret_cast<PVOID>(notifyList + 8), &last, sizeof(uintptr_t), nullptr);
	}

	Logging->Detail(Debug, oxorany("%s @ 0x%p"), oxorany("Module.dll"), reinterpret_cast<void*>(moduleBase));
	Logging->Ok(Both, oxorany("Attached"));

	CloseHandle(process);
	Filesystem->RestrictFileAccess(modulePath.string().c_str(), true);
	return 0;
}