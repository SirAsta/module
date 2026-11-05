#include "Security.h"

#include "Dependencies/Logging/Logging.h"
#include "Dependencies/Oxorany/Oxorany.h"

#include <bcrypt.h>
#include <intrin.h>
#include <winternl.h>

#include <algorithm>
#include <chrono>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

#pragma comment(lib, "bcrypt.lib")

namespace {
	std::string BytesToHex(const uint8_t* data, size_t length) {
		const std::string hexDigits = std::string(oxorany("0123456789abcdef"));
		std::string out(length * 2, '\0');

		for (size_t index = 0; index < length; ++index) {
			out[index * 2] = hexDigits[(data[index] >> 4) & 0xF];
			out[index * 2 + 1] = hexDigits[data[index] & 0xF];
		}

		return out;
	}

	bool ComputeFileSha256(const std::filesystem::path& filePath, std::string& outHex) {
		std::ifstream file(filePath, std::ios::binary);
		if (!file)
			return false;

		BCRYPT_ALG_HANDLE algorithm = nullptr;
		if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) != 0)
			return false;

		DWORD hashObjectSize = 0;
		DWORD resultSize = 0;
		if (BCryptGetProperty(algorithm, BCRYPT_OBJECT_LENGTH, reinterpret_cast<PUCHAR>(&hashObjectSize), sizeof(hashObjectSize), &resultSize, 0) != 0) {
			BCryptCloseAlgorithmProvider(algorithm, 0);
			return false;
		}

		DWORD hashLength = 0;
		if (BCryptGetProperty(algorithm, BCRYPT_HASH_LENGTH, reinterpret_cast<PUCHAR>(&hashLength), sizeof(hashLength), &resultSize, 0) != 0) {
			BCryptCloseAlgorithmProvider(algorithm, 0);
			return false;
		}

		std::vector<uint8_t> hashObject(hashObjectSize);
		BCRYPT_HASH_HANDLE hashHandle = nullptr;
		if (BCryptCreateHash(algorithm, &hashHandle, hashObject.data(), hashObjectSize, nullptr, 0, 0) != 0) {
			BCryptCloseAlgorithmProvider(algorithm, 0);
			return false;
		}

		std::vector<uint8_t> buffer(64 * 1024);
		while (file) {
			file.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(buffer.size()));
			const std::streamsize bytesRead = file.gcount();
			if (bytesRead <= 0)
				break;

			if (BCryptHashData(hashHandle, buffer.data(), static_cast<ULONG>(bytesRead), 0) != 0) {
				BCryptDestroyHash(hashHandle);
				BCryptCloseAlgorithmProvider(algorithm, 0);
				return false;
			}
		}

		std::vector<uint8_t> hash(hashLength);
		if (BCryptFinishHash(hashHandle, hash.data(), hashLength, 0) != 0) {
			BCryptDestroyHash(hashHandle);
			BCryptCloseAlgorithmProvider(algorithm, 0);
			return false;
		}

		BCryptDestroyHash(hashHandle);
		BCryptCloseAlgorithmProvider(algorithm, 0);

		outHex = BytesToHex(hash.data(), hash.size());
		return true;
	}

	using NtQueryInformationProcess_t = NTSTATUS(NTAPI*)(
		HANDLE ProcessHandle,
		PROCESSINFOCLASS ProcessInformationClass,
		PVOID ProcessInformation,
		ULONG ProcessInformationLength,
		PULONG ReturnLength);

	NtQueryInformationProcess_t GetNtQueryInformationProcess() {
		static const auto query = reinterpret_cast<NtQueryInformationProcess_t>(
			GetProcAddress(GetModuleHandleW(oxorany(L"ntdll.dll")), oxorany("NtQueryInformationProcess")));
		return query;
	}

	bool QueryProcess(HANDLE process, PROCESSINFOCLASS infoClass, PVOID buffer, ULONG size) {
		const auto query = GetNtQueryInformationProcess();
		if (!query)
			return false;

		ULONG returnLength = 0;
		return query(process, infoClass, buffer, size, &returnLength) >= 0;
	}

	bool HasHardwareBreakpoints() {
		CONTEXT context{};
		context.ContextFlags = CONTEXT_DEBUG_REGISTERS;
		if (!GetThreadContext(GetCurrentThread(), &context))
			return false;

		return context.Dr0 || context.Dr1 || context.Dr2 || context.Dr3;
	}

	bool HasPebDebugIndicators() {
#ifdef _WIN64
		const auto* peb = reinterpret_cast<const uint8_t*>(__readgsqword(0x60));
		constexpr size_t kNtGlobalFlagOffset = 0xBC;
#else
		const auto* peb = reinterpret_cast<const uint8_t*>(__readfsdword(0x30));
		constexpr size_t kNtGlobalFlagOffset = 0x68;
#endif
		if (!peb)
			return false;

		if (peb[2] != 0)
			return true;

		const ULONG ntGlobalFlag = *reinterpret_cast<const ULONG*>(peb + kNtGlobalFlagOffset);
		return (ntGlobalFlag & 0x70) != 0;
	}

	std::wstring ToLowerWide(std::wstring value) {
		std::transform(value.begin(), value.end(), value.begin(), ::towlower);
		return value;
	}

	bool IsWordChar(wchar_t character) {
		return iswalnum(character) || character == L'_';
	}

	bool ContainsKeywordAt(const std::wstring& text, size_t position, const std::wstring& keyword) {
		if (position + keyword.size() > text.size())
			return false;

		if (text.compare(position, keyword.size(), keyword) != 0)
			return false;

		if (position > 0 && IsWordChar(text[position - 1]))
			return false;

		const size_t end = position + keyword.size();
		if (end < text.size() && IsWordChar(text[end]))
			return false;

		return true;
	}

	bool ContainsDebuggerWindowKeyword(const std::wstring& text) {
		static const std::wstring kWindowKeywords[] = {
			std::wstring(oxorany(L"x64dbg")),
			std::wstring(oxorany(L"x32dbg")),
			std::wstring(oxorany(L"ollydbg")),
			std::wstring(oxorany(L"ida -")),
			std::wstring(oxorany(L"ida:")),
			std::wstring(oxorany(L"ida v")),
			std::wstring(oxorany(L"ida pro")),
			std::wstring(oxorany(L"interactive disassembler")),
			std::wstring(oxorany(L"windbg")),
			std::wstring(oxorany(L"windbg:")),
			std::wstring(oxorany(L"windbg -")),
			std::wstring(oxorany(L"cheat engine")),
			std::wstring(oxorany(L"dnspy")),
			std::wstring(oxorany(L"process hacker")),
			std::wstring(oxorany(L"process hacker 2")),
			std::wstring(oxorany(L"process hacker 3")),
			std::wstring(oxorany(L"system informer")),
			std::wstring(oxorany(L"sleep engine")),
			std::wstring(oxorany(L"ghidra:")),
			std::wstring(oxorany(L"ghidra -")),
			std::wstring(oxorany(L"ghidra v")),
			std::wstring(oxorany(L"immunity debugger")),
			std::wstring(oxorany(L"binary ninja")),
			std::wstring(oxorany(L"frida")),
			std::wstring(oxorany(L"pestudio")),
			std::wstring(oxorany(L"scylla")),
			std::wstring(oxorany(L"scylla hide")),
			std::wstring(oxorany(L"hxd -")),
			std::wstring(oxorany(L"reclass")),
			std::wstring(oxorany(L"reclass.net")),
			std::wstring(oxorany(L"reclass64")),
			std::wstring(oxorany(L"radare2")),
			std::wstring(oxorany(L"r2 -")),
			std::wstring(oxorany(L"cutter -")),
			std::wstring(oxorany(L"cutter:")),
			std::wstring(oxorany(L"rizin")),
			std::wstring(oxorany(L"gdb -")),
			std::wstring(oxorany(L"gdb:")),
			std::wstring(oxorany(L"edb -")),
			std::wstring(oxorany(L"edb:")),
			std::wstring(oxorany(L"evan's debugger")),
			std::wstring(oxorany(L"evans debugger")),
			std::wstring(oxorany(L"lldb")),
			std::wstring(oxorany(L"softice")),
			std::wstring(oxorany(L"syser")),
			std::wstring(oxorany(L"syser debugger")),
			std::wstring(oxorany(L"ntsd")),
			std::wstring(oxorany(L"cdb -")),
			std::wstring(oxorany(L"kd -")),
			std::wstring(oxorany(L"hyperdbg")),
			std::wstring(oxorany(L"hyperdbg:")),
			std::wstring(oxorany(L"dbgx")),
			std::wstring(oxorany(L"visual studio - debugger")),
			std::wstring(oxorany(L"vs debugger")),
			std::wstring(oxorany(L"process monitor")),
			std::wstring(oxorany(L"procmon")),
			std::wstring(oxorany(L"procmon64")),
			std::wstring(oxorany(L"process explorer")),
			std::wstring(oxorany(L"procexp")),
			std::wstring(oxorany(L"procexp64")),
			std::wstring(oxorany(L"api monitor")),
			std::wstring(oxorany(L"apimonitor")),
			std::wstring(oxorany(L"api monitor v2")),
			std::wstring(oxorany(L"wireshark")),
			std::wstring(oxorany(L"fiddler")),
			std::wstring(oxorany(L"fiddler classic")),
			std::wstring(oxorany(L"fiddler everywhere")),
			std::wstring(oxorany(L"charles -")),
			std::wstring(oxorany(L"charles proxy")),
			std::wstring(oxorany(L"httpdebugger")),
			std::wstring(oxorany(L"http debugger")),
			std::wstring(oxorany(L"http debugger pro")),
			std::wstring(oxorany(L"proxifier")),
			std::wstring(oxorany(L"de4dot")),
			std::wstring(oxorany(L"dotpeek")),
			std::wstring(oxorany(L"dotpeek64")),
			std::wstring(oxorany(L"ilspy")),
			std::wstring(oxorany(L"justdecompile")),
			std::wstring(oxorany(L"reflexil")),
			std::wstring(oxorany(L"dnlib")),
			std::wstring(oxorany(L".net reflector")),
			std::wstring(oxorany(L"telerik justdecompile")),
			std::wstring(oxorany(L"dotnet reflector")),
			std::wstring(oxorany(L"redgate reflector")),
			std::wstring(oxorany(L"resource hacker")),
			std::wstring(oxorany(L"reshacker")),
			std::wstring(oxorany(L"resource tuner")),
			std::wstring(oxorany(L"exeinfope")),
			std::wstring(oxorany(L"exeinfo pe")),
			std::wstring(oxorany(L"peid")),
			std::wstring(oxorany(L"pe-bear")),
			std::wstring(oxorany(L"pe bear")),
			std::wstring(oxorany(L"pe explorer")),
			std::wstring(oxorany(L"cff explorer")),
			std::wstring(oxorany(L"pe studio")),
			std::wstring(oxorany(L"lordpe")),
			std::wstring(oxorany(L"lord pe")),
			std::wstring(oxorany(L"importrec")),
			std::wstring(oxorany(L"import reconstructor")),
			std::wstring(oxorany(L"protection id")),
			std::wstring(oxorany(L"detect it easy")),
			std::wstring(oxorany(L"die -")),
			std::wstring(oxorany(L"hollows hunter")),
			std::wstring(oxorany(L"pe-sieve")),
			std::wstring(oxorany(L"pe sieve")),
			std::wstring(oxorany(L"mal_unpack")),
			std::wstring(oxorany(L"dumpe2e")),
			std::wstring(oxorany(L"x64_dbg")),
			std::wstring(oxorany(L"x32_dbg")),
			std::wstring(oxorany(L"bochs")),
			std::wstring(oxorany(L"bochs debugger")),
			std::wstring(oxorany(L"qemu -")),
			std::wstring(oxorany(L"vmware debugger")),
			std::wstring(oxorany(L"virtualbox debugger")),
			std::wstring(oxorany(L"titan hide")),
			std::wstring(oxorany(L"titanhide")),
			std::wstring(oxorany(L"sharpod")),
			std::wstring(oxorany(L"iscoderz")),
			std::wstring(oxorany(L"scriptbay")),
			std::wstring(oxorany(L"cheatengine")),
			std::wstring(oxorany(L"ce -")),
			std::wstring(oxorany(L"cheat engine:")),
			std::wstring(oxorany(L"winhex")),
			std::wstring(oxorany(L"winhex -")),
			std::wstring(oxorany(L"010 editor")),
			std::wstring(oxorany(L"010editor")),
			std::wstring(oxorany(L"hex workshop")),
			std::wstring(oxorany(L"hexworkshop")),
			std::wstring(oxorany(L"hexed.it")),
			std::wstring(oxorany(L"frhed")),
			std::wstring(oxorany(L"hex fiend")),
			std::wstring(oxorany(L"imhex")),
			std::wstring(oxorany(L"imhex -")),
			std::wstring(oxorany(L"hexinator")),
			std::wstring(oxorany(L"sysinternals")),
			std::wstring(oxorany(L"autoruns")),
			std::wstring(oxorany(L"tcpview")),
			std::wstring(oxorany(L"regshot")),
			std::wstring(oxorany(L"regshot:")),
			std::wstring(oxorany(L"spy++")),
			std::wstring(oxorany(L"spyxx")),
			std::wstring(oxorany(L"winsight")),
			std::wstring(oxorany(L"window detective")),
			std::wstring(oxorany(L"winspy")),
			std::wstring(oxorany(L"winspy++")),
			std::wstring(oxorany(L"accesschk")),
			std::wstring(oxorany(L"handle -")),
			std::wstring(oxorany(L"vmmap")),
			std::wstring(oxorany(L"rammap")),
			std::wstring(oxorany(L"strings -")),
			std::wstring(oxorany(L"listdlls")),
			std::wstring(oxorany(L"depends -")),
			std::wstring(oxorany(L"dependency walker")),
			std::wstring(oxorany(L"dependencies -")),
			std::wstring(oxorany(L"dumpbin")),
			std::wstring(oxorany(L"ltrace")),
			std::wstring(oxorany(L"strace")),
			std::wstring(oxorany(L"dtrace")),
			std::wstring(oxorany(L"apk easy tool")),
			std::wstring(oxorany(L"jadx")),
			std::wstring(oxorany(L"bytecode viewer")),
			std::wstring(oxorany(L"recaf")),
			std::wstring(oxorany(L"java decompiler")),
			std::wstring(oxorany(L"procyon")),
			std::wstring(oxorany(L"fernflower")),
			std::wstring(oxorany(L"cfr -")),
			std::wstring(oxorany(L"snowman")),
			std::wstring(oxorany(L"retdec")),
			std::wstring(oxorany(L"retdec -")),
			std::wstring(oxorany(L"binarlyai")),
			std::wstring(oxorany(L"binja")),
			std::wstring(oxorany(L"hopper")),
			std::wstring(oxorany(L"hopper disassembler")),
			std::wstring(oxorany(L"jeb -")),
			std::wstring(oxorany(L"jeb decompiler")),
			std::wstring(oxorany(L"ffdec")),
			std::wstring(oxorany(L"jpexs")),
			std::wstring(oxorany(L"nidhogg")),
			std::wstring(oxorany(L"mhyprot")),
			std::wstring(oxorany(L"kdmapper")),
			std::wstring(oxorany(L"kdmapper:")),
			std::wstring(oxorany(L"gwinternals")),
			std::wstring(oxorany(L"rohitab api monitor")),
			std::wstring(oxorany(L"sniff_hit")),
			std::wstring(oxorany(L"capstone")),
			std::wstring(oxorany(L"unicorn")),
			std::wstring(oxorany(L"keystone")),
			std::wstring(oxorany(L"pin -")),
			std::wstring(oxorany(L"intel pin")),
			std::wstring(oxorany(L"dynamorio")),
			std::wstring(oxorany(L"drstrace")),
			std::wstring(oxorany(L"drsyms")),
			std::wstring(oxorany(L"valgrind")),
			std::wstring(oxorany(L"apitrace")),
			std::wstring(oxorany(L"renderdoc")),
			std::wstring(oxorany(L"nsight")),
			std::wstring(oxorany(L"pix -")),
			std::wstring(oxorany(L"intel vtune")),
			std::wstring(oxorany(L"amd uprof")),
			std::wstring(oxorany(L"dumpchk")),
			std::wstring(oxorany(L"windbgx")),
			std::wstring(oxorany(L"windows debugger")),
			std::wstring(oxorany(L"drwatson")),
			std::wstring(oxorany(L"dr. watson")),
			std::wstring(oxorany(L"adplus")),
			std::wstring(oxorany(L"userdump")),
			std::wstring(oxorany(L"procdump")),
			std::wstring(oxorany(L"ollyice")),
			std::wstring(oxorany(L"ollydbg 2")),
			std::wstring(oxorany(L"phantom-evasion")),
			std::wstring(oxorany(L"xdbg")),
			std::wstring(oxorany(L"x96dbg")),
		};

		const std::wstring lowerText = ToLowerWide(text);
		for (const std::wstring& keyword : kWindowKeywords) {
			size_t position = 0;
			while ((position = lowerText.find(keyword, position)) != std::wstring::npos) {
				if (ContainsKeywordAt(lowerText, position, keyword))
					return true;

				++position;
			}
		}

		return false;
	}

	bool MatchesDebuggerWindowClass(const std::wstring& className) {
		static const std::wstring kWindowClasses[] = {
			std::wstring(oxorany(L"ollydbg")),
			std::wstring(oxorany(L"windbgframeclass")),
		};

		const std::wstring lowerClass = ToLowerWide(className);
		for (const std::wstring& debuggerClass : kWindowClasses) {
			if (lowerClass == debuggerClass)
				return true;
		}

		return false;
	}

	struct DebuggerWindowScanContext {
		bool Found = false;
		std::wstring MatchedText;
	};

	BOOL CALLBACK EnumDebuggerWindowsCallback(HWND window, LPARAM param) {
		auto* context = reinterpret_cast<DebuggerWindowScanContext*>(param);
		if (!IsWindowVisible(window))
			return TRUE;

		wchar_t title[512]{};
		const int titleLength = GetWindowTextW(window, title, static_cast<int>(sizeof(title) / sizeof(title[0])));

		if (titleLength > 0 && ContainsDebuggerWindowKeyword(title)) {
			context->Found = true;
			context->MatchedText = title;
			return FALSE;
		}

		wchar_t className[256]{};
		if (GetClassNameW(window, className, static_cast<int>(sizeof(className) / sizeof(className[0])))
			&& MatchesDebuggerWindowClass(className)) {
			context->Found = true;
			context->MatchedText = titleLength > 0 ? title : className;
			return FALSE;
		}

		return TRUE;
	}
}

