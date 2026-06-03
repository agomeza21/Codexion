/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: agomez-a <agomez-a@student.42urduliz.co    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/03 15:54:21 by agomez-a          #+#    #+#             */
/*   Updated: 2026/06/03 15:56:06 by agomez-a         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "structs.h"

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
