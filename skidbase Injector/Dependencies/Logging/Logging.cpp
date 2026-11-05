#include "Logging.h"
#include "Dependencies/Oxorany/Oxorany.h"

void CLogging::InitConsole() {
	if (ConsoleReady)
		return;

	ConsoleReady = true;
	SetConsoleOutputCP(CP_UTF8);

	if (HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE)) {
		DWORD mode = 0;
		if (GetConsoleMode(out, &mode))
			SetConsoleMode(out, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
	}
}

void CLogging::Print(const char* tag, const char* tagColor, const char* message) {
	InitConsole();

	SYSTEMTIME time{};
	GetLocalTime(&time);

	printf(
		oxorany("%s[%s%02u:%02u:%02u%s]%s %s[%s%s%s] %s%s%s\n"),
		oxorany("\x1b[38;5;237m"),
		oxorany("\x1b[38;5;245m"),
		time.wHour, time.wMinute, time.wSecond,
		oxorany("\x1b[38;5;237m"),
		oxorany("\x1b[38;5;250m"),
		oxorany("\x1b[38;5;237m"),
		tagColor,
		tag,
		oxorany("\x1b[38;5;237m"),
		oxorany("\x1b[38;5;250m"),
		message,
		oxorany("\x1b[0m"));
	fflush(stdout);
}

bool CLogging::ShouldLog(LogTarget target) const {
	switch (target) {
	case Both:
		return true;
	case Debug:
		return Debugging;
	case Release:
		return !Debugging;
	default:
		return false;
	}
}

void CLogging::Emit(LogLevel level, const char* message) {
	switch (level) {
	case LogLevel::Info:
		Print(oxorany("INFO"), oxorany("\x1b[38;5;74m"), message);
		return;
	case LogLevel::Ok:
		Print(oxorany("OK"), oxorany("\x1b[38;5;70m"), message);
		return;
	case LogLevel::Warn:
		Print(oxorany("WARN"), oxorany("\x1b[38;5;178m"), message);
		return;
	case LogLevel::Error:
		Print(oxorany("FAIL"), oxorany("\x1b[38;5;131m"), message);
		return;
	case LogLevel::Detail:
		Print(oxorany("DETAIL"), oxorany("\x1b[38;5;104m"), message);
		return;
	default:
		Print(oxorany("LOG"), oxorany("\x1b[38;5;250m"), message);
		return;
	}
}

int CLogging::FailAttach() {
	Error(Release, oxorany("Failed To Attach"));
	return 1;
}
