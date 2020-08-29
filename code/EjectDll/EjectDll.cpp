#include "windows.h"
#include "tlhelp32.h"
#include "tchar.h"

//由进程名找到进程id号
DWORD FindProcessID(LPCTSTR szProcessName) {
	DWORD dwPID = 0xFFFFFFFF;
	PROCESSENTRY32 pe = {sizeof(PROCESSENTRY32),};

	HANDLE hSnapShot = CreateToolhelp32Snapshot( TH32CS_SNAPALL, NULL ); // 获得系统进程的快照
	Process32First(hSnapShot, &pe);
	do {
		if(!lstrcmpi(szProcessName, (LPCTSTR)pe.szExeFile)) {
			dwPID = pe.th32ProcessID;
			break;
		}
	} while(Process32Next(hSnapShot, &pe));

	CloseHandle(hSnapShot);
	return dwPID;
}

BOOL SetPrivilege(LPCTSTR lpszPrivilege, BOOL bEnablePrivilege) {
	TOKEN_PRIVILEGES tp;
	HANDLE hToken;
	LUID luid;

	if( !OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken) )
	return FALSE;

	if( !LookupPrivilegeValue(NULL, lpszPrivilege, &luid) )
		return FALSE; 

	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	if( bEnablePrivilege )
		tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	else
		tp.Privileges[0].Attributes = 0;

	if( !AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), (PTOKEN_PRIVILEGES) NULL, (PDWORD) NULL) )
		return FALSE; 

	if( GetLastError() == ERROR_NOT_ALL_ASSIGNED )
		return FALSE;

	return TRUE;
}

BOOL EjectDll(DWORD dwPID, LPCTSTR szDllName) {
	HANDLE hSnapshot, hProcess;
	MODULEENTRY32 me = {sizeof(me),};

	// 使用TH32CS_SNAPMODULE，获得装载到notepad进程地址空间的DLL信息, dwPID为notepad进程的id号
	hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, dwPID);

	BOOL bFound = FALSE;
	BOOL bMore = Module32First(hSnapshot, &me);
	for( ; bMore ; bMore = Module32Next(hSnapshot, &me) ){
		if( !lstrcmpi((LPCTSTR)me.szModule, szDllName) || 
			!lstrcmpi((LPCTSTR)me.szExePath, szDllName) ){
			bFound = TRUE;
			break;
		}
	}
	if( !bFound ){
		CloseHandle(hSnapshot);
		return FALSE;
	}
	if ( !(hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPID)) )
		return FALSE;

	HMODULE hModule = GetModuleHandle(_T("kernel32.dll"));
	LPTHREAD_START_ROUTINE pThreadProc = (LPTHREAD_START_ROUTINE)GetProcAddress(hModule, "FreeLibrary");
	HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, pThreadProc, me.modBaseAddr, 0, NULL);

	WaitForSingleObject(hThread, INFINITE);	
	CloseHandle(hThread);
	CloseHandle(hProcess);
	CloseHandle(hSnapshot);
	return TRUE;
}

int _tmain(int argc, TCHAR* argv[]) {
	DWORD dwPID = 0xFFFFFFFF;
	dwPID = FindProcessID(_T("notepad.exe"));
	if( dwPID == 0xFFFFFFFF ) //没有找到notepad进程
		return FALSE;
	if( !SetPrivilege(SE_DEBUG_NAME, TRUE) ) // 更改特权
		return FALSE;
	if( EjectDll(dwPID, _T("MyDll.dll")) )  // 卸载DLL
		_tprintf(_T("EjectDll(%d, \"%s\") success.\n"), dwPID, _T("MyDll.dll"));
	else
		_tprintf(_T("EjectDll(%d, \"%s\") failed.\n"), dwPID, _T("MyDll.dll"));
	return TRUE;
}
