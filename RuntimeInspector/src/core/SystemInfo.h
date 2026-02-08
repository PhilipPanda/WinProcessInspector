#pragma once

#include <Windows.h>

namespace RuntimeInspector {
namespace Core {

	bool DetectSystemInfo();
	bool SetProcessPrivilege(HANDLE hToken, LPCTSTR Privilege, BOOL bEnablePrivilege);
	void DisplaySystemError(LPCSTR szAPI);
	int ElevateToSystemPrivileges();

}
}
