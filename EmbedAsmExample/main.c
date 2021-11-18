/*******************************************************************
Purpose: Image RED with C, assembly, MMX

Reference guide:

http://www.plantation-productions.com/Webster/www.artofasm.com/Windows/HTML/AoATOC.html

Note: above guide uses format <inst>(<source>, <dest>) while our compiler
requires <inst> <dest>, <source>

*******************************************************************/

#include <windows.h>
#include <mmsystem.h>

HBITMAP g_hBitmap = NULL;
BYTE* g_pBits = NULL;

LRESULT CALLBACK HelloWndProc(HWND, UINT, WPARAM, LPARAM);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	PSTR szCMLine, int iCmdShow) {
	static TCHAR szAppName[] = TEXT("HelloApplication");//name of app
	HWND	hwnd;//holds handle to the main window
	MSG		msg;//holds any message retrieved from the msg queue
	WNDCLASS wndclass;//wnd class for registration

	//defn wndclass attributes for this application
	wndclass.style = CS_HREDRAW | CS_VREDRAW;//redraw on refresh both directions
	wndclass.lpfnWndProc = HelloWndProc;//wnd proc to handle windows msgs/commands
	wndclass.cbClsExtra = 0;//class space for expansion/info carrying
	wndclass.cbWndExtra = 0;//wnd space for info carrying
	wndclass.hInstance = hInstance;//application instance handle
	wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);//set icon for window
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);//set cursor for window
	wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);//set background
	wndclass.lpszMenuName = NULL;//set menu
	wndclass.lpszClassName = szAppName;//set application name

	//register wndclass to O/S so approp. wnd msg are sent to application
	if (!RegisterClass(&wndclass)) {
		MessageBox(NULL, TEXT("This program requires Windows 95/98/NT"),
			szAppName, MB_ICONERROR);//if unable to be registered
		return 0;
	}
	//create the main window and get it's handle for future reference
	hwnd = CreateWindow(szAppName,		//window class name
		TEXT("Hello World for Windows"), // window caption
		WS_OVERLAPPEDWINDOW,	//window style
		CW_USEDEFAULT,		//initial x position
		CW_USEDEFAULT,		//initial y position
		CW_USEDEFAULT,		//initial x size
		CW_USEDEFAULT,		//initial y size
		NULL,				//parent window handle
		NULL,				//window menu handle
		hInstance,			//program instance handle
		NULL);				//creation parameters
	ShowWindow(hwnd, iCmdShow);//set window to be shown
	UpdateWindow(hwnd);//force an update so window is drawn

	//messgae loop
	while (GetMessage(&msg, NULL, 0, 0)) {//get message from queue
		TranslateMessage(&msg);//for keystroke translation
		DispatchMessage(&msg);//pass msg back to windows for processing
		//note that this is to put windows o/s in control, rather than this app
	}

	return msg.wParam;
}
void nonAsmBlur(BITMAP* bitmap, INT brighten, BYTE* temppBits) {
	INT width = bitmap->bmWidth;
	INT height = bitmap->bmHeight;
	INT bitsperPixel = bitmap->bmBitsPixel;
	INT byteWidth = bitmap->bmWidthBytes;
	INT value = 0;//temp location of pixel channel value
	INT value1 = 0;
	INT value2 = 0;
	int bytesPerPixel = bitsperPixel / 8;
	int sizeBuf = width * height * bytesPerPixel;

	// new[i] = (old[i] + old[i+bytesPerPixel] + old[i+2*bytesPerPixel]) / 3
	// example, bytesPerPixel = 3 R(1)G(1)B(1)
	// new[0] = (old[0] + old[3] + old[6]) , 
	//   - the R part of the first pixel becomes the average of the R part of the first 3 pixels.
	for (int i = 0; i < sizeBuf; i+=bytesPerPixel) {
		value = 0;
		value1 = 0;
		value2 = 0;
		for (int k = 0; k < 3*bytesPerPixel; k+=bytesPerPixel) {
			if ((i + k) >= sizeBuf) break;
			value += temppBits[i + k];	
			value1 += temppBits[i + 1 + k];
			value2 += temppBits[i + 2 + k];
		}
		g_pBits[i] = (BYTE)(value / 3);
		g_pBits[i + 1] = (BYTE)(value1 / 3);
		g_pBits[i + 2] = (BYTE)(value2 / 3);
	}
}
/**
mmx registers: mm0-mm7 - general purpose
pxor - packed xor between two registers
movd - moves a double word (32 bits) between memory and reg or reg to reg
por - packed "or" between two registers
psllq - packed shift left quad (4 DWORDS)
movq - move quad (4 DWORDS or 64 bits) between memory and reg or reg to reg
paddusb - adds unsigned bytes (8 bytes) with saturation between two reg

*/
void mmx_brighten(BITMAP* bitmap, INT brighten, BYTE* buffer) 
{
	int w = bitmap->bmWidth;
	int h = bitmap->bmHeight;
	int bitsPerPixel = bitmap->bmBitsPixel;
	BYTE B[8];
	for (int i = 0; i < 8; i++) {
		B[i] = (BYTE)brighten;
	}
	__asm {
		//save all reg values to stack
		push eax
		push ebx
		push ecx
		push edx
		push edi
		push ebp
		push esi

		//number of pixels in image (w*h) in reg eax
		mov eax, w
		mul h

		//number of bytes in image (bitsPerPixel/8*pixels)
		mov ebx, bitsPerPixel
		shr ebx, 3
		mul ebx

		//divide eax by 8 as each mmx reg holds 8 bytes
		shr eax, 3

		//store buffer in reg ebx
		mov ebx, buffer

		//clear mm2 reg
		pxor mm2, mm2

		//store brighten value to mm0
		movq mm0, B

		//brighten value needs to be in each byte of an mmx reg
		//loop and shift and load brighten value and "or"
		//until each byte in an mmx reg holds brighten value
		//use mm0 to hold value
		//note: can't use mm2 as work (calc) can only be done
		//using mmx reg. Only loading in a value can be done using
		//memory and mmx reg

		//clear ecx reg to use as counter
		xor ecx, ecx

		//start a loop
		loop1:
			
		//end loop if number of loops is greater than bytes
		//in image/8
			cmp eax, ecx
			jle loop1_end

		//load 8 bytes into mm1 
			movq mm1, [ebx]

		//add brighten value with saturation
			paddusb mm1, mm0
		//copy brighten value back to buffer
			movq [ebx], mm1
		//move the buffer pointer position by 8
		//since we are grabbing 8 bytes at once
			add ebx, 8

		//inc our counter (ecx)
			inc ecx

		//loop back to repeat
			jmp loop1

		//restore registers
		loop1_end:
			pop esi
			pop ebp
			pop edi
			pop edx
			pop ecx
			pop ebx
			pop eax
		//end mmx (emms)
			emms
	}
}

