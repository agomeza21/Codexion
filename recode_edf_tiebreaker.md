# Recode: EDF tie-breaker (mayor coder_id gana en empate)

3 archivos: `structs.h`, `dongles.c`, `heap.c`.

---

## 1. structs.h — `t_request`

```c
typedef struct s_request
{
	pthread_cond_t	self_cond;
	long			priority;
	int				coder_id;
}	t_request;
```

---

## 2. dongles.c — `take_dongle`

```c
	pthread_cond_init(&my_turn.self_cond, NULL);
	my_turn.priority = compute_priority(coder);
	my_turn.coder_id = coder->id;
	pthread_mutex_lock(&dongle->mutex);
```

---

## 3. heap.c — `sift_up`

```c
static void	sift_up(t_dongle *dongle, int i)
{
	int			parent;
	t_request	*tmp;

	while (i > 0)
	{
		parent = (i - 1) / 2;
		if (dongle->heap[i]->priority > dongle->heap[parent]->priority)
			break ;
		if (dongle->heap[i]->priority == dongle->heap[parent]->priority
			&& dongle->heap[i]->coder_id <= dongle->heap[parent]->coder_id)
			break ;
		tmp = dongle->heap[i];
		dongle->heap[i] = dongle->heap[parent];
		dongle->heap[parent] = tmp;
		i = parent;
	}
}
```

## 3. heap.c — `sift_down`

```c
static void	sift_down(t_dongle *dongle, int i)
{
	int			smallest;
	t_request	*tmp;

	while (2 * i + 1 < dongle->heap_size)
	{
		smallest = 2 * i + 1;
		if (2 * i + 2 < dongle->heap_size)
		{
			if (dongle->heap[2 * i + 2]->priority
				< dongle->heap[smallest]->priority)
				smallest = 2 * i + 2;
			else if (dongle->heap[2 * i + 2]->priority
				== dongle->heap[smallest]->priority
				&& dongle->heap[2 * i + 2]->coder_id
				> dongle->heap[smallest]->coder_id)
				smallest = 2 * i + 2;
		}
		if (dongle->heap[i]->priority < dongle->heap[smallest]->priority)
			break ;
		if (dongle->heap[i]->priority == dongle->heap[smallest]->priority
			&& dongle->heap[i]->coder_id >= dongle->heap[smallest]->coder_id)
			break ;
		tmp = dongle->heap[i];
		dongle->heap[i] = dongle->heap[smallest];
		dongle->heap[smallest] = tmp;
		i = smallest;
	}
}
```
