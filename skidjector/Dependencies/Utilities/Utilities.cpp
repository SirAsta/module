#include "Utilities.h"

#include "Dependencies/Logging/Logging.h"
#include "Dependencies/Oxorany/Oxorany.h"

#include <Psapi.h>
#include <TlHelp32.h>

#include <cstring>
#include <string>

DWORD CUtilities::FindProcessByName(const char* name) {
	const std::wstring wideName(name, name + strlen(name));

	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (snapshot == INVALID_HANDLE_VALUE)
		return 0;

	PROCESSENTRY32W entry{};
	entry.dwSize = sizeof(entry);

	DWORD pid = 0;
	if (Process32FirstW(snapshot, &entry)) {
		do {
			if (!_wcsicmp(entry.szExeFile, wideName.c_str())) {
				pid = entry.th32ProcessID;
				break;
			}
		} while (Process32NextW(snapshot, &entry));
	}

	CloseHandle(snapshot);
	return pid;
}

uintptr_t CUtilities::GetModuleBase(HANDLE process, const char* name) {
	if (!process)
		return 0;

	HMODULE modules[1024]{};
	DWORD needed = 0;
	if (!EnumProcessModulesEx(process, modules, sizeof(modules), &needed, LIST_MODULES_ALL))
		return 0;

	const unsigned count = needed / sizeof(HMODULE);
	for (unsigned i = 0; i < count; ++i) {
		char moduleName[MAX_PATH]{};
		if (!GetModuleBaseNameA(process, modules[i], moduleName, sizeof(moduleName)))
			continue;
		if (_stricmp(moduleName, name) == 0)
			return reinterpret_cast<uintptr_t>(modules[i]);
	}

	return 0;
}

bool CUtilities::ReadMem(HANDLE process, uintptr_t address, void* buffer, size_t size) {
	if (process == GetCurrentProcess()) {
		std::memcpy(buffer, reinterpret_cast<const void*>(address), size);
		return true;
	}

	SIZE_T read = 0;
	if (!ReadProcessMemory(process, reinterpret_cast<LPCVOID>(address), buffer, size, &read)) {
		Logging->LastError(Debug, oxorany("ReadProcessMemory"));
		Logging->Detail(Debug, oxorany("address=0x%p size=0x%zX read=0x%zX"), reinterpret_cast<void*>(address), size, read);
		return false;
	}

	if (read != size) {
		Logging->Error(Debug, oxorany("ReadProcessMemory short read at 0x%p"), reinterpret_cast<void*>(address));
		Logging->Detail(Debug, oxorany("expected 0x%zX bytes got 0x%zX"), size, read);
		return false;
	}

	return true;
}

bool CUtilities::WriteMem(HANDLE process, uintptr_t address, const void* buffer, size_t size) {
	if (!WriteProcessMemory(process, reinterpret_cast<PVOID>(address), buffer, size, nullptr)) {
		Logging->LastError(Debug, oxorany("WriteProcessMemory"));
		Logging->Detail(Debug, oxorany("address=0x%p size=0x%zX"), reinterpret_cast<void*>(address), size);
		return false;
	}

	return true;
}

bool CUtilities::ProtectMem(HANDLE process, uintptr_t address, size_t size, DWORD newProtect, DWORD* oldProtect) {
	if (!VirtualProtectEx(process, reinterpret_cast<PVOID>(address), size, newProtect, oldProtect)) {
		Logging->LastError(Debug, oxorany("VirtualProtectEx"));
		Logging->Detail(Debug, oxorany("address=0x%p size=0x%zX"), reinterpret_cast<void*>(address), size);
		return false;
	}

	return true;
}

bool CUtilities::IsUserPointer(uint64_t value) {
	return value >= 0x10000 && value < 0x00007FFFFFFFFFFFULL;
}

bool CUtilities::LooksLikePointer(uint64_t value) {
	if (!IsUserPointer(value))
		return false;

	
	
	if (value & 7ULL)
		return false;

	return true;
}

