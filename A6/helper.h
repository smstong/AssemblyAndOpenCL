#include <tchar.h>

int MsgBoxF(const TCHAR* fmt, ...);
int MsgBoxFA(const char* fmt, ...);

int SetWindowTextF(HWND hwnd, const TCHAR* fmt, ...);

LONGLONG GetTicks();
double DurSeconds(LONGLONG start);