/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   structs.h                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: agomez-a <agomez-a@student.42urduliz.co    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/27 15:32:42 by agomez-a          #+#    #+#             */
/*   Updated: 2026/05/27 18:50:05 by agomez-a         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef STRUCTS_H
# define STRUCTS_H

# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <pthread.h>
# include <sys/time.h>
# include <unistd.h>

typedef struct s_params
{
    int     number_of_coders;
    int     time_to_burnout;
    int     time_to_compile;
    int     time_to_debug;
    int     time_to_refactor;
    int     number_of_compiles_required;
    int     dongle_cooldown;
    int     scheduler;
}   t_params;

typedef struct s_coder
{
    int         id;
    t_params    *params;
    pthread_t   thread;
}   t_coder;

typedef struct s_dongle
{
	int				in_use;
	long			release_time;
	pthread_mutex_t	mutex;
	pthread_cond_t	cond;
}	t_dongle;

void	create_coders(t_coder *coders, t_params *params);
long	calculate_time(void);

#endif