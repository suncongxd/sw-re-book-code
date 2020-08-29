.386
.model flat, stdcall

include kernel32.inc
includelib kernel32.lib

include msvcrt.inc
includelib msvcrt.lib

.data
format		db	"%d",0AH,0

.code

main PROC

; initialize EAX=3
MOV EAX,3

; The following instruction sequence is in the slides:
XOR EBX, EAX
XOR EAX, EBX
XOR EBX, EAX
INC EAX
NEG EBX
ADD EBX, 0A6098326H
CMP EAX, ESP
MOV EAX, 59F67CD5H
XOR EAX, 0FFFFFFFFH
SUB EBX, EAX
RCL EAX, CL
PUSH 0F9CBE47AH
ADD DWORD PTR [ESP], 6341B86H
SBB EAX, EBP
SUB DWORD PTR [ESP], EBX
PUSHFD
PUSHAD
POP EAX
ADD ESP, 20H
TEST EBX, EAX
POP EAX
;;;;;;;;;;;;;;;;;;

; print EAX=EAX+4=7
INVOKE crt_printf, addr format, EAX

INVOKE crt_getchar
INVOKE ExitProcess, 0
main ENDP

END main