bool CSecurity::IsLocalProcessDebugged() {
	if (IsDebuggerPresent())
		return true;

	BOOL remoteDebugger = FALSE;
	if (CheckRemoteDebuggerPresent(GetCurrentProcess(), &remoteDebugger) && remoteDebugger)
		return true;

	if (HasHardwareBreakpoints())
		return true;

	if (HasPebDebugIndicators())
		return true;

	HANDLE process = GetCurrentProcess();

	DWORD_PTR debugPort = 0;
	if (QueryProcess(process, static_cast<PROCESSINFOCLASS>(7), &debugPort, sizeof(debugPort)) && debugPort != 0)
		return true;

	HANDLE debugObject = nullptr;
	if (QueryProcess(process, static_cast<PROCESSINFOCLASS>(30), &debugObject, sizeof(debugObject)) && debugObject)
		return true;

	ULONG debugFlags = 0;
	if (QueryProcess(process, static_cast<PROCESSINFOCLASS>(31), &debugFlags, sizeof(debugFlags)) && debugFlags == 0)
		return true;

	return false;
}

bool CSecurity::IsRemoteProcessDebugged(HANDLE process) {
	if (!process || process == INVALID_HANDLE_VALUE)
		return false;

	BOOL remoteDebugger = FALSE;
	if (CheckRemoteDebuggerPresent(process, &remoteDebugger) && remoteDebugger)
		return true;

	DWORD_PTR debugPort = 0;
	if (QueryProcess(process, static_cast<PROCESSINFOCLASS>(7), &debugPort, sizeof(debugPort)) && debugPort != 0)
		return true;

	HANDLE debugObject = nullptr;
	if (QueryProcess(process, static_cast<PROCESSINFOCLASS>(30), &debugObject, sizeof(debugObject)) && debugObject)
		return true;

	ULONG debugFlags = 0;
	if (QueryProcess(process, static_cast<PROCESSINFOCLASS>(31), &debugFlags, sizeof(debugFlags)) && debugFlags == 0)
		return true;

	return false;
}

