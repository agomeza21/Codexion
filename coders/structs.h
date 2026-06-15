/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   structs.h                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: agomez-a <agomez-a@student.42urduliz.co    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/27 15:32:42 by agomez-a          #+#    #+#             */
/*   Updated: 2026/06/15 16:33:13 by agomez-a         ###   ########.fr       */
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

typedef struct s_sim		t_sim;
typedef struct s_request	t_request;

/*
Simulation parameters, parsed from the command-line arguments.
scheduler is stored as 0 (fifo) or 1 (edf).
*/
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

/*
Represents one coder's request to take a specific dongle.
Each take_dongle() call creates its own t_request on the calling
thread's stack, with its own condition variable (self_cond), so
that thread can be woken up individually when it's its turn.
next links it into the dongle's waiting queue.
deadline = last_compile_start + time_to_burnout, only used to
order the queue when scheduler == edf.
*/
typedef struct s_request
{
	pthread_cond_t	self_cond;
	t_request		*next;
	long			timestamp;
	long			deadline;

}	t_request;

/*
Represents one dongle on the table.
in_use: 1 while some coder is currently holding it.
release_time: timestamp of the last release, used to enforce
dongle_cooldown.
mutex: protects in_use, release_time and queue from concurrent
access by multiple coder threads.
queue: linked list of t_request waiting for this dongle, ordered
either by arrival (fifo) or by deadline (edf).
*/
typedef struct s_dongle
{
	int				in_use;
	long			release_time;
	pthread_mutex_t	mutex;
	t_request		*queue;
}	t_dongle;

/*
Represents one programmer/thread.
id: 1 to number_of_coders, as required by the subject.
compile_count: how many times this coder has finished compiling;
checked by the monitor against number_of_compiles_required.
last_compile_start: timestamp of the start of the current/last
compilation; checked by the monitor against time_to_burnout.
left/right: pointers to this coder's two neighboring dongles.
*/
typedef struct s_coder
{
	int			id;
	int			compile_count;
	long		last_compile_start;
	t_sim		*sim;
	pthread_t	thread;
	t_dongle	*left;
	t_dongle	*right;
}	t_coder;

/*
Global simulation state, shared by every thread.
log_mutex: serializes printf calls so log lines never interleave.
monitor: handle for the monitor thread.
running: 1 while the simulation is active; set to 0 by the monitor
once it decides to stop it (either a burnout was detected, or every
coder reached number_of_compiles_required). All threads check this
flag to know when to stop.
start_time: timestamp taken at the beginning of the simulation,
used as the 0 reference for every log timestamp.
*/
typedef struct s_sim
{
	pthread_mutex_t	log_mutex;
	pthread_t		monitor;
	t_params		params;
	t_coder			*coders;
	t_dongle		*dongles;
	int				running;
	long			start_time;
}	t_sim;

void	enqueue(t_dongle *dongle, t_request *my_turn, int scheduler);
void	init_coders(t_coder *coders, t_dongle *dongles, t_sim *sim);
void	enqueue_edf(t_dongle *dongle, t_request *my_turn);
int		take_dongle(t_dongle *dongle, t_coder *coder);
void	dequeue(t_dongle *dongle, t_request *my_turn);
void	log_action(t_sim *sim, int id, char *action);
void	start_coders(t_coder *coders, t_sim *sim);
void	create_dongles(t_dongle *dongles, int n);
void	release_dongle(t_dongle *dongle);
void	*monitor_func(void *arg);
void	cleanup_sim(t_sim *sim);
void	setup_sim(t_sim *sim);
long	calculate_time(void);

#endif