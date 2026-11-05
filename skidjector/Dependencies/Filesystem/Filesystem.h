#pragma once

#include <Windows.h>

#include <filesystem>
#include <memory>

class CFilesystem {
public:
	std::filesystem::path GetExecutablePath();
	std::filesystem::path GetExecutableDirectory();
	bool Exists(const std::filesystem::path& path);

	void RestrictFileAccess(const char* path, bool restrict);
};

inline auto Filesystem = std::make_unique<CFilesystem>();
