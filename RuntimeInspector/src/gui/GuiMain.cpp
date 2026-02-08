#include "MainWindow.h"
#include "../core/SystemInfo.h"
#include <Windows.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	using namespace RuntimeInspector::GUI;
	using namespace RuntimeInspector::Core;

	if (!DetectSystemInfo()) {
		MessageBoxA(nullptr, "Failed to detect Windows NT version", "Error", MB_OK | MB_ICONERROR);
	}

	int epResult = ElevateToSystemPrivileges();
	if (epResult != 0) {
		MessageBoxA(nullptr, "Warning: Failed to elevate privileges. Some operations may fail.", "Warning", MB_OK | MB_ICONWARNING);
	}

	MainWindow window(hInstance);
	if (!window.Initialize()) {
		MessageBoxA(nullptr, "Failed to initialize GUI", "Error", MB_OK | MB_ICONERROR);
		return 1;
	}

	return window.Run();
}
