/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   coders.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: agomez-a <agomez-a@student.42urduliz.co    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/27 18:51:51 by agomez-a          #+#    #+#             */
/*   Updated: 2026/06/03 17:42:50 by agomez-a         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "structs.h"

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

static int	compile_cycle(t_coder *coder)
{
	coder->last_compile_start = calculate_time();
	log_action(coder->sim, coder->id, "is compiling");
	coder->compile_count++;
	usleep(coder->sim->params.time_to_compile * 1000);
	release_dongle(coder->left);
	release_dongle(coder->right);
	if (coder->sim->running == 0)
		return (0);
	log_action(coder->sim, coder->id, "is debugging");
	usleep(coder->sim->params.time_to_debug * 1000);
	if (coder->sim->running == 0)
		return (0);
	log_action(coder->sim, coder->id, "is refactoring");
	usleep(coder->sim->params.time_to_refactor * 1000);
	if (coder->sim->running == 0)
		return (0);
	return (1);
}

static void	*func(void *arg)
{
	t_coder		*coder;
	int			result;

	coder = (t_coder *)arg;
	while (coder->sim->running == 1)
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
	}
	return (NULL);
}

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
