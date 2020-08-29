#include "windows.h"

typedef BOOL (WINAPI *PFWRITEFILE)(HANDLE hFile, LPCVOID lpBuffer, DWORD nBytesToWrite, LPDWORD lpBytesWrittern, LPOVERLAPPED lpOverlapped);

BYTE g_pOrgBytes[5] = {0,};

BOOL hook_by_code(LPCSTR szDllName, 		//包含要钩取的API的DLL名称
					LPCSTR szFuncName, 		//要钩取的API名称
					PROC pfnNew, 			//用户提供的钩取函数的地址
					PBYTE pOrgBytes) {		//存储原5字节指令的缓冲区，用于脱钩
	DWORD dwOldProtect;
	BYTE pBuf[5] = {0xE9, 0, };

	//获取要钩取的API地址
	FARPROC pfnOrg = (FARPROC)GetProcAddress(GetModuleHandleA(szDllName), szFuncName);
	PBYTE pByte = (PBYTE)pfnOrg;

	if( pByte[0] == 0xE9 ) //目标函数已经被钩取了
		return FALSE;

	//更改前5字节的读写权限
	VirtualProtect((LPVOID)pfnOrg, 5, PAGE_EXECUTE_READWRITE, &dwOldProtect);

	//保存kernel32.WriteFile的原始前5字节指令到pOrgBytes
	memcpy(pOrgBytes, pfnOrg, 5);

	//计算JMP到的相对地址，使pBuf的5字节为JMP到此相对地址
	DWORD dwAddress = (DWORD)pfnNew - (DWORD)pfnOrg - 5;
	memcpy(&pBuf[1], &dwAddress, 4);

	//用5字节JMP指令替换kernel32.WriteFile的原始起始5字节
	memcpy(pfnOrg, pBuf, 5);

	//恢复前5字节的读写权限
	VirtualProtect((LPVOID)pfnOrg, 5, dwOldProtect, &dwOldProtect);
	return TRUE;
}

BOOL unhook_by_code(LPCSTR szDllName, LPCSTR szFuncName, PBYTE pOrgBytes) {
    DWORD dwOldProtect;

    //获取要脱钩的API地址
    FARPROC pFunc = GetProcAddress(GetModuleHandleA(szDllName), szFuncName);
    PBYTE pByte = (PBYTE)pFunc;

    //如果脱钩操作针对一个首字节不是JMP指令的函数，则直接返回
    if( pByte[0] != 0xE9 )
        return FALSE;

    // 更改前5字节的读写权限
    VirtualProtect((LPVOID)pFunc, 5, PAGE_EXECUTE_READWRITE, &dwOldProtect);

    //用kernel32.WriteFile的原始前5字节替换当前函数前5字节的JMP指令
    memcpy(pFunc, pOrgBytes, 5);

    //恢复前5字节的读写权限
    VirtualProtect((LPVOID)pFunc, 5, dwOldProtect, &dwOldProtect);
    return TRUE;
}

BOOL WINAPI MyWriteFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nBytesToWrite, LPDWORD lpBytesWrittern, LPOVERLAPPED lpOverlapped){
	unhook_by_code("kernel32.dll", "WriteFile", g_pOrgBytes);

	FARPROC pFunc = GetProcAddress(GetModuleHandleA("kernel32.dll"), "WriteFile");	//获得目标API地址
	DWORD i = 0;
	LPSTR lpString=(LPSTR)lpBuffer;
	for(i = 0; i < nBytesToWrite; i++){
		if( 0x61 <= lpString[i] && lpString[i] <= 0x7A ){
			lpString[i] = 0x61 + (lpString[i]-0x60)%0x1A;
		}
	}
	BOOL status = ((PFWRITEFILE)pFunc)(hFile, lpBuffer, nBytesToWrite, lpBytesWrittern, lpOverlapped);

	hook_by_code("kernel32.dll", "WriteFile", (PROC)MyWriteFile, g_pOrgBytes);
	return status;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, 
					LPVOID lpvReserved){
    switch( fdwReason ){
        case DLL_PROCESS_ATTACH : 
        	hook_by_code("kernel32.dll", "WriteFile", (PROC)MyWriteFile, g_pOrgBytes);
        	break;
        case DLL_PROCESS_DETACH :
	        unhook_by_code("kernel32.dll", "WriteFile", g_pOrgBytes);
    	    break;
    }
    return TRUE;
}
