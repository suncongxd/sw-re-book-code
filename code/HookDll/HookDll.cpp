#include "windows.h"
#include "tchar.h"

HINSTANCE g_hInstance = NULL;
HHOOK g_hHook = NULL;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD dwReason, LPVOID lpvReserved){
	switch( dwReason ){
        case DLL_PROCESS_ATTACH:
			g_hInstance = hinstDLL;
			break;
	}
	return TRUE;
}

LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam){
	TCHAR szPath[MAX_PATH] = {0,};
	TCHAR *p = NULL;

	if( nCode >= 0 ) {
		if( !(lParam & 0x80000000) ){ //lParam的第31位（0：按键；1：释放键）
			GetModuleFileName(NULL, szPath, MAX_PATH);
			p = _tcsrchr(szPath, _T('\\'));
            //若装载当前DLL的进程为notepad.exe，则消息不会传递给下一个钩子
			if( !lstrcmpi(p + 1, _T("notepad.exe")) )
				return 1;
		}
	}
    // 当前进程不是notepad.exe，将消息传递给下一个钩子
	return CallNextHookEx(g_hHook, nCode, wParam, lParam);
}

#ifdef __cplusplus
extern "C" {
#endif
	__declspec(dllexport) void HkStart() {
		g_hHook = SetWindowsHookEx(WH_KEYBOARD, KeyboardProc, g_hInstance, 0);
	}

	__declspec(dllexport) void HkStop() {
		if( g_hHook ) {
			UnhookWindowsHookEx(g_hHook);
			g_hHook = NULL;
		}
	}
#ifdef __cplusplus
}
#endif