void assembly_BLUR(BITMAP* bitmap, INT brighten, BYTE* buffer) 
{
	INT width = bitmap->bmWidth;
	INT height = bitmap->bmHeight;
	INT bitsPerPixel = bitmap->bmBitsPixel;
	//REGISTERS

	//EAX, EBX, ECX, EDX are general purpose registers
	//ESI, EDI, EBP are also available as general purpose registers
	//AX, BX, CX, DX are the lower 16-bit of the above registers (think E as extended)
	//AH, AL, BH, BL, CH, CL, DH, DL are 8-bit high/low registers of the above (AX, BX, etc)
	//Typical use:
	//EAX accumulator for operands and results
	//EBX base pointer to data in the data segment
	//ECX counter for loops
	//EDX data pointer and I/O pointer
	//EBP frame pointer for stack frames
	//ESP stack pointer hardcoded into PUSH/POP operations
	//ESI source index for array operations
	//EDI destination index for array operations [e.g. copying arrays]
	//EIP instruction pointer
	//EFLAGS results flag hardcoded into conditional operations

	//SOME INSTRUCTIONS

	//MOV <source>, <destination>: mov reg, reg; mov reg, immediate; mov reg, memory; mov mem, reg; mov mem, imm
	//INC and DEC on registers or memory
	//ADD destination, source
	//SUB destination, source
	//CMP destination, source : sets the appropriate flag after performing (destination) - (source)
	//JMP label - jumps unconditionally ie. always to location marked by "label"
	//JE - jump if equal, JG/JL - jump if greater/less, JGE/JLE if greater or equal/less or equal, JNE - not equal, JZ - zero flag set
	//LOOP target: uses ECX to decrement and jump while ECX>0
	//logical instructions: AND, OR, XOR, NOT - performs bitwise logical operations. Note TEST is non-destructive AND instruction
	//SHL destination, count : shift left, SHR destination, count :shift right - carry flag (CF) and zero (Z) bits used, CL register often used if shift known
	//ROL - rotate left, ROR rotate right, RCL (rotate thru carry left), RCR (rotate thru carry right)
	//EQU - used to elimate hardcoding to create constants
	//MUL destination, source : multiplication
	//PUSH <source> - pushes source onto the stack
	//POP <destination> - pops off the stack into destination
	__asm
	{
		// save registers
		push eax
		push ebx
		push ecx
		push edx
		push edi
		push ebp
		push esi

		mov eax, width		// eax = width
		mul  height			// eax = eax * height

		//mov esp, brighten
		//xor ebx, ebx		// ebx = 0

		mov ebx, bitsPerPixel
		shr ebx, 3			// ebx = ebx >> 3 (i.e. ebx=ebx/8), bytes per pixel
		mul ebx				// eax = eax * ebx, (eax = width * height * bytesPerPixel)
		mov ebx, buffer 

		xor ecx, ecx		// ecx = 0

		start:
			cmp eax, ecx 
			jle end			// jump to label "end" if eax <= ecx

			mov edx, [ebx]	// edx = *ebx
			mov esi, edx
			mov edi, edx
			mov ebp, edx

			and edx, 255  // edx = edx & 0x000000FF
			shr esi, 8
			and esi, 255
			shr edi, 16
			and edi, 255
			shr ebp, 24
			and ebp, 255

			add edx, 10 // brighten
			add esi, 10
			add edi, 10
			add ebp, 10

			cmp edx, 255
			jle nonsaturate
			mov edx, 255 //if value is greater than 255 saturate to 255

		nonsaturate :
			cmp esi, 255
			jle nonsaturate2
			mov esi, 255

		nonsaturate2 :
			cmp edi, 255
			jle nonsaturate3
			mov edi, 255

		nonsaturate3 :
			cmp ebp, 255
			jle nonsaturate4
			mov ebp, 255

		nonsaturate4 :
			shl edi, 16
			shl esi, 8
			shl ebp, 24
			add edi, esi
			add edx, edi
			add edx, ebp
			mov [ebx], edx  // *(buffer + ecx ) = edx

			add ebx, 4		// RGBA model? 
			add ecx, 4
			jmp start

		end:
			//restore registers
			pop esi
			pop ebp
			pop edi
			pop edx
			pop ecx
			pop ebx			
			pop eax
	}
}

