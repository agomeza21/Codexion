/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   structs.h                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: agomez-a <agomez-a@student.42urduliz.co    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/27 15:32:42 by agomez-a          #+#    #+#             */
/*   Updated: 2026/05/29 13:40:37 by agomez-a         ###   ########.fr       */
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
	int		number_of_coders;
	int		time_to_burnout;
	int		time_to_compile;
	int		time_to_debug;
	int		time_to_refactor;
	int		number_of_compiles_required;
	int		dongle_cooldown;
	int		scheduler;
}	t_params;

typedef struct s_dongle
{
	int				in_use;
	long			release_time;
	pthread_mutex_t	mutex;
	pthread_cond_t	cond;
}	t_dongle;

typedef struct s_coder
{
	int			id;
	long		start_time;
	t_params	*params;
	pthread_t	thread;
	t_dongle	*left;
	t_dongle	*right;
}	t_coder;

void	create_coders(t_coder *coders, t_dongle *dongles, t_params *params);
void    take_dongle(t_dongle *dongle, t_coder *coder);
void    create_dongles(t_dongle *dongles, int n);
void    release_dongle(t_dongle *dongle);
long	calculate_time(void);

#endif