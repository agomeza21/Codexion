/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: agomez-a <agomez-a@student.42urduliz.co    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/27 15:29:58 by agomez-a          #+#    #+#             */
/*   Updated: 2026/06/04 13:14:32 by agomez-a         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "structs.h"

int	validate_number(char *str, int index)
{
	int	j;

	j = 0;
	if (str[0] == '\0')
		return (1);
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
		printf("ERROR: invalid scheduer\n");
		return (1);
	}
	return (0);
}

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

int	main(int argc, char **argv)
{
	t_sim	sim;

	if (argc != 9)
	{
		printf("ERROR: invalid number of arguments");
		return (1);
	}
	if (validate_args(argv) != 0)
		return (1);
	if (parse_args(&sim.params, argv) != 0)
		return (1);
	sim.dongles = malloc(sizeof(t_dongle) * sim.params.number_of_coders);
	create_dongles(sim.dongles, sim.params.number_of_coders);
	sim.running = 1;
	pthread_mutex_init(&sim.log_mutex, NULL);
	sim.coders = malloc(sizeof(t_coder) * sim.params.number_of_coders);
	sim.start_time = calculate_time();
	init_coders(sim.coders, sim.dongles, &sim);
	pthread_create(&sim.monitor, NULL, monitor_func, &sim);
	start_coders(sim.coders, &sim);
	pthread_join(sim.monitor, NULL);
	return (0);
}
