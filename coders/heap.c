/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   heap.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: agomez-a <agomez-a@student.42urduliz.co    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/12 10:28:07 by agomez-a          #+#    #+#             */
/*   Updated: 2026/06/16 12:09:34 by agomez-a         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "structs.h"

/*
Restores the heap property upward from position i.
While the element at i has a smaller priority than its parent,
they swap and the process repeats with the parent's position.
Used by heap_push and heap_remove when the replaced element
is smaller than its new parent.
*/
static void	sift_up(t_dongle *dongle, int i)
{
	int			parent;
	t_request	*tmp;

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
Restores the heap property downward from position i.
Finds the smallest child of i; if it's smaller than i, they
swap and the process repeats from the child's position.
Used by heap_pop and heap_remove when the replaced element
is larger than one of its new children.
*/
static void	sift_down(t_dongle *dongle, int i)
{
	int			smallest;
	t_request	*tmp;
	
	while (2 * i + 1 < dongle->heap_size)
	{
		smallest = 2 * i + 1;
		if (2 * i + 2 < dongle->heap_size
			&& dongle->heap[2 * i + 2]->priority
			< dongle->heap[smallest]->priority)
			smallest = 2 * i + 2;
		if (dongle->heap[i]->priority <= dongle->heap[smallest]->priority)
			break ;
		tmp = dongle->heap[i];
		dongle->heap[i] = dongle->heap[smallest];
		dongle->heap[smallest] = tmp;
		i = smallest;
	}
}

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

	dongle->heap[dongle->heap_size] = req;
	i = dongle->heap_size;
	dongle->heap_size++;
	sift_up(dongle, i);
}

/*
Removes and returns the request with the smallest priority.
The last element is moved to the root and "sunk down"
(sift-down): while it's larger than its smallest child,
they swap. This restores the heap property after removal.
Returns NULL if the heap is empty.
*/
t_request	*heap_pop(t_dongle *dongle)
{
	t_request	*min;

	if (dongle->heap_size == 0)
		return (NULL);
	min = dongle->heap[0];
	dongle->heap_size--;
	dongle->heap[0] = dongle->heap[dongle->heap_size];
	sift_down(dongle, 0);
	return (min);
}

/*
Removes a specific request from the heap (not necessarily the
root). Used when running becomes 0 while a coder is waiting:
it needs to remove itself from the heap without having taken
the dongle. Finds the request by pointer comparison, replaces
it with the last element, and restores the heap property by
calling sift_up or sift_down as needed.
*/
void	heap_remove(t_dongle *dongle, t_request *req)
{
	int 		i;
	int			parent;

	i = 0;
	while (i < dongle->heap_size && dongle->heap[i] != req)
		i++;
	if (i == dongle->heap_size)
		return ;
	dongle->heap_size--;
	dongle->heap[i] = dongle->heap[dongle->heap_size];
	if (dongle->heap_size == 0 || i == dongle->heap_size)
		return ;
	parent = (i - 1) / 2;
	if (i > 0 && dongle->heap[i]->priority < dongle->heap[parent]->priority)
		sift_up(dongle, i);
	else
		sift_down(dongle, i);
}
