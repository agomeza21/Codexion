/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   dongles.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: agomez-a <agomez-a@student.42urduliz.co    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/29 14:27:08 by agomez-a          #+#    #+#             */
/*   Updated: 2026/06/12 10:28:30 by agomez-a         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "structs.h"

void	release_dongle(t_dongle *dongle)
{
	pthread_mutex_lock(&dongle->mutex);
	dongle->in_use = 0;
	dongle->release_time = calculate_time();
	if (dongle->queue != NULL)
		pthread_cond_signal(&dongle->queue->self_cond);
	pthread_mutex_unlock(&dongle->mutex);
}

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
