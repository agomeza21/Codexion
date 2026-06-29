/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   coders_aux.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: agomez-a <agomez-a@student.42urduliz.co    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/20 22:48:13 by agomez-a          #+#    #+#             */
/*   Updated: 2026/06/29 10:12:26 by agomez-a         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "structs.h"

/*
Returns 1 if this coder has already reached number_of_compiles_required.
Called after the last compile to avoid printing debug/refactor logs
once the simulation is effectively  done foe this coder.
*/
int	reached_compile_limit(t_coder *coder)
{
	int	reached;

	pthread_mutex_lock(&coder->sim->state_mutex);
	reached = (coder->compile_count
		>= coder->sim->params.number_of_compiles_required);
	pthread_mutex_unlock(&coder->sim->state_mutex);
	return  (reached);
}

/*
Called at the start of compile_cycle, AFTER both dongles are held.
Checks sim->running under state_mutex:
 - If running == 0 (simulation stopped while waiting for dongles):
   releases both dongles and returns 1 so compile_cycle aborts.
 - If running == 1:
   records last_compile_start = now (resets the burnout clock)
   and returns 0 so compile_cycle proceeds normally.
Writing last_compile_start inside state_mutex is required because
the monitor reads it under the same mutex in check_burnout.
*/
int	abort_if_stopped(t_coder *coder)
{
	pthread_mutex_lock(&coder->sim->state_mutex);
	if (coder->sim->running == 0)
	{
		pthread_mutex_unlock(&coder->sim->state_mutex);
		release_dongle(coder->left);
		release_dongle(coder->right);
		return (1);
	}
	coder->last_compile_start = calculate_time();
	pthread_mutex_unlock(&coder->sim->state_mutex);
	return (0);
}

/*
Thread-safe read of sim->running.
Returns 1 if the simulation is still active, 0 if it has stopped.
Used after each sleep phase (debug, refactor) to avoid printing
logs or starting a new cycle after the simulation has ended.
*/
int	still_running(t_coder *coder)
{
	int	is_running;

	pthread_mutex_lock(&coder->sim->state_mutex);
	is_running = coder->sim->running;
	pthread_mutex_unlock(&coder->sim->state_mutex);
	return (is_running);
}

/*
Returns 1 if the coder should attempt another compile cycle:
 - the simulation is still running (running == 1), AND
 - this coder hasn't yet reached number_of_compiles_required.
Both fields are read under state_mutex to avoid a race with the
monitor thread, which writes running and the coder threads which
write compile_count.
Used as the condition of the main loop in func().
*/
int	should_keep_working(t_coder *coder)
{
	int	keep_going;

	pthread_mutex_lock(&coder->sim->state_mutex);
	keep_going = (coder->sim->running == 1
			&& coder->compile_count
			< coder->sim->params.number_of_compiles_required);
	pthread_mutex_unlock(&coder->sim->state_mutex);
	return (keep_going);
}
