#include "windows.h"
#include "stdio.h"

LPVOID pWriteFile = NULL;
HANDLE hProcess;
HANDLE hThread;
BYTE INT3 = 0xCC;
BYTE OriginalByte = 0;

BOOL OnCreateProcessDebugEvent(LPDEBUG_EVENT pde){
	// 获得WriteFile()的函数地址
	pWriteFile = GetProcAddress(GetModuleHandleA("kernel32.dll"), "WriteFile");

	hProcess = ((CREATE_PROCESS_DEBUG_INFO)pde->u.CreateProcessInfo).hProcess;
	hThread = ((CREATE_PROCESS_DEBUG_INFO)pde->u.CreateProcessInfo).hThread;

	// 将WriteFile()的第一字节保存到OriginalByte中，将0xCC写入WriteFile()第一字节
	ReadProcessMemory(hProcess, pWriteFile, &OriginalByte, sizeof(BYTE), NULL);
	WriteProcessMemory(hProcess, pWriteFile, &INT3, sizeof(BYTE), NULL);
	return TRUE;
}

BOOL OnExceptionDebugEvent(LPDEBUG_EVENT pde){
	CONTEXT ctx;
	DWORD nBytesToWrite, dataAddr, i;
	PEXCEPTION_RECORD per = &pde->u.Exception.ExceptionRecord;

	if( EXCEPTION_BREAKPOINT == per->ExceptionCode ){ //产生的异常事件是断点异常INT3
		if( pWriteFile == per->ExceptionAddress ){ //断点地址是WriteFile()的起始地址
			//把WriteFile()起始的0xCC改回原始内容
			WriteProcessMemory(hProcess, pWriteFile, &OriginalByte, sizeof(BYTE), NULL);

			//获得线程上下文
			ctx.ContextFlags = CONTEXT_CONTROL;
			GetThreadContext(hThread, &ctx);

			//获取WriteFile()的第2,3参数,即希望写入的数据的起始地址和字节数
			ReadProcessMemory(hProcess, (LPVOID)(ctx.Esp+0x8), &dataAddr, sizeof(DWORD), NULL);
			ReadProcessMemory(hProcess, (LPVOID)(ctx.Esp+0xC), &nBytesToWrite, sizeof(DWORD), NULL);

			//开辟一块临时空间用于修改要写入文件的内容
			PBYTE buf = (PBYTE)malloc(nBytesToWrite+1);
			memset(buf, 0, nBytesToWrite+1);

			//将第二个参数所指向的要写入的数据内容读取到临时空间,用于修改
			ReadProcessMemory(hProcess, (LPVOID)dataAddr, buf, nBytesToWrite, NULL);

			for( i = 0; i < nBytesToWrite; i++ ){ //修改要写入的内容
				if( 0x61 <= buf[i] && buf[i] <= 0x7A )
					buf[i] = 0x61 + (buf[i] - 0x61 + 0x1) % 0x1A;
			}

			//把修改后的临时空间数据写回到第二个参数所指向的数据地址
			WriteProcessMemory(hProcess, (LPVOID)dataAddr, buf, nBytesToWrite, NULL);

			free(buf); //释放临时空间

			//将线程上下文的EIP设为WriteFile()的起始地址
			ctx.Eip = (DWORD)pWriteFile;
			SetThreadContext(hThread, &ctx);

			//触发被调试进程的执行(被调试进程从所设的ctx.Eip位置即WriteFile()起始地址处运行)
			ContinueDebugEvent(pde->dwProcessId, pde->dwThreadId, DBG_CONTINUE);
			Sleep(0);

			// 再次钩取WriteFile()
			WriteProcessMemory(hProcess, pWriteFile, &INT3, sizeof(BYTE), NULL);
			return TRUE;
		}
	}
	return FALSE;
}

int main(int argc, char* argv[]){
	DEBUG_EVENT de;

	if( argc != 2 )
		return 1;
	DWORD dwPID = atoi(argv[1]);
	if( !DebugActiveProcess(dwPID) )
		return 1;

	// 阻塞并等待被调试者发送调试事件
	while( WaitForDebugEvent(&de, INFINITE) ) {
		if( CREATE_PROCESS_DEBUG_EVENT == de.dwDebugEventCode ) {
			OnCreateProcessDebugEvent(&de); //当被调试进程创建或被关联到调试器时执行
		}
		else if( EXCEPTION_DEBUG_EVENT == de.dwDebugEventCode ) {
			if( OnExceptionDebugEvent(&de) ) //当异常事件发生时执行
				continue;
		}
		else if( EXIT_PROCESS_DEBUG_EVENT == de.dwDebugEventCode ) { //被调试进程终止
			break; //调试器随之终止
		}
		//ContinueDebugEvent(de.dwProcessId, de.dwThreadId, DBG_CONTINUE); //再次运行被调试者
		ContinueDebugEvent(de.dwProcessId, de.dwThreadId, DBG_CONTINUE); //再次运行被调试者
	}
	return 0;
}