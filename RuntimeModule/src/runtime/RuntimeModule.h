#pragma once

#include <stdio.h>
#include <Windows.h>

namespace RuntimeInspector {
namespace Runtime {

	// Logging functionality
	void WriteLogMessage(const char* message);

	// Hook procedure for SetWindowsHookEx injection
	extern "C" __declspec(dllexport) void HookProcedure();

} // namespace Runtime
} // namespace RuntimeInspector

// Native API function pointer definitions
typedef NTSTATUS (WINAPI* pNtTerminateProcess)(
	IN	NTSTATUS	ExitStatus
);
