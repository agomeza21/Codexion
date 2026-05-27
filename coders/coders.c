#include "structs.h"

static void *func(void *arg)
{
	t_coder *coder;

	coder = (t_coder *)arg;
	printf("%d\n", coder->id);
	return NULL;
}

void create_coders(t_coder *coders, t_params *params)
{
	int i;

	i = 0;
	while (i < params->number_of_coders)
	{
		coders[i].id = i;
		coders[i].params = params;
		pthread_create(&coders[i].thread, NULL, func, &coders[i]);
		i++;
	}
	i = 0;
	while (i < params->number_of_coders)
	{
		pthread_join(coders[i].thread, NULL);
		i++;
	}
}
