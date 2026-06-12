/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: agomez-a <agomez-a@student.42urduliz.co    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/03 15:54:21 by agomez-a          #+#    #+#             */
/*   Updated: 2026/06/12 10:29:19 by agomez-a         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "structs.h"

void	cleanup_sim(t_sim *sim)
{
	int		i;

	i = 0;
	pthread_mutex_destroy(&sim->log_mutex);
	while (i < sim->params.number_of_coders)
	{
		pthread_mutex_destroy(&sim->dongles[i].mutex);
		i++;
	}
	free(sim->dongles);
	free(sim->coders);
}

void	setup_sim(t_sim *sim)
{
	sim->dongles = malloc(sizeof(t_dongle) * sim->params.number_of_coders);
	create_dongles(sim->dongles, sim->params.number_of_coders);
	sim->running = 1;
	pthread_mutex_init(&sim->log_mutex, NULL);
	sim->coders = malloc(sizeof(t_coder) * sim->params.number_of_coders);
	sim->start_time = calculate_time();
	init_coders(sim->coders, sim->dongles, sim);
}

long	calculate_time(void)
{
	struct timeval	tv;

	gettimeofday(&tv, NULL);
	return ((long)(tv.tv_sec * 1000 + tv.tv_usec / 1000));
}

void	log_action(t_sim *sim, int id, char *action)
{
	pthread_mutex_lock(&sim->log_mutex);
	printf("%ld %d %s\n", calculate_time() - sim->start_time, id, action);
	pthread_mutex_unlock(&sim->log_mutex);
}
