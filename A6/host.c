#include <windows.h>
#include <mmsystem.h>
#include <CL/cl.h>
#include <stdio.h>
#include "helper.h"

HBITMAP g_hBitmap = NULL;
BYTE* g_pBits = NULL;

/** INSTRUCTIONS

You are to complete the code to brighten the splash.bmp image using OpenCL
Follow the comments to guide your solution. Much of it can be followed from
the example given in lecture however you will need to read up on how to work
with images - use the lecture notes and openCL instructions available from
Intel.

Once you get it to work, benchmark it against the C version provided

Note: you will need to download and install OpenCL for Intel (or similar based
on your computer)

Marking

4 marks for getting it to work
1 mark for benchmarking

******************************************/



/** KernelSource:bright
in: image2d_t image to brighten
in: image2d_t image returned
returns: void

You need to define a pixel as a uint4 which is a unsigned 4 int vector
to hold the pixel the work_item will brighten.
You need to define and obtain the position of the pixel as an int2.
As the position is 2D and global_id holds a specific location you need
to get the two global ids as a vector for the position
Then you can call read_imageui to obtain the pixel. You should use
non-normalized values, clamp address to edge, filter to nearest.
Now you can modify the pixel - I multiplied each channel by 0.001
Then write it back usig write_imageui.
**/
const char* KernelSource = NULL;

