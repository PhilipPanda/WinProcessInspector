#include "../InjectionEngine.h"

namespace WinProcessInspector {
namespace Injection {

bool InjectViaSetWindowsHookEx(DWORD processId, LPCSTR dllPath) {
	HMODULE hModDll = LoadLibraryA(dllPath);
	if (!hModDll) {
		return false;
	}

	HOOKPROC procAddress = (HOOKPROC)GetProcAddress(hModDll, "HookProcedure");
	if (!procAddress) {
		FreeLibrary(hModDll);
		return false;
	}

	HANDLE hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
	if (hThreadSnap == INVALID_HANDLE_VALUE) {
		FreeLibrary(hModDll);
		return false;
	}

	THREADENTRY32 te32 = {};
	te32.dwSize = sizeof(THREADENTRY32);

	if (!Thread32First(hThreadSnap, &te32)) {
		CloseHandle(hThreadSnap);
		FreeLibrary(hModDll);
		return false;
	}

	DWORD threadId = 0;
	do {
		if (te32.th32OwnerProcessID == processId) {
			threadId = te32.th32ThreadID;
			break;
		}
	} while (Thread32Next(hThreadSnap, &te32));

	CloseHandle(hThreadSnap);

	if (threadId == 0) {
		FreeLibrary(hModDll);
		return false;
	}

	HHOOK hookHandle = SetWindowsHookExA(WH_GETMESSAGE, procAddress, hModDll, threadId);
	if (!hookHandle) {
		FreeLibrary(hModDll);
		return false;
	}

	PostThreadMessageA(threadId, WM_NULL, 0, 0);
	Sleep(100);
	
	UnhookWindowsHookEx(hookHandle);
	
	return true;
}

}
}