/**
Purpose: To handle windows messages for specific cases including when
the window is first created, refreshing (painting), and closing
the window.

Returns: Long - any error message (see Win32 API for details of possible error messages)
Notes:	 CALLBACK is defined as __stdcall which defines a calling
convention for assembly (stack parameter passing)
**/
LRESULT CALLBACK HelloWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
	PAINTSTRUCT ps;

	switch (message) {
	case WM_CREATE://additional things to do when window is created
	{
		BITMAPINFO* pbmi;
		BITMAPFILEHEADER bmfh;
		DWORD dwBytesRead, dwInfoSize;

		HANDLE hFile = CreateFile(TEXT("splash.bmp"), GENERIC_READ, FILE_SHARE_READ,
			NULL, OPEN_EXISTING, 0, NULL);

		if (hFile == INVALID_HANDLE_VALUE) {
			MessageBox(hwnd, TEXT("Cannot open file"), TEXT("WARNING"), MB_OK);
			PostQuitMessage(1);
			return NULL;
		}

		BOOL bSuccess = ReadFile(hFile, &bmfh, sizeof(BITMAPFILEHEADER),
			&dwBytesRead, NULL);

		if (!bSuccess || (dwBytesRead != sizeof(BITMAPFILEHEADER))
			|| (bmfh.bfType != *((WORD*)"BM")))
		{
			MessageBox(hwnd, TEXT("Error"), TEXT("WARNING"), MB_OK);
			PostQuitMessage(1);
			return NULL;
		}
		dwInfoSize = bmfh.bfOffBits - sizeof(BITMAPFILEHEADER);

		pbmi = (BITMAPINFO*)malloc(dwInfoSize);
		if (pbmi == NULL) {
			MessageBox(hwnd, TEXT("Low memory"), TEXT("WARNING"), MB_OK);
			PostQuitMessage(1);
			return NULL;
		}

		bSuccess = ReadFile(hFile, pbmi, dwInfoSize, &dwBytesRead, NULL);
		if (!bSuccess || (dwBytesRead != dwInfoSize))
		{
			free(pbmi);
			CloseHandle(hFile);
			PostQuitMessage(1);
			return NULL;
		}

		g_hBitmap = CreateDIBSection(NULL, 
			pbmi, DIB_RGB_COLORS, 
			(VOID**)&g_pBits,
			NULL, 0);
		
		bSuccess = ReadFile(hFile, g_pBits, bmfh.bfSize - bmfh.bfOffBits, &dwBytesRead, NULL);

		if (!bSuccess || (g_hBitmap == NULL) )
		{
			free(pbmi);
			PostQuitMessage(1);
			return NULL;
		}

		CloseHandle(hFile);
		return 0;
	}
	case WM_LBUTTONDOWN:
	{
		HDC hdc = GetDC(hwnd);
		HDC hdcMem = CreateCompatibleDC(hdc);
		BITMAP bmpInfo;
		GetObject(g_hBitmap, sizeof(BITMAP), &bmpInfo);
		int bmpSize = bmpInfo.bmWidth * bmpInfo.bmHeight * bmpInfo.bmBitsPixel/8;
		

		BYTE* tempBits = (BYTE*)malloc(bmpSize);
		if (tempBits == NULL) {
			return 0;
		}

		memcpy(tempBits, g_pBits, bmpSize);
		//assembly_BLUR(&bmpInfo, 30, tempBits);
		mmx_brighten(&bmpInfo, 30, tempBits);
		memcpy(g_pBits, tempBits, bmpSize);

		free(tempBits);

		SelectObject(hdcMem, g_hBitmap);
		BitBlt(hdc, 0, 0, bmpInfo.bmWidth, bmpInfo.bmHeight,
			hdcMem, 0, 0, SRCCOPY);

		DeleteDC(hdcMem);
		ReleaseDC(hwnd, hdc);
		return 0;
	}
	case WM_RBUTTONDOWN:
	{
		HDC hdc = GetDC(hwnd);
		HDC hdcMem = CreateCompatibleDC(hdc);
		BITMAP bmpInfo;
		GetObject(g_hBitmap, sizeof(BITMAP), &bmpInfo);
		int bmpSize = bmpInfo.bmWidth * bmpInfo.bmHeight * bmpInfo.bmBitsPixel/8;

		BYTE* tempBits = (BYTE*)malloc(bmpSize);
		if (tempBits == NULL) {
			return 0;
		}

		memcpy(tempBits, g_pBits, bmpSize);
		nonAsmBlur(&bmpInfo, 30, tempBits);
		//memcpy(g_pBits, tempBits, bmpSize);

		free(tempBits);

		SelectObject(hdcMem, g_hBitmap);
		BitBlt(hdc, 0, 0, bmpInfo.bmWidth, bmpInfo.bmHeight,
			hdcMem, 0, 0, SRCCOPY);

		DeleteDC(hdcMem);
		ReleaseDC(hwnd, hdc);
		return 0;
	}
	case WM_PAINT://what to do when a paint msg occurs
	{
		HDC hdc = BeginPaint(hwnd, &ps);//get a handle to a device context for drawing

		BITMAP bmpInfo;
		GetObject(g_hBitmap, sizeof(BITMAP), &bmpInfo);

		HDC hdcMem = CreateCompatibleDC(hdc);
		SelectObject(hdcMem, g_hBitmap);

		BitBlt(hdc, 0, 0, bmpInfo.bmWidth, bmpInfo.bmHeight,
			hdcMem, 0, 0, SRCCOPY);

		DeleteDC(hdcMem);
		EndPaint(hwnd, &ps);//release the device context
		return 0;
	}
	case WM_DESTROY://how to handle a destroy (close window app) msg
		DeleteObject(g_hBitmap);
		PostQuitMessage(0);
		return 0;
	}
	//return the message to windows for further processing
	return DefWindowProc(hwnd, message, wParam, lParam);
}
