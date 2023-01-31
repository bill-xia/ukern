#include "stdio.h"

int main()
{
	int i = 1 / 0;
	printf("%d", i);
	while (1);
	return 0;
}