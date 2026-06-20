/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   dongles_aux.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: agomez-a <agomez-a@student.42urduliz.co    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/20 23:19:58 by agomez-a          #+#    #+#             */
/*   Updated: 2026/06/21 01:06:42 by agomez-a         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "structs.h"

/*
Returns 1 if this coder's request (my_turn) is the one the dongle
should serve next, 0 otherwise.
Two conditions must both be true:
 - dongle->in_use == 0: the dongle is currently free.
 - heap_peek(dongle) == my_turn: my_turn is at the top of the
   min-heap, meaning it has the smallest priority value and is
   therefore first in line according to the active scheduler.
Called inside wait_for_dongle's loop (always under dongle->mutex)
to decide whether to break out of the wait or keep sleeping.
*/
int	is_my_turn(t_dongle *dongle, t_request *my_turn)
{
	if (dongle->in_use == 0 && heap_peek(dongle) == my_turn)
		return (1);
	else
		return (0);
}

/*
Computes the scheduling priority for this coder's next request.
The heap is a min-heap: smaller priority value = served first.

FIFO (scheduler == 0):
 priority = current timestamp. The earlier the request arrives,
 he smaller the value, so requests are served in arrival order.

EDF (scheduler == 1):
 priority = last_compile_start + time_to_burnout, the
 absolute moment at which this coder will burn out if it doesn't
 start compiling. The sooner the deadline, the smaller the value,
 so the coder closest to burning out is always served first.
 last_compile_start is read under state_mutex because the monitor
 thread reads and the coder thread writes it under that same mutex.
*/
long	compute_priority(t_coder *coder)
{
	long	deadline;

	if (coder->sim->params.scheduler == 0)
		return (calculate_time());
	pthread_mutex_lock(&coder->sim->state_mutex);
	deadline = coder->last_compile_start + coder->sim->params.time_to_burnout;
	pthread_mutex_unlock(&coder->sim->state_mutex);
	return (deadline);
}
