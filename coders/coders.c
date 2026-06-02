/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   coders.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: agomez-a <agomez-a@student.42urduliz.co    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/27 18:51:51 by agomez-a          #+#    #+#             */
/*   Updated: 2026/06/02 15:42:04 by agomez-a         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "structs.h"

void	log_action(t_sim *sim, int id, char *action)
{
	pthread_mutex_lock(&sim->log_mutex);
	printf("%ld %d %s\n", calculate_time() - sim->coders->start_time, sim->coders->id, action);
	pthread_mutex_unlock(&sim->log_mutex);
}

long	calculate_time(void)
{
	struct timeval	tv;

	gettimeofday(&tv, NULL);
	return ((long)(tv.tv_sec * 1000 + tv.tv_usec / 1000));
}

static void	*func(void *arg)
{
	t_coder		*coder;

	coder = (t_coder *)arg;

	while (1)
	{
		if (coder->id % 2 == 0)
		{
			take_dongle(coder->right, coder);
			log_action(coder->sim, coder->id, "has taken a dongle");
			take_dongle(coder->left, coder);
			log_action(coder->sim, coder->id, "has taken a dongle");
		}
		else
		{
			take_dongle(coder->left, coder);
			log_action(coder->sim, coder->id, "has taken a dongle");
			take_dongle(coder->right, coder);
			log_action(coder->sim, coder->id, "has taken a dongle");
		}
		log_action(coder->sim, coder->id, "is compiling");
		usleep(coder->sim->params.time_to_compile * 1000);
		release_dongle(coder->left);
		release_dongle(coder->right);
		log_action(coder->sim, coder->id, "is debugging");
		usleep(coder->sim->params.time_to_debug * 1000);
		log_action(coder->sim, coder->id, "is refactoring");
		usleep(coder->sim->params.time_to_refactor * 1000);
	}
	return (NULL);
}

void	create_coders(t_coder *coders, t_dongle *dongles, t_sim *sim)
{
	int		i;
	long	start_time;

	start_time = calculate_time();
	i = 0;
	while (i < sim->params.number_of_coders)
	{
		coders[i].id = i + 1;
		coders[i].start_time = start_time;
		coders[i].sim = sim;
		coders[i].left = &dongles[i];
		coders[i].right = &dongles[(i + 1) % sim->params.number_of_coders];
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
