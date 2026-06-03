/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   monitor.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: agomez-a <agomez-a@student.42urduliz.co    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/03 12:50:25 by agomez-a          #+#    #+#             */
/*   Updated: 2026/06/03 16:41:00 by agomez-a         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "structs.h"

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
