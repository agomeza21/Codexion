/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   monitor.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: agomez-a <agomez-a@student.42urduliz.co    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/03 12:50:25 by agomez-a          #+#    #+#             */
/*   Updated: 2026/06/15 21:31:33 by agomez-a         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "structs.h"

/*
Goes through every dongle and sends pthread_cond_signal to each
request in its queue. Called when running becomes 0, so that no
coder is left sleeping forever in pthread_cond_timedwait
waiting for a dongle that will never come -- this way every
thread can wake up, check running==0, and finish cleanly.
*/
static void	wake_all_dongles(t_sim *sim)
{
	t_request	*current;
	int			i;

	i = 0;
	while (i < sim->params.number_of_coders)
	{
		pthread_mutex_lock(&sim->dongles[i].mutex);
		current = sim->dongles[i].queue;
		while (current != NULL)
		{
			pthread_cond_signal(&current->self_cond);
			current = current->next;
		}
		pthread_mutex_unlock(&sim->dongles[i].mutex);
		i++;
	}
}

/*
Checks whether ALL coders have reached number_of_compiles_required.
If so, sets running=0 and returns 1 (successful end of simulation).
The subject requires "all of them", not "any of them" -- hence the
full loop before deciding.
*/
static int	check_all_done(t_sim *sim)
{
	int		j;
	int		all_done;

	all_done = 1;
	j = 0;
	while (j < sim->params.number_of_coders)
	{
		if (sim->coders[j].compile_count
			< sim->params.number_of_compiles_required)
		{
			all_done = 0;
			break ;
		}
		j++;
	}
	if (all_done == 1)
	{
		sim->running = 0;
		return (1);
	}
	return (0);
}

/*
Checks whether any coder has gone >= time_to_burnout without
compiling since its last_compile_start. If so, logs it as
"burned out", sets running=0, and returns 1.
This stops the simulation immediately (no need to keep checking
the others).
*/
static int	check_burnout(t_sim *sim)
{
	int		i;

	i = 0;
	while (i < sim->params.number_of_coders)
	{
		if (calculate_time() - sim->coders[i].last_compile_start
			>= sim->params.time_to_burnout)
		{
			log_action(sim, sim->coders[i].id, "burned out");
			sim->running = 0;
			return (1);
		}
		i++;
	}
	return (0);
}

/*
Monitor thread: runs in parallel with the coders and checks every
1ms whether there's a burnout or whether the simulation has
finished successfully.
The 1ms interval guarantees the requirement that a burnout be
logged within 10ms of when it actually occurs.
When either condition is detected, wakes up every blocked coder
(wake_all_dongles) so they can finish.
*/
void	*monitor_func(void *arg)
{
	t_sim	*sim;

	sim = (t_sim *)arg;
	while (sim->running == 1)
	{
		if (check_burnout(sim) || check_all_done(sim))
		{
			sim->running = 0;
			wake_all_dongles(sim);
			return (NULL);
		}
		usleep(1000);
	}
	return (NULL);
}
