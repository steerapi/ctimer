/*
 * timer.h
 *
 *  Created on: Feb 17, 2012
 *      Author: Au
 */

#ifndef TIMER_H_
#define TIMER_H_

#include <sys/time.h>
#include "nanotime.h"
#include <pthread.h>
#include "pipe/pipe_util.h"
#include <signal.h>

#define MIN(X,Y) (X < Y ? X : Y)

typedef void (*action)(void*);

typedef struct {
	pipeline_t pl;
	struct nanotime interval;
	action callback;
	void *args;
	pthread_mutex_t lock;
	int times;
} TimeItem;

#define NANO 1000000000

static void *handle_interval(const void* elem_in, size_t count,
		pipe_producer_t* elem_out, void* aux) {
	TimeItem *ti = ((TimeItem*) aux);
	uint64_t deadline = *(uint64_t*) elem_in;
	if (deadline == 0) {
		pthread_mutex_lock(&ti->lock);
		if (ti->callback)
			ti->callback(ti->args);
		pthread_mutex_unlock(&ti->lock);
		pthread_mutex_destroy(&ti->lock);
		pipe_producer_free(ti->pl.in);
		pipe_consumer_free(ti->pl.out);
		free(ti);
		pthread_cancel(pthread_self());
		return NULL;
	}

	//If pass deadline
	struct nanotime nt = nanotime_now();
	int64_t timeleft = deadline - nt.ns;
	if (timeleft < 0) {
		//callback
		pthread_mutex_lock(&ti->lock);
		if (ti->callback)
			ti->callback(ti->args);
		pthread_mutex_unlock(&ti->lock);
		if (ti->times > 0) {
			ti->times--;
			if (ti->times == 0) {
				pthread_mutex_destroy(&ti->lock);
				pipe_producer_free(ti->pl.in);
				pipe_consumer_free(ti->pl.out);
				free(ti);
				pthread_cancel(pthread_self());
			}
		}
		//update deadline
		deadline = nanotime_now().ns + ti->interval.ns;
	}
	//queue up for next round
	pipe_push(ti->pl.in, &deadline, 1);
	//calculate the min ns
	//sleep
	struct timespec ts;
	ts.tv_sec = timeleft / NANO;
	ts.tv_nsec = timeleft % NANO;
	nanosleep(&ts, 0);
	return NULL;
}

void clearInterval(TimeItem *ti, action callback, void* args) {
	if (ti == NULL)
		return;
	pthread_mutex_lock(&ti->lock);
	ti->callback = callback;
	ti->args = args;
	pthread_mutex_unlock(&ti->lock);
	uint64_t deadline = 0;
	pipe_push(ti->pl.in, &deadline, 1);
}

TimeItem *setInterval(uint64_t nsec, action callback, void* args, int times) {
	TimeItem *ti = malloc(sizeof(TimeItem));
	pthread_mutex_init(&ti->lock, NULL);
	ti->callback = callback;
	ti->args = args;
	ti->times = times;
	ti->interval.ns = nsec;
	struct nanotime nt = nanotime_now();
	uint64_t deadline = nt.ns + nsec;
	pipeline_t pl = pipe_pipeline(sizeof(uint64_t), handle_interval, ti,
			sizeof(uint64_t), (void*) 0);
	ti->pl = pl;
	pipe_push(pl.in, &deadline, 1);
	return ti;
}

TimeItem *setTimeout(uint64_t nsec, action callback, void* args) {
	return setInterval(nsec, callback, args, 1);
}

#endif /* TIMER_H_ */
