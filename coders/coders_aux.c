/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   coders_aux.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: agomez-a <agomez-a@student.42urduliz.co    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/20 22:48:13 by agomez-a          #+#    #+#             */
/*   Updated: 2026/06/21 00:22:04 by agomez-a         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "structs.h"

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

int	still_running(t_coder *coder)
{
	int	is_running;

	pthread_mutex_lock(&coder->sim->state_mutex);
	is_running = coder->sim->running;
	pthread_mutex_unlock(&coder->sim->state_mutex);
	return (is_running);
}

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
