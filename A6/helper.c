#include <tchar.h>
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdarg.h>
#include <windows.h>

int MsgBoxF(const TCHAR* fmt, ...)
{
	TCHAR msg[1024];
	va_list argList;
	va_start(argList, fmt);
	_vstprintf(msg, sizeof(msg) / sizeof(msg[0]), fmt, argList);
	va_end(argList);

	return MessageBox(NULL, msg, TEXT("MessageBoxF"), MB_OK);
}

int MsgBoxFA(const char* fmt, ...)
{
	char msg[1024];
	va_list argList;
	va_start(argList, fmt);
	vsprintf(msg,  fmt, argList);
	va_end(argList);

	return MessageBoxA(NULL, msg, "MessageBoxF", MB_OK);
}

int SetWindowTextF(HWND hwnd, const TCHAR* fmt, ...)
{
	TCHAR msg[1024];
	va_list argList;
	va_start(argList, fmt);
	_vstprintf(msg, sizeof(msg) / sizeof(msg[0]), fmt, argList);
	va_end(argList);

	return SetWindowText(hwnd, msg);
}

LONGLONG GetTicks() 
{
	LARGE_INTEGER ticks;
	QueryPerformanceCounter(&ticks);
	return ticks.QuadPart;
}

double DurSeconds(LONGLONG start)
{
	LARGE_INTEGER ticksNow, ticksPerSecond;
	LONGLONG durTicks;
	double durSeconds;
	QueryPerformanceCounter(&ticksNow);
	QueryPerformanceFrequency(&ticksPerSecond);

	durTicks = ticksNow.QuadPart - start;
	durSeconds = (double)durTicks / ticksPerSecond.QuadPart;

	return durSeconds;
}