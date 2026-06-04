/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: agomez-a <agomez-a@student.42urduliz.co    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/03 15:54:21 by agomez-a          #+#    #+#             */
/*   Updated: 2026/06/04 13:11:56 by agomez-a         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "structs.h"

void	dequeue(t_dongle *dongle, t_request *my_turn)
{
	t_request	*current;

	if (dongle->queue == my_turn)
		dongle->queue = my_turn->next;
	else
	{
		current = dongle->queue;
		while (current->next && current->next != my_turn)
			current = current->next;
		if (current->next == my_turn)
			current->next = my_turn->next;
	}
}

void	enqueue_edf(t_dongle *dongle, t_request *my_turn)
{
	t_request		*current;

	if (dongle->queue == NULL)
		dongle->queue = my_turn;
	else if (my_turn->deadline < dongle->queue->deadline)
	{
		my_turn->next = dongle->queue;
		dongle->queue = my_turn;
	}
	else
	{
		current = dongle->queue;
		while (current->next && my_turn->deadline >= current->next->deadline)
			current = current->next;
		my_turn->next = current->next;
		current->next = my_turn;
	}
}

long	calculate_time(void)
{
	struct timeval	tv;

	gettimeofday(&tv, NULL);
	return ((long)(tv.tv_sec * 1000 + tv.tv_usec / 1000));
}

void	log_action(t_sim *sim, int id, char *action)
{
	pthread_mutex_lock(&sim->log_mutex);
	printf("%ld %d %s\n", calculate_time() - sim->start_time, id, action);
	pthread_mutex_unlock(&sim->log_mutex);
}
