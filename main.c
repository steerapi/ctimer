#include <sys/time.h>
#include <pthread.h>
#include "pipe/pipe_util.h"
#include "nanotime/nanotime.h"
#include "timer/timer.h"

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
//	for (i = 0; i < 10; ++i) {
//		items[i] = setTimeout((uint64_t) (i + 1) * NANO, hello, &args[i]);
//	}
	for (i = 0; i < 10; ++i) {
		items[i] = setInterval((uint64_t) (i + 1) * NANO, hello, &args[i], 0);
	}
	sleep(1);
	for (i = 0; i < 10; ++i) {
		clearInterval(items[i]);
	}
	clear_pool();
	printf("DONE\n");
	for (i = 0; i < 10; ++i) {
		items[i] = setInterval((uint64_t) (i + 1) * NANO, hello, &args[i], 0);
	}
//	printf("Clear again\n");
//	for (i = 0; i < 10; ++i) {
//		sleep(1);
//		clearInterval(items[i]);
//	}
	while (1) {
		sleep(100);
	}
}
