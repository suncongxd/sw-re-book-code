#include "windows.h"
#include "tchar.h"

BOOL SetPrivilege(LPCTSTR lpszPrivilege, BOOL bEnablePrivilege){
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

BOOL InjectDll(DWORD dwPID, LPCTSTR szDllPath){
	HANDLE hProcess = NULL, hThread = NULL;
	HMODULE hMod = NULL;
	LPVOID pRemoteBuf = NULL;
	DWORD dwBufSize = (DWORD)(lstrlen(szDllPath) + 1) * sizeof(TCHAR);
	LPTHREAD_START_ROUTINE pThreadProc;

	// 获得dwPID进程ID对应的目标进程句柄
	if ( !(hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPID)) )
		return FALSE;

	// 在目标进程地址空间中为DLL路径名szDllPath开辟一块存储空间，将szDllPath路径字符串写入该空间
	pRemoteBuf = VirtualAllocEx(hProcess, NULL, dwBufSize, MEM_COMMIT, PAGE_READWRITE);
	WriteProcessMemory(hProcess, pRemoteBuf, (LPVOID)szDllPath, dwBufSize, NULL);

	// 获取当前进程地址空间中LoadLibraryW()函数的地址，该函数由kernel32.dll导入
	hMod = GetModuleHandle(L"kernel32.dll");
	pThreadProc = (LPTHREAD_START_ROUTINE)GetProcAddress(hMod, "LoadLibraryW");

	// 在目标进程中运行线程，该线程执行LoadLibraryW()函数并传入被注入DLL路径作为参数
	hThread = CreateRemoteThread(hProcess, NULL, 0, pThreadProc, pRemoteBuf, 0, NULL);
	WaitForSingleObject(hThread, INFINITE);

	CloseHandle(hThread);
	CloseHandle(hProcess);

	return TRUE;
}

int _tmain(int argc, TCHAR *argv[]){
	if( argc != 3) {
		_tprintf(L"USAGE : %s <pid> <dll_path>\n", argv[0]);
		return 1;
	}
	if( !SetPrivilege(SE_DEBUG_NAME, TRUE) )
		return 1;

	// inject dll
	if( InjectDll((DWORD)_tstol(argv[1]), argv[2]) )
		_tprintf(L"InjectDll(\"%s\") success.\n", argv[2]);
	else
		_tprintf(L"InjectDll(\"%s\") failed.\n", argv[2]);

	return 0;
}