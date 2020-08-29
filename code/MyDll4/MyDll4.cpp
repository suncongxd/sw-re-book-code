#include "windows.h"

typedef BOOL (WINAPI *PFWRITEFILE)(HANDLE hFile, LPCVOID lpBuffer, DWORD nBytesToWrite, LPDWORD lpBytesWrittern, LPOVERLAPPED lpOverlapped);

FARPROC g_pOrgFunc = NULL;

BOOL WINAPI MyWriteFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nBytesToWrite, LPDWORD lpBytesWrittern, LPOVERLAPPED lpOverlapped) {
	DWORD i = 0;
	LPSTR lpString=(LPSTR)lpBuffer;
	for(i = 0; i < nBytesToWrite; i++){
		if( 0x61 <= lpString[i] && lpString[i] <= 0x7A ){
			lpString[i] = 0x61 + (lpString[i]-0x60)%0x1A;
		}
	}
	return ((PFWRITEFILE)g_pOrgFunc)(hFile, lpBuffer, nBytesToWrite, lpBytesWrittern, lpOverlapped);
}

BOOL hook_iat(LPCSTR szDllName, PROC pfnOrg, PROC pfnNew) {
	LPCSTR szLibName;
	PIMAGE_THUNK_DATA pThunk;
	DWORD dwOldProtect;

	//pAddr保存当前PE文件的ImageBase地址
	HMODULE hMod = GetModuleHandle(NULL);
	PBYTE pAddr = (PBYTE)hMod;

	//pAddr指向NT头起始(偏移量从DOS头的最后4字节读出)
	pAddr += *((DWORD*)&pAddr[0x3C]);

	//dwRVA获得DataDirectory[1]的第一个4字节内容, 即Import Table的RVA
	DWORD dwRVA = *((DWORD*)&pAddr[0x80]);

	//pImportDesc指向IMAGE_IMPORT_DESCRIPTOR数组的起始
	PIMAGE_IMPORT_DESCRIPTOR pImportDesc = (PIMAGE_IMPORT_DESCRIPTOR)((DWORD)hMod+dwRVA);

	//遍历IMAGE_IMPORT_DESCRIPTOR数组
	for( ; pImportDesc->Name; pImportDesc++ ){
		szLibName = (LPCSTR)((DWORD)hMod + pImportDesc->Name);
		if( !_stricmp(szLibName, szDllName) ) { 
			//找到参数szDllName对应的IAT，由pThunk指向
			pThunk = (PIMAGE_THUNK_DATA)((DWORD)hMod + pImportDesc->FirstThunk);

			for( ; pThunk->u1.Function; pThunk++ ){
				//在IAT中找到pfnOrg对应的地址项
				if( pThunk->u1.Function == (DWORD)pfnOrg ){
					// 更改内存属性，便于修改IAT
					VirtualProtect((LPVOID)&pThunk->u1.Function, 4, PAGE_EXECUTE_READWRITE, &dwOldProtect);
					//修改IAT中pfnOrg函数地址为pfnNew函数地址
                    pThunk->u1.Function = (DWORD)pfnNew;
					// 将内存属性改回
                    VirtualProtect((LPVOID)&pThunk->u1.Function, 4,	dwOldProtect, &dwOldProtect);
					return TRUE;
				}
			}
		}
	}
	return FALSE;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
	switch( fdwReason ){
		case DLL_PROCESS_ATTACH : 
            //被钩取的目标API的地址
           	g_pOrgFunc = GetProcAddress(GetModuleHandle(L"kernel32.dll"),"WriteFile");
            // 钩取，kernel32.WriteFile()被MyDll4.MyWriteFile()钩取
			hook_iat("kernel32.dll", g_pOrgFunc, (PROC)MyWriteFile);
			break;
		case DLL_PROCESS_DETACH :
            //脱钩，用kernel32.WriteFile()的原地址恢复IAT
            hook_iat("kernel32.dll", (PROC)MyWriteFile, g_pOrgFunc);
			break;
	}
	return TRUE;
}
