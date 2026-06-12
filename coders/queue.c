/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   queue.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: agomez-a <agomez-a@student.42urduliz.co    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/12 10:28:07 by agomez-a          #+#    #+#             */
/*   Updated: 2026/06/12 10:30:12 by agomez-a         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "structs.h"

void	enqueue(t_dongle *dongle, t_request *my_turn, int scheduler)
{
	t_request		*current;

	if (scheduler == 0)
	{
		if (dongle->queue == NULL)
			dongle->queue = my_turn;
		else
		{
			current = dongle->queue;
			while (current->next)
				current = current->next;
			current->next = my_turn;
		}
	}
	else
	{
		enqueue_edf(dongle, my_turn);
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
