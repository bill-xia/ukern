#include "stdio.h"
#include "usyscall.h"

int a[10] = {0,1,2,3,4,5,6,7,8,9};

int main()
{
	for (int i = 0; i < 10; ++i) {
		for (int j = i + 1; j < 10; ++j) {
			if (a[i] < a[j]) {
				int tmp = a[i];
				a[i] = a[j];
				a[j] = tmp;
			}
		}
	}
	for (int i = 0; i < 10; i += 2) {
		printf("%d %d ", 1ll * a[i], 1ll * a[i + 1]);
	}
	printf("\n");
	sys_exit(0);
}