#include "windows.h"
#include "tchar.h"

#pragma comment(lib, "urlmon.lib")

DWORD WINAPI ThreadProc(LPVOID lParam){
	TCHAR szPath[MAX_PATH] = {0,};
	if( !GetModuleFileName( NULL, szPath, MAX_PATH ) )
		return FALSE;
	TCHAR *p = _tcsrchr( szPath, _T('\\') );
	if( !p )
		return FALSE;
	lstrcpy(p+1, _T("index.html"));
	URLDownloadToFile(NULL, _T("https://www.xidian.edu.cn") , szPath, 0, NULL);
	return TRUE;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
	switch( fdwReason ){
	case DLL_PROCESS_ATTACH:
		HANDLE hThread = CreateThread(NULL, 0, ThreadProc, NULL, 0, NULL);
		CloseHandle(hThread);
		break;
	}
	return TRUE;
}