bool CSecurity::IsDebuggerWindowOpen() {
	DebuggerWindowScanContext context{};
	EnumWindows(EnumDebuggerWindowsCallback, reinterpret_cast<LPARAM>(&context));

	if (context.Found)
		Logging->Detail(Debug, oxorany("Debugger Window Detected: %ls"), context.MatchedText.c_str());

	return context.Found;
}

bool CSecurity::DetectDebugger(HANDLE remoteProcess) {
	if (IsLocalProcessDebugged()
		|| IsDebuggerWindowOpen()
		|| (remoteProcess && IsRemoteProcessDebugged(remoteProcess))) {
		Logging->Error(Both, oxorany("Debugger Detected"));
		return true;
	}

	return false;
}

bool CSecurity::CheckHash(const std::filesystem::path& filePath) {
	std::string hash;
	if (!ComputeFileSha256(filePath, hash)) {
		Logging->Error(Debug, oxorany("Failed To Hash Module: %s"), filePath.string().c_str());
		return false;
	}

	if (_stricmp(hash.c_str(), oxorany("35efe12d07586fd8ba168e364dda5269f246a7980a029b6f624669b52304122b")) != 0) {
		Logging->Error(Debug, oxorany("Module Hash Mismatch"));
		Logging->Detail(Debug, oxorany("Actual:   %s"), hash.c_str());
		return false;
	}

	Logging->Ok(Debug, oxorany("Module Hash Verified"));
	return true;
}

