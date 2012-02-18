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

typedef struct {
	pipe_producer_t* in;
	pipe_consumer_t* out;
	struct nanotime interval;
	action callback;
	void *args;
	pthread_mutex_t lock;
	int times;
	int canceled;
} TimeItem;

#define NANO 1000000000

static void handle_interval(const void* elem_in, size_t count,
		pipe_producer_t* elem_out, void* aux) {
	TimeItem *ti = ((TimeItem*) aux);
	if (elem_in == NULL) {
		free(ti);
		return;
	}
	if (ti->canceled) {
		return;
	}
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
//				pthread_mutex_destroy(&ti->lock);
				pipe_producer_free(ti->in);
				pipe_consumer_free(ti->out);
//				free(ti);
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
		nanosleep(&ts, 0);
	}
	return;
}

void clearInterval(TimeItem *ti) {
	if (ti == NULL)
		return;
	ti->canceled = 1;
//	pthread_mutex_destroy(&ti->lock);
	pipe_producer_free(ti->in);
	pipe_consumer_free(ti->out);
//	free(ti);
}

TimeItem *setInterval(uint64_t nsec, action callback, void* args, int times) {
	TimeItem *ti = malloc(sizeof(TimeItem));
//	pthread_mutex_init(&ti->lock, NULL);
	ti->callback = callback;
	ti->args = args;
	ti->times = times;
	ti->interval.ns = nsec;
	struct nanotime nt = nanotime_now();
	uint64_t deadline = nt.ns + nsec;

	int elms_size = sizeof(uint64_t);
	pipe_t* pipe1 = pipe_new(elms_size, 0);
	pipe_t* pipe2 = pipe_new(elms_size, 0);
	ti->in = pipe_producer_new(pipe1);
	ti->out = pipe_consumer_new(pipe2);
	pipe_connect(pipe_consumer_new(pipe1), handle_interval, ti,
			pipe_producer_new(pipe2));
	pipe_free(pipe1);
	pipe_free(pipe2);

	pipe_push(ti->in, &deadline, 1);
	return ti;
}

TimeItem *setTimeout(uint64_t nsec, action callback, void* args) {
	return setInterval(nsec, callback, args, 1);
}

#endif /* TIMER_H_ */
