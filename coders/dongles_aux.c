/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   dongles_aux.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: agomez-a <agomez-a@student.42urduliz.co    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/20 23:19:58 by agomez-a          #+#    #+#             */
/*   Updated: 2026/06/21 00:20:30 by agomez-a         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "structs.h"

int	is_my_turn(t_dongle *dongle, t_request *my_turn)
{
	if (dongle->in_use == 0 && heap_peek(dongle) == my_turn)
		return (1);
	else
		return (0);
}

long	compute_priority(t_coder *coder)
{
	if (coder->sim->params.scheduler == 0)
		return (calculate_time());
	return (coder->last_compile_start
		+ coder->sim->params.time_to_burnout);
}
