#include "stdio.h"
#include "usyscall.h"

const int cos_taylor[4] = {1, 0, -1, 0};
const int sin_taylor[4] = {0, 1, 0, -1};

double cos1(double x) {
	double x_k = 1, cosx = 0;
	for (int i = 0; i < 100; ++i) {
		if (i)
			x_k /= i;
		cosx += cos_taylor[i % 4] * x_k;
		x_k *= x;
	}
	return cosx;
}

double sin1(double x) {
	double x_k = 1, sinx = 0;
	for (int i = 0; i < 100; ++i) {
		if (i)
			x_k /= i;
		sinx += sin_taylor[i % 4] * x_k;
		x_k *= x;
	}
	return sinx;
}

double fabs(double a)
{
	if (a >= 0)
		return a;
	return -a;
}

int main()
{
	// Test whether sin^2(x)+cos^2(x)==1 holds for many times,
	// thus testing whether float point registers are stored properly.
	// Exercise: draw the fork tree
	int ch = sys_fork();
	if (ch != 0)
		sys_wait(NULL);
	ch = sys_fork();
	if (ch != 0)
		sys_wait(NULL);
	ch = sys_fork();
	if (ch != 0)
		sys_wait(NULL);
	ch = sys_fork();
	if (ch != 0)
		sys_wait(NULL);
	ch = sys_fork();
	if (ch != 0)
		sys_wait(NULL);
	ch = sys_fork();
	double pi = 3.1415926535;
	for (int k = 0; k < 100; ++k) {
		for (int i = 0; i < 72; ++i) {
			double x = pi * i / 36;
			double s1 = sin1(x), c1 = cos1(x);
			if (fabs(s1 * s1 + c1 * c1 - 1) > 0.00000001) {
				printf("float() failed: sin^2(pi/%d) + cos^2(pi/%d) = %lf\n", i, i, s1 * s1 + c1 * c1);
			}
		}
	}
	printf("Pass!\n");
	for (int k = 0; k < 100; ++k) {
		for (int i = 0; i < 72; ++i) {
			double x = pi * i / 36;
			double s1 = sin1(x), c1 = cos1(x);
			if (fabs(s1 * s1 + c1 * c1 - 1) > 0.00000001) {
				printf("float() failed: sin^2(pi/%d) + cos^2(pi/%d) = %lf\n", i, i, s1 * s1 + c1 * c1);
			}
		}
	}
	printf("Pass!\n");
	sys_exit(0);
}