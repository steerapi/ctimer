#include <sys/time.h>
#include "nanotime.h"
#include <pthread.h>
#include "pipe/pipe_util.h"
#include "timer.h"

void hello(void* args) {
	int a = *(int*) args;
	printf("Hello ");
	int i;
	for (i = 0; i < a; ++i) {
		printf(" ");
	}
	printf("%d\n", a);
}

int main() {
	int args[10] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
	TimeItem* items[10];
	int i;
	for (i = 0; i < 10; ++i) {
		items[i] = setTimeout((uint64_t) (i + 1) * NANO, hello, &args[i]);
	}
//	for (i = 0; i < 10; ++i) {
//		sleep(5);
//		clearInterval(items[i], 0, 0);
//	}
	while (1) {
		sleep(100);
	}
}
