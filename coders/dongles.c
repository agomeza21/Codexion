/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   dongles.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: agomez-a <agomez-a@student.42urduliz.co    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/29 14:27:08 by agomez-a          #+#    #+#             */
/*   Updated: 2026/06/16 11:43:49 by agomez-a         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "structs.h"

/*
Returns the request with the smallest priority (heap[0])
without removing it. Used to check whether a waiting coder
is next in line, and to know who to wake up in release_dongle.
Returns NULL if the heap is empty.
*/
t_request	*heap_peek(t_dongle *dongle)
{
	if (dongle->heap_size == 0)
		return (NULL);
	return (dongle->heap[0]);
}

/*
Releases a dongle: marks it as free, stores the release timestamp
needed to compute the cooldown), and signals the request at the
top of the heap (smallest priority), which is the next one to be
served according to the active scheduler (fifo or edf).
*/
void	release_dongle(t_dongle *dongle)
{
	pthread_mutex_lock(&dongle->mutex);
	dongle->in_use = 0;
	dongle->release_time = calculate_time();
	if (heap_peek(dongle) != NULL)
		pthread_cond_signal(&heap_peek(dongle)->self_cond);
	pthread_mutex_unlock(&dongle->mutex);
}

/*
Wait loop for take_dongle. The coder sleeps until ALL of these
conditions are met:
	- it's its turn (it's at the top of the heap, smallest priority)
	- the dongle is not in_use
	- dongle_cooldown has elapsed since release_time
pthread_cond_timedwait is used instead of pthread_cond_wait because
the cooldown expires purely with the passage of time, with nobody
signaling; timedwait lets the thread wake itself up once the
cooldown ends, avoiding indefinite waits.
Also exits if running becomes 0, so it doesn't block the
simulation from shutting down.
*/
static void	wait_for_dongle(t_dongle *dongle, t_coder *coder,
				t_request *my_turn)
{
	struct timespec	ts;
	long			wake_time;

	while (coder->sim->running == 1
		&& (heap_peek(dongle) != my_turn
			|| dongle->in_use == 1
			|| calculate_time() - dongle->release_time
			< coder->sim->params.dongle_cooldown))
	{
		wake_time = dongle->release_time + coder->sim->params.dongle_cooldown;
		ts.tv_sec = wake_time / 1000;
		ts.tv_nsec = (wake_time % 1000) * 1000000;
		pthread_cond_timedwait(&my_turn->self_cond, &dongle->mutex, &ts);
	}
}

/*
Tries to take a dongle. Pushes itself onto the heap with a priority
that depends on the scheduler: for fifo, priority = arrival
timestamp (earlier = smaller = first); for edf, priority = deadline
(last_compile_start + time_to_burnout, sooner = smaller = first).
Waits for its turn with wait_for_dongle, and if successful pops
itself from the heap and marks the dongle as in_use.
If running becomes 0 while waiting, removes itself from the heap
without marking the dongle as taken and returns 0.
my_turn is a LOCAL variable on this thread's stack: each call
creates its own heap entry with its own pthread_cond_t, so each
thread can be woken up individually.
*/
int	take_dongle(t_dongle *dongle, t_coder *coder)
{
	t_request		my_turn;

	pthread_cond_init(&my_turn.self_cond, NULL);
	if (coder->sim->params.scheduler == 0)
		my_turn.priority = calculate_time();
	else
		my_turn.priority = coder->last_compile_start 
			+ coder->sim->params.time_to_burnout;
	pthread_mutex_lock(&dongle->mutex);
	heap_push(dongle, &my_turn);
	wait_for_dongle(dongle, coder, &my_turn);
	if (coder->sim->running == 0)
	{
		heap_remove(dongle, &my_turn);
		pthread_mutex_unlock(&dongle->mutex);
		pthread_cond_destroy(&my_turn.self_cond);
		return (0);
	}
	heap_pop(dongle);
	dongle->in_use = 1;
	pthread_mutex_unlock(&dongle->mutex);
	pthread_cond_destroy(&my_turn.self_cond);
	return (1);
}

/*
Initializes every dongle to its starting state before any thread
runs: marked as free, with release_time=0 so the first cooldown
check always passes, and an empty heap.
Also allocates the heap array and initializes each dongle's mutex,
which protects in_use, release_time and the heap from concurrent
access by multiple coder threads..
*/
void	create_dongles(t_dongle *dongles, int n)
{
	int	i;

	i = 0;
	while (i < n)
	{
		dongles[i].in_use = 0;
		dongles[i].release_time = 0;
		dongles[i].heap = malloc(sizeof(t_request *) * n);
		dongles[i].heap_size = 0;
		pthread_mutex_init(&dongles[i].mutex, NULL);
		i++;
	}
}
