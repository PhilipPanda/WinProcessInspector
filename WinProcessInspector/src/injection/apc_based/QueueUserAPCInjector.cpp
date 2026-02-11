#include "../InjectionEngine.h"

namespace WinProcessInspector {
namespace Injection {

bool InjectViaQueueUserAPC(LPCSTR DllPath, HANDLE hProcess, DWORD processId) {
	HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
	if (!hKernel32) {
		return false;
	}
	LPVOID LoadLibAddr = GetProcAddress(hKernel32, "LoadLibraryA");

	if (!LoadLibAddr) {
		return false;
	}

	SIZE_T dllPathSize = strlen(DllPath) + 1;
	LPVOID pDllPath = VirtualAllocEx(hProcess, 0, dllPathSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

	if (!pDllPath) {
		return false;
	}

	SIZE_T bytesWritten = 0;
	BOOL Written = WriteProcessMemory(hProcess, pDllPath, LPVOID(DllPath), dllPathSize, &bytesWritten);

	if (!Written || bytesWritten != dllPathSize) {
		VirtualFreeEx(hProcess, pDllPath, 0, MEM_RELEASE);
		return false;
	}

	HANDLE hThreadSnap = INVALID_HANDLE_VALUE;
	THREADENTRY32 te32;

	hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);

	if (hThreadSnap == INVALID_HANDLE_VALUE) {
		VirtualFreeEx(hProcess, pDllPath, 0, MEM_RELEASE);
		return false;
	}

	te32.dwSize = sizeof(THREADENTRY32);

	if (!Thread32First(hThreadSnap, &te32)) {
		CloseHandle(hThreadSnap);
		VirtualFreeEx(hProcess, pDllPath, 0, MEM_RELEASE);
		return false;
	}

	DWORD threadId = 0;
	do {
		if (te32.th32OwnerProcessID == processId) {
			threadId = te32.th32ThreadID;
			break;
		}
	} while (Thread32Next(hThreadSnap, &te32));

	if (threadId == 0) {
		CloseHandle(hThreadSnap);
		VirtualFreeEx(hProcess, pDllPath, 0, MEM_RELEASE);
		return false;
	}

	HANDLE hThread = OpenThread(THREAD_SET_CONTEXT, FALSE, threadId);
	if (!hThread) {
		CloseHandle(hThreadSnap);
		VirtualFreeEx(hProcess, pDllPath, 0, MEM_RELEASE);
		return false;
	}

	DWORD dwResult = QueueUserAPC((PAPCFUNC)LoadLibAddr, hThread, (ULONG_PTR)pDllPath);
	
	CloseHandle(hThread);
	CloseHandle(hThreadSnap);
	
	if (dwResult == 0) {
		VirtualFreeEx(hProcess, pDllPath, 0, MEM_RELEASE);
		return false;
	}
	
	Sleep(100);
	VirtualFreeEx(hProcess, pDllPath, 0, MEM_RELEASE);
	
	return true;
}

}
}
