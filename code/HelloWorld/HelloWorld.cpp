#include "windows.h"
#include "tchar.h"

int _tmain(int argc, TCHAR *argv[]){
	MessageBox(NULL,
			_T("Hello World!"),
			_T("www.xidian.edu.cn"),
			MB_OK);
	return 0;
}
