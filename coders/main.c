/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: agomez-a <agomez-a@student.42urduliz.co    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/27 15:29:58 by agomez-a          #+#    #+#             */
/*   Updated: 2026/06/21 13:42:29 by agomez-a         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "structs.h"

/*
Checks that a string represents a valid unsigned integer.
Used to validate the first 7 arguments before converting them
with atoi, since atoi doesn't distinguish between "0" and an
invalid string like "abc" (both return 0).
*/
int	validate_number(char *str, int index)
{
	int	j;

	j = 0;
	if (str[0] == '\0')
	{
		printf("ERROR: argument %d is invalid\n", index);
		return (1);
	}
	while (str[j])
	{
		if (j == 0 && str[j] == '-')
		{
			printf("ERROR: negative number in argument %d\n", index);
			return (1);
		}
		if (str[j] < '0' || str[j] > '9')
		{
			printf("ERROR: argument %d is invalid\n", index);
			return (1);
		}
		j++;
	}
	return (0);
}

/*
Loops through argv[1] to argv[7] validating that they are valid
integers, and checks that argv[8] is exactly "fifo" or "edf".
This is the first validation pass before converting anything
with atoi.
*/
int	validate_args(char **argv)
{
	int	i;

	i = 1;
	while (i < 8)
	{
		if (validate_number(argv[i], i) != 0)
			return (1);
		i++;
	}
	if (strcmp(argv[8], "fifo") != 0 && strcmp(argv[8], "edf") != 0)
	{
		printf("ERROR: invalid scheduler\n");
		return (1);
	}
	return (0);
}

/*
Once the arguments have been converted with atoi, checks that the
values make sense: all > 0 except dongle_cooldown, which can be 0
(no cooldown is valid).
*/
int	check_values(t_params *params)
{
	if (params->number_of_coders <= 0 || params->time_to_burnout <= 0
		|| params->time_to_compile <= 0 || params->time_to_debug <= 0)
	{
		printf("ERROR: arguments have to be greater than 0\n");
		return (1);
	}
	if (params->time_to_refactor <= 0
		|| params->number_of_compiles_required <= 0)
	{
		printf("ERROR: arguments have to be greater than 0\n");
		return (1);
	}
	if (params->dongle_cooldown < 0)
	{
		printf("ERROR: dongle_cooldown can't be negative\n");
		return (1);
	}
	return (0);
}

/*
Converts all text arguments to integers with atoi and stores them
in params. The scheduler is stored as 0 (fifo) or 1 (edf) so it
can easily be used as a condition/index throughout the rest of
the code. Calls check_values for the final validation.
*/
int	parse_args(t_params *params, char **argv)
{
	params->number_of_coders = atoi(argv[1]);
	params->time_to_burnout = atoi(argv[2]);
	params->time_to_compile = atoi(argv[3]);
	params->time_to_debug = atoi(argv[4]);
	params->time_to_refactor = atoi(argv[5]);
	params->number_of_compiles_required = atoi(argv[6]);
	params->dongle_cooldown = atoi(argv[7]);
	params->scheduler = (strcmp(argv[8], "edf") == 0);
	if (check_values(params) != 0)
		return (1);
	return (0);
}

/*
Entry point of the program.
Flow: validate argc, validate and parse arguments, initialize the
simulation, launch the monitor thread and the coder threads, wait
for all of them to finish, and free all memory/resources before
exiting.
*/
int	main(int argc, char **argv)
{
	t_sim	sim;

	if (argc != 9)
	{
		printf("ERROR: invalid number of arguments\n");
		return (1);
	}
	if (validate_args(argv) != 0)
		return (1);
	if (parse_args(&sim.params, argv) != 0)
		return (1);
	setup_sim(&sim);
	pthread_create(&sim.monitor, NULL, monitor_func, &sim);
	start_coders(sim.coders, &sim);
	pthread_join(sim.monitor, NULL);
	cleanup_sim(&sim);
	return (0);
}
