/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   structs.h                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: agomez-a <agomez-a@student.42urduliz.co    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/27 15:32:42 by agomez-a          #+#    #+#             */
/*   Updated: 2026/06/21 00:22:51 by agomez-a         ###   ########.fr       */
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
priority is the value the heap compares: for fifo it's the
arrival timestamp (earlier = smaller = served first), for edf
it's the deadline (last_compile_start + time_to_burnout, sooner
= smaller = served first). The heap doesn't know which scheduler
is active, it just always extracts the smallest priority.
*/
typedef struct s_request
{
	pthread_cond_t	self_cond;
	long			priority;
}	t_request;

/*
Represents one dongle on the table.
in_use: 1 while some coder is currently holding it.
release_time: timestamp of the last release, used to enforce
dongle_cooldown.
mutex: protects in_use, release_time and the heap from
concurrent access by multiple coder threads.
heap: min-heap of pointers to waiting t_request, ordered by
priority (smallest first). heap[0] is always the next request
to be served. Allocated with size number_of_coders, since at
most one request per coder can be waiting on a single dongle.
heap_size: current number of elements stored in heap.
*/
typedef struct s_dongle
{
	int				in_use;
	long			release_time;
	pthread_mutex_t	mutex;
	t_request		**heap;
	int				heap_size;
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
	pthread_mutex_t	state_mutex;
	pthread_t		monitor;
	t_params		params;
	t_coder			*coders;
	t_dongle		*dongles;
	int				running;
	long			start_time;
}	t_sim;

void		init_coders(t_coder *coders, t_dongle *dongles, t_sim *sim);
int			is_my_turn(t_dongle *dongle, t_request *my_turn);
int			take_dongle(t_dongle *dongle, t_coder *coder);
void		log_action(t_sim *sim, int id, char *action);
void		heap_remove(t_dongle *dongle, t_request *req);
void		heap_push(t_dongle *dongle, t_request *req);
void		start_coders(t_coder *coders, t_sim *sim);
void		create_dongles(t_dongle *dongles, int n);
int			should_keep_working(t_coder *coder);
void		release_dongle(t_dongle *dongle);
int			abort_if_stopped(t_coder *coder);
long		compute_priority(t_coder *coder);
int			still_running(t_coder *coder);
t_request	*heap_peek(t_dongle *dongle);
t_request	*heap_pop(t_dongle *dongle);
void		*monitor_func(void *arg);
void		cleanup_sim(t_sim *sim);
void		setup_sim(t_sim *sim);
long		calculate_time(void);

#endif