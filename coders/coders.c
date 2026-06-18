/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   coders.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: agomez-a <agomez-a@student.42urduliz.co    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/27 18:51:51 by agomez-a          #+#    #+#             */
/*   Updated: 2026/06/18 16:16:43 by agomez-a         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "structs.h"

/*
Tries to take the two dongles needed to compile.
The order of acquisition depends on the parity of the id: even ids
take the right dongle first, then the left; odd ids do the opposite.
This breaks the circular wait chain and avoids the classic deadlock
from the dining philosophers problem (not everyone waits in the
same direction).

Returns:
	1 -> both dongles acquired
	0 -> running became 0 while waiting for the FIRST dongle
		(nothing was taken)
	2 -> got the right dongle but running became 0 while waiting
		for the left one
	3 -> got the left dongle but running became 0 while waiting
		for the right one
	Values 2/3 let func() know which dongle needs to be released.
*/
static int	grab_dongles(t_coder *coder)
{
	if (coder->id % 2 == 0)
	{
		if (!take_dongle(coder->right, coder))
			return (0);
		if (!take_dongle(coder->left, coder))
			return (2);
		log_action(coder->sim, coder->id, "has taken a dongle");
		log_action(coder->sim, coder->id, "has taken a dongle");
	}
	else
	{
		if (!take_dongle(coder->left, coder))
			return (0);
		if (!take_dongle(coder->right, coder))
			return (3);
		log_action(coder->sim, coder->id, "has taken a dongle");
		log_action(coder->sim, coder->id, "has taken a dongle");
	}
	return (1);
}

/*
Runs one full cycle: compile -> debug -> refactor.
Checks sim->running BEFORE each phase (including the first one,
before compiling) so that if the simulation has ended, the coder
doesn't register a compile or print logs that shouldn't exist.
If running becomes 0 during compilation, the dongles are released
before returning; during debug/refactor there are no dongles left
to release.
*/
static int	compile_cycle(t_coder *coder)
{
	int is_running;

	pthread_mutex_lock(&coder->sim->state_mutex);
	if (coder->sim->running == 0)
	{
		pthread_mutex_unlock(&coder->sim->state_mutex);
		release_dongle(coder->left);
		release_dongle(coder->right);
		return (0);
	}
	coder->last_compile_start = calculate_time();
	pthread_mutex_unlock(&coder->sim->state_mutex);
	log_action(coder->sim, coder->id, "is compiling");
	usleep(coder->sim->params.time_to_compile * 1000);
	pthread_mutex_lock(&coder->sim->state_mutex);
	coder->compile_count++;
	pthread_mutex_unlock(&coder->sim->state_mutex);
	release_dongle(coder->left);
	release_dongle(coder->right);
	pthread_mutex_lock(&coder->sim->state_mutex);
	is_running = coder->sim->running;
	pthread_mutex_unlock(&coder->sim->state_mutex);
	if (is_running == 0)
		return (0);
	log_action(coder->sim, coder->id, "is debugging");
	usleep(coder->sim->params.time_to_debug * 1000);
	pthread_mutex_lock(&coder->sim->state_mutex);
	is_running = coder->sim->running;
	pthread_mutex_unlock(&coder->sim->state_mutex);
	if (is_running == 0)
		return (0);
	log_action(coder->sim, coder->id, "is refactoring");
	usleep(coder->sim->params.time_to_refactor * 1000);
	pthread_mutex_lock(&coder->sim->state_mutex);
	is_running = coder->sim->running;
	pthread_mutex_unlock(&coder->sim->state_mutex);
	if (is_running == 0)
		return (0);
	return (1);
}

/*
Function executed by each coder thread.
Main loop: while the simulation is active, tries to take both
dongles and run a compile cycle.
If grab_dongles returns 2 or 3, releases whichever dongle it did
manage to get before ending the thread, so it isn't left held.
*/
static void	*func(void *arg)
{
	t_coder		*coder;
	int			result;
	int			is_running;

	coder = (t_coder *)arg;
	pthread_mutex_lock(&coder->sim->state_mutex);
	is_running = coder->sim->running;
	pthread_mutex_unlock(&coder->sim->state_mutex);
	while (is_running == 1)
	{
		result = grab_dongles(coder);
		if (result == 2)
		{
			release_dongle(coder->right);
			return (NULL);
		}
		if (result == 3)
		{
			release_dongle(coder->left);
			return (NULL);
		}
		if (!compile_cycle(coder))
			return (NULL);
		pthread_mutex_lock(&coder->sim->state_mutex);
		is_running = coder->sim->running;
		pthread_mutex_unlock(&coder->sim->state_mutex);
	}
	return (NULL);
}

/*
Initializes each coder's data before launching the threads:
- id from 1 to N (as required by the subject)
- pointers to its left and right dongles (circular layout)
- compile_count set to 0
- last_compile_start = simulation's start_time, so the monitor
  doesn't detect a false burnout before anyone has started
  compiling
*/
void	init_coders(t_coder *coders, t_dongle *dongles, t_sim *sim)
{
	int		i;

	i = 0;
	while (i < sim->params.number_of_coders)
	{
		coders[i].id = i + 1;
		coders[i].sim = sim;
		coders[i].left = &dongles[i];
		coders[i].right = &dongles[(i + 1) % sim->params.number_of_coders];
		coders[i].compile_count = 0;
		coders[i].last_compile_start = sim->start_time;
		i++;
	}
}

/*
Creates one thread per coder (pthread_create) and then waits for
all of them to finish (pthread_join). Split into two loops because
join can't be called until ALL threads have been created.
*/
void	start_coders(t_coder *coders, t_sim *sim)
{
	int	i;

	i = 0;
	while (i < sim->params.number_of_coders)
	{
		pthread_create(&coders[i].thread, NULL, func, &coders[i]);
		i++;
	}
	i = 0;
	while (i < sim->params.number_of_coders)
	{
		pthread_join(coders[i].thread, NULL);
		i++;
	}
}
