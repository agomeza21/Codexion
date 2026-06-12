/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   dongles.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: agomez-a <agomez-a@student.42urduliz.co    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/29 14:27:08 by agomez-a          #+#    #+#             */
/*   Updated: 2026/06/12 12:46:57 by agomez-a         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "structs.h"

/*
Releases a dongle: marks it as free, stores the release timestamp
needed to compute the cooldown), and signals the first request
in the queue, which is the next one with priority according to
the scheduler (fifo or edf).
*/
void	release_dongle(t_dongle *dongle)
{
	pthread_mutex_lock(&dongle->mutex);
	dongle->in_use = 0;
	dongle->release_time = calculate_time();
	if (dongle->queue != NULL)
		pthread_cond_signal(&dongle->queue->self_cond);
	pthread_mutex_unlock(&dongle->mutex);
}

/*
Wait loop for take_dongle. The coder sleeps until ALL of these
conditions are met:
	- it's its turn (it's at the front of the queue)
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
		&& (dongle->queue != my_turn
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
Tries to take a dongle. Enqueues itself with its computed deadline
(last_compile_start + time_to_burnout, only used when scheduler is
edf), waits for its turn with wait_for_dongle, and if successful
marks the dongle as in_use.
If running becomes 0 while waiting, it dequeues itself without
marking the dongle as taken and returns 0 -- this way func() knows
this dongle doesn't need to be released.
my_turn is a LOCAL variable on this thread's stack: each call
creates its own queue entry with its own pthread_cond_t, so each
thread can be woken up individually.
*/
int	take_dongle(t_dongle *dongle, t_coder *coder)
{
	t_request		my_turn;

	my_turn.timestamp = calculate_time();
	my_turn.next = NULL;
	pthread_cond_init(&my_turn.self_cond, NULL);
	pthread_mutex_lock(&dongle->mutex);
	my_turn.deadline = coder->last_compile_start
		+ coder->sim->params.time_to_burnout;
	enqueue(dongle, &my_turn, coder->sim->params.scheduler);
	wait_for_dongle(dongle, coder, &my_turn);
	if (coder->sim->running == 0)
	{
		dequeue(dongle, &my_turn);
		pthread_mutex_unlock(&dongle->mutex);
		pthread_cond_destroy(&my_turn.self_cond);
		return (0);
	}
	dequeue(dongle, &my_turn);
	dongle->in_use = 1;
	pthread_mutex_unlock(&dongle->mutex);
	pthread_cond_destroy(&my_turn.self_cond);
	return (1);
}

/*
Initializes every dongle to its starting state before any thread
runs: marked as free, with release_time=0 so the first cooldown
check always passes, and an empty waiting queue.
Also initializes each dongle's mutex, which protects its state
(in_use, release_time, queue) from concurrent access by multiple
coder threads.
*/
void	create_dongles(t_dongle *dongles, int n)
{
	int	i;

	i = 0;
	while (i < n)
	{
		dongles[i].in_use = 0;
		dongles[i].release_time = 0;
		pthread_mutex_init(&dongles[i].mutex, NULL);
		dongles[i].queue = NULL;
		i++;
	}
}
