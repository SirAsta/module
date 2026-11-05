#pragma once

#include <Windows.h>
#include <cstdio>
#include <memory>

#include "Dependencies/Oxorany/Oxorany.h"

enum class LogLevel { Info, Ok, Warn, Error, Detail };

enum LogTarget { Debug, Release, Both };

class CLogging {
public:
	bool Debugging = true;

	template<typename... Args>
	void Info(LogTarget target, const char* fmt, Args... args) {
		Write(LogLevel::Info, target, fmt, args...);
	}

	template<typename... Args>
	void Ok(LogTarget target, const char* fmt, Args... args) {
		Write(LogLevel::Ok, target, fmt, args...);
	}

	template<typename... Args>
	void Warn(LogTarget target, const char* fmt, Args... args) {
		Write(LogLevel::Warn, target, fmt, args...);
	}

	template<typename... Args>
	void Error(LogTarget target, const char* fmt, Args... args) {
		Write(LogLevel::Error, target, fmt, args...);
	}

	template<typename... Args>
	void Detail(LogTarget target, const char* fmt, Args... args) {
		Write(LogLevel::Detail, target, fmt, args...);
	}

	void LastError(LogTarget target, const char* step) {
		Error(target, oxorany("%s failed (GetLastError=0x%08X)"), step, GetLastError());
	}

	void Print(const char* tag, const char* tagColor, const char* message);
	int FailAttach();

private:
	bool ConsoleReady = false;

	void InitConsole();
	bool ShouldLog(LogTarget target) const;
	void Emit(LogLevel level, const char* message);

	template<typename... Args>
	void Write(LogLevel level, LogTarget target, const char* fmt, Args... args) {
		if (!ShouldLog(target))
			return;

		char message[2048]{};
		snprintf(message, sizeof(message), fmt, args...);
		Emit(level, message);
	}
};

inline auto Logging = std::make_unique<CLogging>();
