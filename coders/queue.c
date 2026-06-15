/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   queue.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: agomez-a <agomez-a@student.42urduliz.co    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/12 10:28:07 by agomez-a          #+#    #+#             */
/*   Updated: 2026/06/15 23:26:08 by agomez-a         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "structs.h"

/*
Inserts request into the dongle's min-heap, keeping the heap
property: every parent has a priority <= its children's.
The new element is placed at the end and "bubbled up"
(sift-up): while it's smaller than its parent, they swap.
This guarantees heap[0] is always the smallest priority.
*/
void	heap_push(t_dongle *dongle, t_request *req)
{
	int			i;
	int			parent;
	t_request	*tmp;

	dongle->heap[dongle->heap_size] = req;
	i = dongle->heap_size;
	dongle->heap_size++;
	while (i > 0)
	{
		parent = (i - 1) / 2;
		if (dongle->heap[i]->priority >= dongle->heap[parent]->priority)
			break ;
		tmp = dongle->heap[i];
		dongle->heap[i] = dongle->heap[parent];
		dongle->heap[parent] = tmp;
		i = parent;
	}
}




/*
Adds my_turn to the dongle's waiting queue according to the
scheduler.
- fifo (scheduler==0): added at the end, in arrival order
- edf (scheduler==1): delegates to enqueue_edf, which keeps the
queue sorted by deadline
*/
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

/*
Inserts my_turn into the queue keeping it sorted by ascending
deadline (the closest deadline always comes first). This
implements Earliest Deadline First: when the dongle is released,
whoever is closest to burning out will be woken up first.
*/
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

/*
Removes my_turn from the dongle's queue, whether it's at the
head, in the middle, or at the end.
Called both when the coder gets the dongle (leaves the queue to
use it) and when running becomes 0 while waiting (leaves the
queue without ever getting the dongle).
*/
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
