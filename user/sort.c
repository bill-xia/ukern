#include "stdio.h"

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
    for (int i = 0; i < 10; ++i) {
        printf("%d ", a[i]);
    }
    printf("\n");
    sys_exit(0);
}