#include "windows.h"
#include "tchar.h"
#include "tlhelp32.h"

typedef struct _THREAD_PARAM {
	char moduleName[128];
	char procName[128];
	FARPROC pLoadLibrary;
	FARPROC pGetProcAddress;
} THREAD_PARAM, *PTHREAD_PARAM;

typedef int (WINAPI *PFMESSAGEBOXA) (HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType);

typedef HMODULE (WINAPI *PFLOADLIBRARYA)(LPCSTR lpLibFileName);

typedef FARPROC (WINAPI *PFGETPROCADDRESS)(HMODULE hModule, LPCSTR lpProcName);

BOOL WINAPI ThreadProc(LPVOID lParam) {   
	HMODULE hMod = NULL;
	FARPROC pFunc = NULL;
	PTHREAD_PARAM param = (PTHREAD_PARAM)lParam;
	//在目标进程装载user32.dll
	if( !(hMod = ((PFLOADLIBRARYA)param->pLoadLibrary)(param->moduleName)) )
		return FALSE;
	// 获得MessageBoxA函数的地址
	if( !(pFunc = ((PFGETPROCADDRESS)param->pGetProcAddress)(hMod, param->procName)) )
		return FALSE;
	// 调用MessageBoxA()
	((PFMESSAGEBOXA)pFunc)(NULL, param->moduleName, param->procName, MB_OK);
	return TRUE;
}

BOOL InjectCode(LPCTSTR procName){
	HANDLE hProcess = NULL;
	HANDLE hThread = NULL;
	LPVOID pRemoteBuf[2] = {0,};
	THREAD_PARAM param = {0,};
	
	//设置param内容：
	HMODULE hMod = GetModuleHandle(_T("kernel32.dll"));
	param.pLoadLibrary=GetProcAddress(hMod,"LoadLibraryA");
	param.pGetProcAddress=GetProcAddress(hMod,"GetProcAddress");
	strcpy_s(param.moduleName, "user32.dll");
	strcpy_s(param.procName, "MessageBoxA");

	// 获得目标进程的句柄：
	HANDLE hp;
	if( (hp = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0)) == INVALID_HANDLE_VALUE)
		return FALSE;
	PROCESSENTRY32 pe32 = { sizeof(pe32) };
	DWORD dwPID = 0;
	BOOL flag;
	for( flag=Process32First(hp, &pe32); flag; flag = Process32Next(hp, &pe32)){
		if (!lstrcmp(pe32.szExeFile, procName)){
			dwPID=pe32.th32ProcessID;
			break;
		}
	}
	CloseHandle(hp);
	if ( !(hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPID)) )
		return FALSE;
	//注入THREAD_PARAM：
	DWORD dwSize = sizeof(THREAD_PARAM);
	if( !(pRemoteBuf[0] = VirtualAllocEx(hProcess, NULL, dwSize, MEM_COMMIT, PAGE_READWRITE)) )
		return FALSE;
	if( !WriteProcessMemory(hProcess, pRemoteBuf[0], (LPVOID)&param, dwSize, NULL) )
		return FALSE;
	//注入ThreadProc()
	dwSize = (DWORD)InjectCode - (DWORD)ThreadProc;
	if( !(pRemoteBuf[1] = VirtualAllocEx(hProcess, NULL, dwSize, MEM_COMMIT, PAGE_EXECUTE_READWRITE)) )
		return FALSE;
	if( !WriteProcessMemory(hProcess, pRemoteBuf[1], (LPVOID)ThreadProc, dwSize, NULL) )
		return FALSE;

	if( !(hThread = CreateRemoteThread(hProcess, NULL, 0,
		(LPTHREAD_START_ROUTINE)pRemoteBuf[1], pRemoteBuf[0], 0, NULL)) )
		return FALSE;

	WaitForSingleObject(hThread, INFINITE);	
	CloseHandle(hThread);
	CloseHandle(hProcess);
	return TRUE;
}

int _tmain(int argc, LPTSTR argv[]) {
	InjectCode(_T("notepad.exe"));
	return TRUE;
}
