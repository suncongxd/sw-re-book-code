#include "stdio.h"
#include "windows.h"
#include "tchar.h"

typedef void (*PFN_HOOKSTART)();
typedef void (*PFN_HOOKSTOP)();

int _tmain(int argc, TCHAR* argv[]) {
	HMODULE	hDll = NULL;
	if( (hDll = LoadLibraryA("HookDll.dll")) == NULL )// 装载HookDll.dll
		return FALSE;

	// 获取导出函数HkStart()和HkStop()的地址
	PFN_HOOKSTART HookStart = (PFN_HOOKSTART)GetProcAddress(hDll, "HkStart");
	PFN_HOOKSTOP HookStop = (PFN_HOOKSTOP)GetProcAddress(hDll, "HkStop");

	HookStart(); //开始钩取键盘消息
	_tprintf(_T("press 'q' to quit!\n"));
	while( getchar() != 'q' ); // 等到用户输入'q'才终止钩取
	HookStop(); //终止钩取键盘消息
	FreeLibrary(hDll); //卸载HookDll.dll
	return TRUE;
}
