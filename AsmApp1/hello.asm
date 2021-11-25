INCLUDELIB kernel32.lib
INCLUDELIB user32.lib

.386								; instruction set
.model flat,stdcall					; memory model and function call spec


; Windows API prototype declaration

MessageBox equ MessageBoxA
MessageBoxA PROTO,
	hwnd:DWORD,
	lpCaption:DWORD,
	lpText:DWORD,
	uType:DWORD

ExitProcess PROTO,
	dwExitCode:DWORD


.data								; data section (RW)

.const								; read only data section (RO)
g_szHello db 'Hello World', 0
g_szTitle db 'The Title', 0

.data?								; uninitialized data section (RW)

.code								; code section
start:
	push 0					; MessageBox()'s 1st param: parent window handle
	push offset g_szTitle	; MessageBox()'s 2nd param: window title
	push offset g_szHello	; MessageBox()'s 3rd param: window content
	push 0					; MessageBox()'s 4th param: MB_OK
	call MessageBox			; call MessageBox()

END start							; denote the ENTRY point of this app