////////////////////////////////////////////////////////////////////////////////
/*rgbToRGBA - converts 3 byte colour to 4 byte colour */
BYTE* rgbToRGBA(BYTE* orig, int w, int h) {
	BYTE* rgba = (BYTE*)malloc(w * h * 4);
	if (rgba == NULL) return NULL;
	int k = 0;
	for (int i = 0; i < w * h * 3; ) {
		rgba[i + k] = orig[i];
		rgba[i + 1 + k] = orig[i + 1];
		rgba[i + 2 + k] = orig[i + 2];
		rgba[i + 3 + k] = 255;
		i += 3;
		k++;
	}
	return rgba;
}
/*rgbaToRGB - converts 4 byte colour to 3 byte colour */
BYTE* rgbaToRGB(BYTE* orig, int w, int h) {
	BYTE* rgb = (BYTE*)malloc(w * h * 3);
	if (rgb == NULL) return NULL;
	int k = 0;
	for (int i = 0; i < h * w * 3; ) {

		rgb[i] = orig[i + k];
		rgb[i + 1] = orig[i + 1 + k];
		rgb[i + 2] = orig[i + 2 + k];
		i += 3;
		k++;
	}
	return rgb;
}

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
void  nonAsmBlur(BITMAP* bitmap, INT brighten, BYTE* temppBits) {
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
	for (int i = 0; i < sizeBuf; i += bytesPerPixel) {
		value = 0;
		value1 = 0;
		value2 = 0;
		for (int k = 0; k < 3 * bytesPerPixel; k += bytesPerPixel) {
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
		//xor ecx, ecx

		//mov edx, brighten
		//and edx, 0x000000FF

		//pxor mm0,mm0

		//loop0:
		//	cmp ecx, 8
		//	jge loop0_end
		//	por mm0, brighten
		//	psllq mm0, 8
		//	add ecx, 1
		//	jmp loop0

		//loop0_end:
		//clear ecx reg to use as counter
		xor ecx, ecx

		//start a loop
		loop1 :

		//end loop if number of loops is greater than bytes
		//in image/8
		cmp eax, ecx
			jle loop1_end

			//load 8 bytes into mm1 
			movq mm1, [ebx]

			//add brighten value with saturation
			paddusb mm1, mm0
			//copy brighten value back to buffer
			movq[ebx], mm1
			//move the buffer pointer position by 8
			//since we are grabbing 8 bytes at once
			add ebx, 8

			//inc our counter (ecx)
			inc ecx

			//loop back to repeat
			jmp loop1

			//restore registers
			loop1_end :
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
void assembly_grey(BITMAP* bitmap, BYTE* buffer) {
	INT width = bitmap->bmWidth;
	INT height = bitmap->bmHeight;
	INT bitsPerPixel = bitmap->bmBitsPixel;
	__asm
	{
		// save all registers you will be using onto stack
		push eax
		push ebx
		push ecx
		push edx
		push esi
		push edi



		// calculate the number of pixels
		mov eax, bitsPerPixel
		shr eax, 3

		//clear ebx
		xor ebx, ebx

		// calculate the number of bytes in image (pixels*bitsperpixel)
		mul width
		mul height

		// store the address of the buffer into a register (e.g. ebx)
		mov ebx, buffer

		//setup counter register
		xor ecx, ecx

		//create a loop
		//loop while still have pixels to turn grey
		//jump out of loop if done <2 marks>
	start:
		cmp eax, ecx
			jle end

			mov edx, [ebx]
			mov esi, edx
			mov edi, edx

			and edx, 0xFF0000FF
			shl esi, 8
			and esi, 0x0000FF00
			shl edi, 16
			and edi, 0x00FF0000

			add edx, esi
			add edx, edi

			mov[ebx], edx
			//To grey a pixel our simple method will be to use
			//the red channel for all 3 channels (RGB) e.g
			//RGB = 124, 26, 200 => RGB = 124, 124, 124
			//Note: LSB = R, MSB is B as in BGR in memory
			//There are 3 bytes per pixel for our example
			//A reg loads in 4 bytes (or 2 or 1)
			//If you use the full size reg then you will need to
			//create a 4 byte value consisting of RRR and the next
			//byte. Then move the pointer 3 bytes and repeat 
			//DO NOT LOAD IN SINGLE BYTES TO WORK ON, MUST
			//LOAD IN FULL 4 BYTES INTO REGISTER <6 marks>

			//increment counter and pointer as necessary
			add ecx, 3
			add ebx, 3
			jmp start
			//loop back up

			//code works as expected <2 marks>
		end:
		//restore registers to original values before leaving
		//function

		pop edi
			pop esi
			pop edx
			pop ecx
			pop ebx
			pop eax

	}

}

void assembly_BLUR(BITMAP* bitmap, INT brighten, BYTE* buffer)
{
	INT width = bitmap->bmWidth;
	INT height = bitmap->bmHeight;
	INT bitsPerPixel = bitmap->bmBitsPixel;

	// static variable saved in data section (not stack), 
	// so no EBP needed to reference it. 
	static int staticBrighten = 0;

	staticBrighten = brighten;

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

	/****************************************************************/
	//EBP frame pointer for stack frames
	// 
	// C local variables are referenced via EBP, so local variables CANNOT 
	// be referenced after 
	// EBP was modified by assembly.
	//
	/*****************************************************************/

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

		mov ebx, bitsPerPixel
		shr ebx, 3			// ebx = ebx >> 3 (i.e. ebx=ebx/8), bytes per pixel
		mul ebx				// eax = eax * ebx, (eax = width * height * bytesPerPixel)
		mov ebx, buffer

		xor ecx, ecx		// ecx = 0

		start :

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


			add edx, staticBrighten // brighten
			add esi, staticBrighten
			add edi, staticBrighten
			add ebp, staticBrighten

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
			mov[ebx], edx  // *(buffer + ecx ) = edx

			add ebx, 4		// RGBA model? 
			add ecx, 4
			jmp start

			end :
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

/*
openclbrigthen:
YOUR CODE MOSTLY GOES HERE (KernelSource is defined at top
*/
BYTE* openclbrigthen(int width, int height, BYTE* image)
{
	cl_device_id device_id;             // compute device id 
	cl_context context;                 // compute context
	cl_command_queue commands;          // compute command queue
	cl_program program;                 // compute program
	cl_kernel kernel;                   // compute kernel

	cl_mem inputImage;                       // device memory used for the input array
	cl_mem outputImage;                      // device memory used for the output array
	int err;                            // error code returned from api calls
	unsigned int	bright = 5;

	//BYTE* results = (BYTE*)malloc(width * height * 4);
	//if (results == NULL) { 
	//	MsgBoxF(TEXT("low memory"));
	//	return NULL; 
	//}

	// get total number of available platforms:
	cl_uint numPlatforms;
	cl_int errNum;
	errNum = clGetPlatformIDs(0, NULL, &numPlatforms);

	//    // get IDs for all platforms:
	cl_platform_id* platformIds;
	platformIds = (cl_platform_id*)malloc(sizeof(cl_platform_id) * numPlatforms);
	if (platformIds == NULL) {
		MsgBoxF(TEXT("low memory"));
		return NULL;
	}
	errNum = clGetPlatformIDs(numPlatforms, platformIds, NULL);

	size_t size;
	char* name = NULL;
	int i = 0;
	for (i = 0; i < numPlatforms; i++) {
		errNum = clGetPlatformInfo(platformIds[0], CL_PLATFORM_NAME, 0, NULL, &size);
		name = (char*)malloc(sizeof(char) * size);
		errNum = clGetPlatformInfo(platformIds[0], CL_PLATFORM_NAME, size, name, NULL);
		//MsgBoxFA("%d: platform name: %s", i, name );
	}

	// Get total number of devices belonging to platformId[0],
	// Then use the first device found.
	cl_uint numDevices = 0;
	errNum = clGetDeviceIDs(platformIds[0],
		CL_DEVICE_TYPE_GPU,
		0,
		NULL,
		&numDevices);
	if (numDevices == 0) {
		MsgBoxF("No this type of OpenCL device found");
		return NULL;
	}
	errNum = clGetDeviceIDs(platformIds[0], CL_DEVICE_TYPE_GPU,
		1,
		&device_id,
		NULL);

	cl_uint maxComputeUnits = 0;
	errNum = clGetDeviceInfo(device_id, CL_DEVICE_MAX_COMPUTE_UNITS,
		sizeof(cl_uint), &maxComputeUnits, &size);
	//MsgBoxFA("max computer units: %d", maxComputeUnits);


	// Create a compute context 
	// This context only has one device.
	context = clCreateContext(NULL, 1, &device_id,
		NULL, NULL, &errNum);



	// Create a command queue
	//
	commands = clCreateCommandQueue(context, device_id,
		NULL, &errNum);

	// Create the compute program from the source buffer
	//
	size_t source_size;
	FILE* fp = fopen("main.cl", "rb");
	if (fp == NULL) {
		MsgBoxF(TEXT("Failed to load kernel source: main.cl"));
		return NULL;
	}
	fseek(fp, 0L, SEEK_END);
	source_size = ftell(fp);

	char* source_str = (char*)malloc(source_size);
	if (source_str == NULL) {
		MsgBoxF(TEXT("low mem"));
		return NULL;
	}
	fseek(fp, 0L, SEEK_SET);
	size_t n = fread(source_str, 1, source_size, fp);
	if (n != source_size) {
		MsgBoxF(TEXT("read error"));
		return NULL;
	}

	fclose(fp);

	program = clCreateProgramWithSource(context, 1,
		&source_str, (const size_t*)&source_size, &errNum);


	free(source_str);
	source_str = NULL;

	// Build the program executable
	// here just build it againt one device
	errNum = clBuildProgram(program, 1, &device_id, NULL, NULL, NULL);

	// Create the compute kernel in the program we wish to run
	//Note: the kernel name is the function name in source file.
	kernel = clCreateKernel(program, "bright", &errNum);


	// Create the input and output in device memory for our calculation
	//
	// We use cl_image_format structure that describes format of the created image
	//Our image is not normalized and uses 8 bit "integer" values
	//Colour format is RGBA 
	cl_image_format format;

	// choose sRGBA or RGBA image format
	format.image_channel_order = CL_RGBA;
	format.image_channel_data_type = CL_UNSIGNED_INT8;

	// structure that describes format of created image 


	// image description structure
	// initialized by description for existing inputRGBA image object
	//use cl_image_desc

	// initialize mem_object and image_type

	inputImage = clCreateImage2D(context,
		CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
		&format,
		width,
		height,
		0,
		image,
		&errNum
	);

	outputImage = clCreateImage2D(context,
		CL_MEM_WRITE_ONLY,
		&format,
		width,
		height,
		0,
		NULL,
		&errNum);

	// Set the arguments to our compute kernel
	//

	errNum = clSetKernelArg(kernel, 0, sizeof(cl_mem), &inputImage);
	errNum = clSetKernelArg(kernel, 1, sizeof(cl_mem), &outputImage);

	//// Get the maximum work group size for executing the kernel on the device
	////
	size_t workGrpSize[2] = { 16, 16 };
	size_t globalWorkSize[2] = { width, height };
	/*errNum = clGetKernelWorkGroupInfo(kernel, device_id,
		CL_KERNEL_WORK_GROUP_SIZE, sizeof(workGrpSize), &workGrpSize, NULL);
	if (errNum != CL_SUCCESS)
	{
		MsgBoxF(TEXT("Error: Failed to retrieve kernel work group info! %d\n"), errNum);
		return NULL;
	}*/
	
	//define global size by width and height


	//enqueue the kernel
	errNum = clEnqueueNDRangeKernel(commands, kernel,
		sizeof(workGrpSize)/sizeof(workGrpSize[0]),
		NULL,
		&globalWorkSize, 
		&workGrpSize,
		0, NULL, NULL);

	// Wait for the command commands to get serviced before reading back results
	// Note: not necessary unless command queue is with CL_QUEUE_OUT_OF_ORDER_EXEC_MODE.
	//errNum = clFinish(commands);

	// Read back the results from the device to verify the output
	//
	size_t originst[3] = { 0, 0, 0 };						//offset to start read of image data
	size_t regionst[3] = { width, height, 1 };			//region (size) to read as a vector. depth=1 for 2D images
	errNum = clEnqueueReadImage(commands, outputImage, 
		CL_TRUE,
		originst, regionst, 0, 0, image,
		0, NULL, NULL);

	// Shutdown and cleanup
	//
	clReleaseMemObject(inputImage);
	clReleaseMemObject(outputImage);
	clReleaseProgram(program);
	clReleaseKernel(kernel);
	clReleaseCommandQueue(commands);
	clReleaseContext(context);

	return image;
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

		if (!bSuccess || (g_hBitmap == NULL))
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
		int bmpSize = bmpInfo.bmWidth * bmpInfo.bmHeight * bmpInfo.bmBitsPixel / 8;


		BYTE* tempBits = (BYTE*)malloc(bmpSize);
		if (tempBits == NULL) {
			return 0;
		}
		memcpy(tempBits, g_pBits, bmpSize);

		///////////////////////////////////////////////////////////////////////////////////
		//assembly_BLUR(&bmpInfo, 30, tempBits);	
		//mmx_brighten(&bmpInfo, 30, tempBits);
		//assembly_grey(&bmpInfo, tempBits);
		// OpenCL brighten
		BYTE* temp2 = rgbToRGBA(tempBits, bmpInfo.bmWidth, bmpInfo.bmHeight);
		if (temp2 == NULL) {
			MsgBoxF(TEXT("low mem"));
			exit(1);
		}

		LONGLONG start = GetTicks();
		openclbrigthen(bmpInfo.bmWidth, bmpInfo.bmHeight, temp2);
		SetWindowTextF(hwnd, TEXT("%f ms"), 1000*DurSeconds(start));

		BYTE* resultBits = rgbaToRGB(temp2, bmpInfo.bmWidth, bmpInfo.bmHeight);
		free(temp2);

		if (resultBits == NULL) {
			MsgBoxF(TEXT("low mem"));
			exit(1);
		}
		memcpy(tempBits, resultBits, bmpSize);
		free(resultBits);

		///////////////////////////////////////////////////////////////////////////////////

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
		int bmpSize = bmpInfo.bmWidth * bmpInfo.bmHeight * bmpInfo.bmBitsPixel / 8;

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
