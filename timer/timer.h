/*
 * timer.h
 *
 *  Created on: Feb 17, 2012
 *      Author: Au
 */

#ifndef TIMER_H_
#define TIMER_H_

#include <sys/time.h>
#include "../nanotime/nanotime.h"
#include <pthread.h>
#include "../pipe/pipe_util.h"
#include <signal.h>

#define MIN(X,Y) (X < Y ? X : Y)

typedef void (*action)(void*);

typedef struct TimeItem {
	pipe_producer_t *in;
	pipe_consumer_t *out;
	struct TimeItem* next;
	struct nanotime interval;
	action callback;
	void *args;
	pthread_mutex_t lock;
	pthread_cond_t conv;
	pthread_cond_t canceledv;
	int times;
	int canceled;
} TimeItem;

#define NANO 1000000000

static void handle_interval(const void* elem_in, size_t count,
		pipe_producer_t* elem_out, void* aux) {
	TimeItem *ti = ((TimeItem*) aux);
	if (elem_in == NULL) {
//		printf("rm\n");
		free(ti);
		return;
	}
	//TODO: Watch for race condition
	pthread_mutex_lock(&ti->lock);
	if (ti->canceled) {
//		printf("canceled\n");
		pthread_cond_signal(&ti->canceledv);
		pthread_mutex_unlock(&ti->lock);
		return;
	}
	pthread_mutex_unlock(&ti->lock);

	uint64_t deadline = *(uint64_t*) elem_in;
	struct nanotime nt = nanotime_now();
	int64_t timeleft = deadline - nt.ns;
	if (timeleft < 0) {
		//callback
		if (ti->callback)
			ti->callback(ti->args);
		if (ti->times > 0) {
			ti->times--;
			if (ti->times == 0) {
				//TODO: Watch for race condition
				clearInterval(ti);
			}
		}
		//update deadline
		deadline = nanotime_now().ns + ti->interval.ns;
	}
	//queue up for next round
	pipe_push(ti->in, &deadline, 1);
	timeleft = deadline - nanotime_now().ns;

	if (timeleft > 0) {
		//sleep
		struct timespec ts;
		ts.tv_sec = timeleft / NANO;
		ts.tv_nsec = timeleft % NANO;
		pthread_mutex_lock(&ti->lock);
		pthread_cond_timedwait_relative_np(&ti->conv, &ti->lock, &ts);
		pthread_mutex_unlock(&ti->lock);
//		nanosleep(&ts, 0);
	}
	return;
}

//Watch out for race condition when use in multiple threads
TimeItem free_pipe;
int free_count;
int total_count;

void clear_pool() {
	TimeItem * torem = free_pipe.next;
	while (torem != NULL) {
		free_pipe.next = free_pipe.next->next;
		free_count--;
		pipe_producer_free(torem->in);
		pipe_consumer_free(torem->out);
		torem = free_pipe.next;
	}
}

void clearInterval(TimeItem *ti) {
	if (ti == NULL)
		return;
	//TODO: Watch for race condition
	pthread_mutex_lock(&ti->lock);
	ti->canceled = 1;
	pthread_cond_signal(&ti->conv);
	pthread_cond_wait(&ti->canceledv, &ti->lock);
	pthread_mutex_unlock(&ti->lock);

//	if (free_count > 2*total_count/3) {
//		pipe_producer_free(ti->in);
//		pipe_consumer_free(ti->out);
//		total_count--;
//	} else {
	TimeItem *next = free_pipe.next;
	free_pipe.next = ti;
	ti->next = next;
	free_count++;
//	}
//	printf("%d %d\n", free_count, total_count);
//	pipe_producer_free(ti->pi->in);
//	pipe_consumer_free(ti->pi->out);
}

TimeItem *setInterval(uint64_t nsec, action callback, void* args, int times) {
	TimeItem *ti;
	if (free_pipe.next == NULL) {
		ti = malloc(sizeof(TimeItem));
		total_count++;
		int elms_size = sizeof(uint64_t);
		pipe_t* pipe1 = pipe_new(elms_size, 0);
		pipe_t* pipe2 = pipe_new(elms_size, 0);
		ti->in = pipe_producer_new(pipe1);
		ti->out = pipe_consumer_new(pipe2);
		pipe_connect(pipe_consumer_new(pipe1), handle_interval, ti,
				pipe_producer_new(pipe2));
		pipe_free(pipe1);
		pipe_free(pipe2);
		pthread_mutex_init(&ti->lock, NULL);
		pthread_cond_init(&ti->conv, NULL);
		pthread_cond_init(&ti->canceledv, NULL);

	} else {
		ti = free_pipe.next;
		free_pipe.next = free_pipe.next->next;
		free_count--;
	}
	ti->callback = callback;
	ti->args = args;
	ti->times = times;
	ti->interval.ns = nsec;
	ti->canceled = 0;
	struct nanotime nt = nanotime_now();
	uint64_t deadline = nt.ns + nsec;
	pipe_push(ti->in, &deadline, 1);
	return ti;
}

TimeItem *setTimeout(uint64_t nsec, action callback, void* args) {
	return setInterval(nsec, callback, args, 1);
}

#endif /* TIMER_H_ */
