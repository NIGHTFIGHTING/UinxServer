#include <stdio.h>
#include <stdlib.h>

long func(int n);


int main(int argc, char *argv[]) {
	printf("Entering main ...\n");
	int i;
	long result = 0;
	for(i=1; i<=100; i++) {
		result +=i;
	}
	printf("result[1-100]=%ld\n", result);
	printf("result[1-10]=%ld\n", func(10));

	printf("Exiting main ...\n");
	return 0;
}

long func(int n) {
	long sum = 0;
	int i;
	for(i=1; i<=n; i++) {
		sum += i;
	}
	return sum;
}
