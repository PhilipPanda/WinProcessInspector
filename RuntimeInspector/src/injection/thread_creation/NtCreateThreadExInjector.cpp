#include "../InjectionEngine.h"

namespace RuntimeInspector {
namespace Injection {

bool InjectViaNtCreateThreadEx(LPCSTR DllPath, HANDLE hProcess) {
	LPVOID LoadLibraryAddr = (LPVOID)GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA");

	if (!LoadLibraryAddr) {
		return false;
	}

	LPVOID pDllPath = VirtualAllocEx(hProcess, 0, strlen(DllPath), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

	if (!pDllPath) {
		return false;
	}

	BOOL Written = WriteProcessMemory(hProcess, pDllPath, (LPVOID)DllPath, strlen(DllPath), NULL);

	if (!Written) {
		VirtualFreeEx(hProcess, pDllPath, 0, MEM_RELEASE);
		return false;
	}

	HMODULE modNtDll = GetModuleHandle("ntdll.dll");

	if (!modNtDll) {
		VirtualFreeEx(hProcess, pDllPath, 0, MEM_RELEASE);
		return false;
	}

	lpNtCreateThreadEx funNtCreateThreadEx = (lpNtCreateThreadEx)GetProcAddress(modNtDll, "NtCreateThreadEx");

	if (!funNtCreateThreadEx) {
		VirtualFreeEx(hProcess, pDllPath, 0, MEM_RELEASE);
		return false;
	}

#ifdef DEBUG_NTBUFFER
	NtCreateThreadExBuffer ntBuffer;
	memset(&ntBuffer, 0, sizeof(NtCreateThreadExBuffer));
	ULONG temp0[2];
	ULONG temp1;

	ntBuffer.Size = sizeof(NtCreateThreadExBuffer);
	ntBuffer.Unknown1 = 0x10003;
	ntBuffer.Unknown2 = sizeof(temp0);
	ntBuffer.Unknown3 = temp0;
	ntBuffer.Unknown4 = 0;
	ntBuffer.Unknown5 = 0x10004;
	ntBuffer.Unknown6 = sizeof(temp1);
	ntBuffer.Unknown7 = &temp1;
	ntBuffer.Unknown8 = 0;
#endif

	HANDLE hThread = nullptr;

	NTSTATUS status = funNtCreateThreadEx(
		&hThread,
		THREAD_ALL_ACCESS,
		nullptr,
		hProcess,
		(LPTHREAD_START_ROUTINE)LoadLibraryAddr,
		pDllPath,
		NULL,
		0,
		0,
		0,
		nullptr
	);

	if (!hThread) {
		VirtualFreeEx(hProcess, pDllPath, 0, MEM_RELEASE);
		return false;
	}

	WaitForSingleObject(hThread, INFINITE);

	VirtualFreeEx(hProcess, pDllPath, 0, MEM_RELEASE);
	CloseHandle(hThread);

	return true;
}

}
}
