#pragma once

#include <Windows.h>

#include <cstdint>
#include <memory>
#include <vector>

class CUtilities {
public:
	DWORD FindProcessByName(const char* name);
	uintptr_t GetModuleBase(HANDLE process, const char* name);

	bool ReadMem(HANDLE process, uintptr_t address, void* buffer, size_t size);
	bool WriteMem(HANDLE process, uintptr_t address, const void* buffer, size_t size);
	bool ProtectMem(HANDLE process, uintptr_t address, size_t size, DWORD newProtect, DWORD* oldProtect);

	bool IsUserPointer(uint64_t value);
	bool LooksLikePointer(uint64_t value);

	uintptr_t GetExportAddress(HANDLE process, uintptr_t moduleBase, const char* exportName);
	uintptr_t ResolveJumpTarget(const std::vector<uint8_t>& bytes, uint64_t instructionAddress);

	std::vector<uint8_t> CopyFunctionBytes(uintptr_t function);
	void PatchShellcodeImmediate(std::vector<uint8_t>& data, uint64_t searchValue, uint64_t replaceValue);

	bool RestoreBytes(HANDLE process, uintptr_t address, const std::vector<uint8_t>& originalBytes, DWORD oldProtect);
};

inline auto Utilities = std::make_unique<CUtilities>();