CSecurity::~CSecurity() {
	StopMonitor();
}

void CSecurity::MonitorLoop() {
	while (m_MonitorRunning.load(std::memory_order_acquire)) {
		const HANDLE remoteProcess = reinterpret_cast<HANDLE>(m_MonitorTarget.load(std::memory_order_acquire));

		if (DetectDebugger(remoteProcess)) {
			m_DebuggerDetected.store(true, std::memory_order_release);
			m_MonitorRunning.store(false, std::memory_order_release);
			ExitProcess(1);
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(500));
	}
}

void CSecurity::StartMonitor(HANDLE remoteProcess) {
	StopMonitor();

	m_DebuggerDetected.store(false, std::memory_order_release);
	m_MonitorTarget.store(reinterpret_cast<uintptr_t>(remoteProcess), std::memory_order_release);
	m_MonitorRunning.store(true, std::memory_order_release);
	m_MonitorThread = std::thread([this]() { MonitorLoop(); });
}

void CSecurity::SetMonitorTarget(HANDLE remoteProcess) {
	m_MonitorTarget.store(reinterpret_cast<uintptr_t>(remoteProcess), std::memory_order_release);
}

void CSecurity::StopMonitor() {
	m_MonitorRunning.store(false, std::memory_order_release);

	if (m_MonitorThread.joinable())
		m_MonitorThread.join();

	m_MonitorTarget.store(0, std::memory_order_release);
}

bool CSecurity::WasDebuggerDetected() const {
	return m_DebuggerDetected.load(std::memory_order_acquire);
}