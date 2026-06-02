/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   dongles.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: agomez-a <agomez-a@student.42urduliz.co    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/29 14:27:08 by agomez-a          #+#    #+#             */
/*   Updated: 2026/06/02 18:39:24 by agomez-a         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "structs.h"

void    release_dongle(t_dongle *dongle)
{
	pthread_mutex_lock(&dongle->mutex);
	dongle->in_use = 0;
	dongle->release_time = calculate_time();
	if (dongle->queue != NULL)
		pthread_cond_signal(&dongle->queue->self_cond);
	pthread_mutex_unlock(&dongle->mutex);
}

void    take_dongle(t_dongle *dongle, t_coder *coder)
{
	t_request	my_turn;
	t_request	*current;

	my_turn.timestamp = calculate_time();
	my_turn.next = NULL;
	pthread_cond_init(&my_turn.self_cond, NULL);
	pthread_mutex_lock(&dongle->mutex);
	if (dongle->queue == NULL)
		dongle->queue = &my_turn;
	else
	{
		current = dongle->queue;
		while (current->next)
			current = current->next;
		current->next = &my_turn;
	}
	while (dongle->queue != &my_turn || dongle->in_use == 1 || calculate_time() - dongle->release_time < coder->sim->params.dongle_cooldown)
		pthread_cond_wait(&my_turn.self_cond, &dongle->mutex);
	dongle->queue = my_turn.next;
	dongle->in_use = 1;
	pthread_mutex_unlock(&dongle->mutex);
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
