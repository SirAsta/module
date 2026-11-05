#include "Filesystem.h"

#include "Dependencies/Oxorany/Oxorany.h"

#include <aclapi.h>
#include <sddl.h>
#include <string>

std::filesystem::path CFilesystem::GetExecutablePath() {
	char exePath[MAX_PATH]{};
	GetModuleFileNameA(nullptr, exePath, MAX_PATH);
	return std::filesystem::path(exePath);
}

std::filesystem::path CFilesystem::GetExecutableDirectory() {
	return GetExecutablePath().parent_path();
}

bool CFilesystem::Exists(const std::filesystem::path& path) {
	return std::filesystem::exists(path);
}

void CFilesystem::RestrictFileAccess(const char* path, bool restrict) {
	PSECURITY_DESCRIPTOR securityDescriptor = nullptr;
	PACL dacl = nullptr;

	const std::wstring sddlString = restrict
		? std::wstring(oxorany(L"D:(D;;FA;;;WD)"))
		: std::wstring(oxorany(L"D:(A;;FA;;;WD)"));

	wchar_t widePath[MAX_PATH]{};
	MultiByteToWideChar(CP_UTF8, 0, path, -1, widePath, MAX_PATH);

	if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(
		sddlString.c_str(),
		SDDL_REVISION_1,
		&securityDescriptor,
		nullptr)) {
		return;
	}

	BOOL daclPresent = FALSE;
	BOOL daclDefaulted = FALSE;
	GetSecurityDescriptorDacl(securityDescriptor, &daclPresent, &dacl, &daclDefaulted);

	SetNamedSecurityInfoW(
		widePath,
		SE_FILE_OBJECT,
		DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION,
		nullptr,
		nullptr,
		dacl,
		nullptr);

	LocalFree(securityDescriptor);
}
