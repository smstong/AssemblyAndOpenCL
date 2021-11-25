#include <stdio.h>

int func_1(int x, int y)
{
	return x + y;
}
int __stdcall func_2(int x, int y)
{
	return x + y;
}

int main(int argc, char** argv)
{
	printf("hello world!\n");
	func_1(1, 2);
	func_2(1, 2);
	return 0;
}

