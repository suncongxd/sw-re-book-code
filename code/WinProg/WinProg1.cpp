#include "windows.h"
#include "tchar.h"

#define BUFSIZE 128
 
int _tmain(int argc, LPTSTR argv[]){
	TCHAR str1[BUFSIZE] = _TEXT("str");
	LPCTSTR str2 = _T("str2");
	LPCTSTR str3 = _T("STR");

	_tprintf(_T("%d\n"), lstrcmp(str1, str3));
	_tprintf(_T("%d\n"), lstrcmpi(str1, str3));

	lstrcat(str1, str2);
	_tprintf(_T("%s\n"), str1);

	lstrcpy(str1, str2);
	_tprintf(_T("%s\n"), str1);

	_tprintf(_T("%d\n"), lstrlen(str1));

	wsprintf(str1, _T("%s;%s\n"), str2, str3);
	_tprintf(_T("%s"),str1);

	while(TRUE);
	return TRUE;
}