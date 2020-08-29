#include "windows.h"
#include "tchar.h"
#include "stdio.h"
 
int _tmain(int argc, LPTSTR argv[]){
	char szA[100];
	WCHAR szW[100];
	
	sprintf_s(szA, "%S", L"Unicode Str");//将Unicode字符串转化为ANSI字符串
	printf("%s\n", szA);

	swprintf_s(szW, L"%S", "ANSI Str");	//将ANSI字符串转化为Unicode字符串
	wprintf(L"%s\n",szW);

	while(TRUE);
	return TRUE;
}