uintptr_t CUtilities::GetExportAddress(HANDLE process, uintptr_t moduleBase, const char* exportName) {
	IMAGE_DOS_HEADER dosHeader{};
	if (!ReadProcessMemory(process, reinterpret_cast<LPCVOID>(moduleBase), &dosHeader, sizeof(dosHeader), nullptr))
		return 0;

	IMAGE_NT_HEADERS ntHeaders{};
	if (!ReadProcessMemory(process, reinterpret_cast<LPCVOID>(moduleBase + dosHeader.e_lfanew), &ntHeaders, sizeof(ntHeaders), nullptr))
		return 0;

	const auto& exportDir = ntHeaders.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
	if (!exportDir.VirtualAddress || !exportDir.Size)
		return 0;

	IMAGE_EXPORT_DIRECTORY exports{};
	if (!ReadProcessMemory(process, reinterpret_cast<LPCVOID>(moduleBase + exportDir.VirtualAddress), &exports, sizeof(exports), nullptr))
		return 0;

	std::vector<DWORD> names(exports.NumberOfNames);
	std::vector<WORD> ordinals(exports.NumberOfNames);
	std::vector<DWORD> functions(exports.NumberOfFunctions);

	if (!ReadProcessMemory(process, reinterpret_cast<LPCVOID>(moduleBase + exports.AddressOfNames), names.data(), names.size() * sizeof(DWORD), nullptr)
		|| !ReadProcessMemory(process, reinterpret_cast<LPCVOID>(moduleBase + exports.AddressOfNameOrdinals), ordinals.data(), ordinals.size() * sizeof(WORD), nullptr)
		|| !ReadProcessMemory(process, reinterpret_cast<LPCVOID>(moduleBase + exports.AddressOfFunctions), functions.data(), functions.size() * sizeof(DWORD), nullptr)) {
		return 0;
	}

	char nameBuffer[256]{};
	for (size_t i = 0; i < names.size(); ++i) {
		if (!ReadProcessMemory(process, reinterpret_cast<LPCVOID>(moduleBase + names[i]), nameBuffer, sizeof(nameBuffer) - 1, nullptr))
			continue;
		if (strcmp(nameBuffer, exportName) != 0)
			continue;

		const WORD ordinal = ordinals[i];
		if (ordinal >= functions.size())
			return 0;

		return moduleBase + functions[ordinal];
	}

	return 0;
}

uintptr_t CUtilities::ResolveJumpTarget(const std::vector<uint8_t>& bytes, uint64_t instructionAddress) {
	if (bytes.size() >= 12 && bytes[0] == 0x48 && bytes[1] == 0xB8 && bytes[10] == 0xFF && bytes[11] == 0xE0) {
		uint64_t target = 0;
		std::memcpy(&target, bytes.data() + 2, sizeof(target));
		return target;
	}

	if (bytes.size() >= 14 && bytes[0] == 0xFF && bytes[1] == 0x25) {
		uint64_t target = 0;
		std::memcpy(&target, bytes.data() + 6, sizeof(target));
		return target;
	}

	if (bytes.size() >= 6 && bytes[0] == 0xFF && bytes[1] == 0x25) {
		int32_t displacement = 0;
		std::memcpy(&displacement, bytes.data() + 2, sizeof(displacement));
		const uint64_t rip = instructionAddress + 6;
		return *reinterpret_cast<uint64_t*>(rip + displacement);
	}

	if (bytes.size() >= 5 && bytes[0] == 0xE9) {
		int32_t offset = 0;
		std::memcpy(&offset, bytes.data() + 1, sizeof(offset));
		return instructionAddress + 5 + offset;
	}

	return 0;
}

std::vector<uint8_t> CUtilities::CopyFunctionBytes(uintptr_t function) {
	MEMORY_BASIC_INFORMATION region{};
	VirtualQuery(reinterpret_cast<void*>(function), &region, sizeof(region));

	std::vector<uint8_t> bytes;
	bytes.reserve(region.RegionSize);

	for (size_t i = 0; i < region.RegionSize; ++i) {
		const uint8_t value = *reinterpret_cast<uint8_t*>(function + i);
		bytes.push_back(value);

		if (value == 0xCC
			&& *reinterpret_cast<uint8_t*>(function + i + 1) == 0xCC
			&& *reinterpret_cast<uint8_t*>(function + i + 2) == 0xCC) {
			break;
		}
	}

	return bytes;
}

void CUtilities::PatchShellcodeImmediate(std::vector<uint8_t>& data, uint64_t searchValue, uint64_t replaceValue) {
	for (size_t i = 0; i + 10 <= data.size(); ++i) {
		const bool regMove = (data[i] == 0x48 || data[i] == 0x49) && data[i + 1] >= 0xB8 && data[i + 1] <= 0xBF;
		if (regMove) {
			const uint64_t immediate = *reinterpret_cast<uint64_t*>(&data[i + 2]);
			const uint32_t offset = *reinterpret_cast<uint32_t*>(&data[i + 2]);
			if (immediate - offset == searchValue) {
				const uint64_t patched = replaceValue + offset;
				std::memcpy(&data[i + 2], &patched, sizeof(patched));
			}
		}

		const bool absMove = data[i] == 0xA1 || data[i] == 0xA2 || data[i] == 0xA3;
		if (absMove) {
			const uint64_t immediate = *reinterpret_cast<uint64_t*>(&data[i + 1]);
			const uint32_t offset = *reinterpret_cast<uint32_t*>(&data[i + 1]);
			if (immediate - offset == searchValue) {
				const uint64_t patched = replaceValue + offset;
				std::memcpy(&data[i + 1], &patched, sizeof(patched));
			}
		}
	}
}

bool CUtilities::RestoreBytes(HANDLE process, uintptr_t address, const std::vector<uint8_t>& originalBytes, DWORD oldProtect) {
	if (!ProtectMem(process, address, originalBytes.size(), PAGE_EXECUTE_READWRITE, &oldProtect))
		return false;

	if (!WriteMem(process, address, originalBytes.data(), originalBytes.size()))
		return false;

	DWORD ignored = 0;
	VirtualProtectEx(process, reinterpret_cast<PVOID>(address), originalBytes.size(), oldProtect, &ignored);
	return true;
}