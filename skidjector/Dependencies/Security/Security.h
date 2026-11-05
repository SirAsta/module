#pragma once

#include <Windows.h>

#include <atomic>
#include <filesystem>
#include <memory>
#include <thread>

class CSecurity {
public:
	~CSecurity();

	bool IsLocalProcessDebugged();
	bool IsRemoteProcessDebugged(HANDLE process);
	bool IsDebuggerWindowOpen();
	bool DetectDebugger(HANDLE remoteProcess = nullptr);
	bool CheckHash(const std::filesystem::path& filePath);

	void StartMonitor(HANDLE remoteProcess = nullptr);
	void SetMonitorTarget(HANDLE remoteProcess);
	void StopMonitor();
	bool WasDebuggerDetected() const;

private:
	void MonitorLoop();

	std::atomic<bool> m_MonitorRunning{ false };
	std::atomic<bool> m_DebuggerDetected{ false };
	std::atomic<uintptr_t> m_MonitorTarget{ 0 };
	std::thread m_MonitorThread;
};

inline auto Security = std::make_unique<CSecurity>();
