/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: agomez-a <agomez-a@student.42urduliz.co    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/03 15:54:21 by agomez-a          #+#    #+#             */
/*   Updated: 2026/06/21 00:48:47 by agomez-a         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "structs.h"

/*
Destroys the mutexes (log_mutex and each dongle's mutex) and frees
the memory used by coders and dongles. Called at the end of main,
after the monitor and all coders have finished.
*/
void	cleanup_sim(t_sim *sim)
{
	int		i;

	i = 0;
	pthread_mutex_destroy(&sim->log_mutex);
	pthread_mutex_destroy(&sim->state_mutex);
	while (i < sim->params.number_of_coders)
	{
		free(sim->dongles[i].heap);
		pthread_mutex_destroy(&sim->dongles[i].mutex);
		i++;
	}
	free(sim->dongles);
	free(sim->coders);
}

/*
Allocates memory for dongles and coders, initializes the dongles,
sets running=1, initializes log_mutex, and stores start_time.
Called before launching any thread.
*/
void	setup_sim(t_sim *sim)
{
	sim->dongles = malloc(sizeof(t_dongle) * sim->params.number_of_coders);
	if (!sim->dongles)
	{
		printf("ERROR: malloc failed\n");
		exit(1);
	}
	create_dongles(sim->dongles, sim->params.number_of_coders);
	sim->running = 1;
	pthread_mutex_init(&sim->log_mutex, NULL);
	pthread_mutex_init(&sim->state_mutex, NULL);
	sim->coders = malloc(sizeof(t_coder) * sim->params.number_of_coders);
	if (!sim->coders)
	{
		free(sim->dongles);
		printf("ERROR: malloc failed\n");
		exit(1);
	}
	sim->start_time = calculate_time();
	init_coders(sim->coders, sim->dongles, sim);
}

/*
Returns the current time in absolute milliseconds.
This is the basis for every time calculation in the program: log
timestamps, deadlines, cooldowns, and burnout detection.
*/
long	calculate_time(void)
{
	struct timeval	tv;

	gettimeofday(&tv, NULL);
	return ((long)(tv.tv_sec * 1000 + tv.tv_usec / 1000));
}

/*
Prints a log line in the format "relative_timestamp id action".
The timestamp is computed as calculate_time() - sim->start_time,
so the log starts at 0 as required by the subject.
Protected by log_mutex so that two threads don't interleave their
lines in the output (the log serialization requirement).
*/
void	log_action(t_sim *sim, int id, char *action)
{
	pthread_mutex_lock(&sim->log_mutex);
	printf("%ld %d %s\n", calculate_time() - sim->start_time, id, action);
	pthread_mutex_unlock(&sim->log_mutex);
}
