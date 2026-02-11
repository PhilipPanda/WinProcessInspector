#include "../InjectionEngine.h"

namespace WinProcessInspector {
namespace Injection {

bool InjectViaNtCreateThreadEx(LPCSTR DllPath, HANDLE hProcess) {
	if (!DllPath || !hProcess) {
		return false;
	}

	HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
	if (!hKernel32) {
		return false;
	}
	
	LPVOID LoadLibraryAddr = GetProcAddress(hKernel32, "LoadLibraryA");
	if (!LoadLibraryAddr) {
		return false;
	}

	SIZE_T dllPathSize = strlen(DllPath) + 1;
	LPVOID pDllPath = VirtualAllocEx(hProcess, nullptr, dllPathSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (!pDllPath) {
		return false;
	}

	SIZE_T bytesWritten = 0;
	BOOL written = WriteProcessMemory(hProcess, pDllPath, DllPath, dllPathSize, &bytesWritten);
	if (!written || bytesWritten != dllPathSize) {
		VirtualFreeEx(hProcess, pDllPath, 0, MEM_RELEASE);
		return false;
	}

	HMODULE modNtDll = GetModuleHandleA("ntdll.dll");
	if (!modNtDll) {
		VirtualFreeEx(hProcess, pDllPath, 0, MEM_RELEASE);
		return false;
	}

	lpNtCreateThreadEx funNtCreateThreadEx = reinterpret_cast<lpNtCreateThreadEx>(GetProcAddress(modNtDll, "NtCreateThreadEx"));
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
		LoadLibraryAddr,
		pDllPath,
		0,
		0,
		0,
		0,
		nullptr
	);

	if (!hThread || status < 0) {
		VirtualFreeEx(hProcess, pDllPath, 0, MEM_RELEASE);
		return false;
	}

	DWORD waitResult = WaitForSingleObject(hThread, 5000);
	
	DWORD exitCode = 0;
	GetExitCodeThread(hThread, &exitCode);
	
	CloseHandle(hThread);
	
	if (waitResult == WAIT_TIMEOUT) {
		VirtualFreeEx(hProcess, pDllPath, 0, MEM_RELEASE);
		return false;
	}
	
	Sleep(100);
	VirtualFreeEx(hProcess, pDllPath, 0, MEM_RELEASE);

	return exitCode != 0 && exitCode != STILL_ACTIVE;
}

}
}
