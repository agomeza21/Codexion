*This project has been created as part of the 42 curriculum by agomez-a.*

# Codexion

## Description

Codexion is a multithreaded simulation of resource sharing and synchronization,
inspired by the classic dining philosophers problem. Instead of philosophers and
forks, the simulation models **coders** sitting at a circular co-working hub,
competing for a limited number of **USB dongles** needed to compile quantum code.

Each coder cycles endlessly through three states:

```
take dongles -> compile -> debug -> refactor -> (repeat)
```

To compile, a coder needs **two dongles** at the same time (one on their left,
one on their right). There are as many dongles as coders, arranged in a circle
so that each dongle is shared between two neighboring coders.

The goal of the simulation is to keep every coder compiling regularly without
anyone **burning out** (failing to start a compilation within `time_to_burnout`
milliseconds), while handling deadlock prevention, dongle cooldowns, and fair
scheduling between two strategies: **FIFO (First In First Out)** and **EDF (Earliest Deadline First)**.

The simulation stops as soon as either:
- a coder burns out, or
- every coder has compiled at least `number_of_compiles_required` times.

## Instructions

### Compilation

```bash
make
```

This builds the `codexion` binary at the root of the repository. The Makefile
compiles all sources from the `coders/` directory with `-Wall -Wextra -Werror
-pthread`, without relinking unnecessarily.

Other available rules:

```bash
make clean   # remove object files
make fclean  # remove object files and the binary
make re      # fclean + all
```

### Running

```bash
./codexion number_of_coders time_to_burnout time_to_compile time_to_debug \
           time_to_refactor number_of_compiles_required dongle_cooldown scheduler
```

| Argument                       | Description                                                                 |
|---------------------------------|------------------------------------------------------------------------------|
| `number_of_coders`              | Number of coders (and dongles)                                                |
| `time_to_burnout`                | Max ms without starting a compile before a coder burns out                   |
| `time_to_compile`                | Duration in ms of the compile phase                                          |
| `time_to_debug`                  | Duration in ms of the debug phase                                            |
| `time_to_refactor`               | Duration in ms of the refactor phase                                         |
| `number_of_compiles_required`    | Simulation stops once every coder reaches this many compiles                |
| `dongle_cooldown`                 | Ms a dongle stays unavailable after being released (0 = no cooldown)        |
| `scheduler`                       | `fifo` or `edf`, the arbitration policy used when several coders want the same dongle |

All arguments are mandatory. Invalid input (negative numbers, non-integer
values, or a scheduler different from `fifo`/`edf`) is rejected with an error
message and the program exits without running the simulation.

### Example

```bash
./codexion 3 800 200 200 200 5 0 fifo
```

## Blocking cases handled

**Deadlock prevention**
Every coder needs to hold two dongles simultaneously (their left and right
neighbor's dongle). If all coders tried to take their left dongle first, they
would form a circular wait and deadlock. To break this cycle, coders with an
even id take their **right** dongle first, while coders with an odd id take
their **left** dongle first. This guarantees that not all coders are waiting
in the same direction, so the circular-wait condition required for deadlock
can never occur.

**Partial acquisition during shutdown**
If the simulation stops (`running = 0`) while a coder is holding one dongle
and waiting for the second, that coder releases the dongle it already holds
before exiting its thread. This prevents a dongle from being left permanently
marked as in use after the simulation ends.

**Starvation prevention**
Two scheduling policies decide who gets a dongle next when it's released:
- **FIFO**: requests are served in arrival order (a linked queue per dongle).
- **EDF**: requests are served by the closest deadline
  (`last_compile_start + time_to_burnout`), so the coder closest to burning
  out is always prioritized first. This reduces starvation compared to FIFO, though with very tight parameters (where the total cycle time leaves little or no slack before time_to_burnout) a burnout can still occur — EDF minimizes who is left waiting longest, but cannot create time that the parameters don't allow.

**Dongle cooldown**
After a dongle is released, it cannot be taken again until
`dongle_cooldown` milliseconds have passed. This is enforced with
`pthread_cond_timedwait`: a waiting coder wakes up automatically once the
cooldown expires, even if no other thread signals it. Without this, a coder
could be left sleeping forever if it was the only one waiting when the
cooldown started.

**Accurate burnout detection and clean shutdown**
A dedicated monitor thread checks every millisecond whether any coder has
exceeded `time_to_burnout` without compiling, or whether every coder has
reached `number_of_compiles_required`. The 1ms polling interval guarantees
the burnout message is logged within 10ms of the real event, as required.
When the monitor decides to stop the simulation, it broadcasts to every
dongle's waiting queue so that no coder thread is left blocked forever in
`pthread_cond_timedwait`.

**Log serialization**
All log lines are printed through a single function protected by a mutex
(`log_mutex`), guaranteeing that two threads can never interleave their
output mid-line.

**Single coder edge case**
With `number_of_coders = 1`, there is only one dongle, but a coder needs two
to compile. The coder takes that single dongle as its "left", then tries to
take the same dongle as its "right" and blocks forever (it's already in use).
The monitor detects the burnout normally and stops the simulation — no
deadlock, no crash.

## Thread synchronization mechanisms

**`pthread_mutex_t` per dongle**
Each dongle has its own mutex protecting its state (`in_use`, `release_time`,
and its waiting queue). Any thread reading or modifying a dongle's state must
hold this mutex first, preventing race conditions such as two coders both
believing they successfully took the same dongle.

**Per-request `pthread_cond_t` (custom waiting queue)**
Instead of a single condition variable per dongle, each waiting request
(`t_request`) carries its **own** `pthread_cond_t`, created on the waiting
thread's stack inside `take_dongle`. This allows the dongle to wake up a
**specific** coder (via `pthread_cond_signal` on that request's condition)
rather than waking everyone and letting them race — which is what makes a
true FIFO/EDF ordering possible: the dongle is only handed to the request at
the front of the queue.

**Queue-based scheduling (`enqueue` / `enqueue_edf` / `dequeue`)**
When a coder calls `take_dongle`, it builds a `t_request` with its arrival
timestamp and computed deadline, and enqueues it:
- FIFO appends it to the end of a linked list.
- EDF inserts it in sorted order by deadline (closest deadline first).

The coder then waits (`pthread_cond_timedwait`) until it is at the front of
the queue, the dongle is free, and the cooldown has elapsed. `release_dongle`
signals only the front of the queue, handing off the dongle to the correct
coder according to the active scheduler.

**`running` flag and `wake_all_dongles`**
`sim->running` is a shared flag (1 while active, 0 once the monitor decides
to stop). It is read by every coder thread before/after each phase. Because
a coder might be asleep in `pthread_cond_timedwait` when `running` is set to
0, the monitor calls `wake_all_dongles`, which locks each dongle's mutex and
signals every pending request — guaranteeing every thread wakes up, sees
`running == 0`, and exits cleanly. This is a textbook example of why
condition variables must always be checked in a `while` loop: a wake-up does
not guarantee the awaited condition is true, only that it's worth re-checking.

**`log_mutex`**
A single mutex serializes all calls to `log_action`, ensuring thread-safe,
non-interleaved output regardless of how many coder threads print at the
same time.

## Resources

- `man pthread_create`, `man pthread_mutex_lock`, `man pthread_cond_wait`,
  `man pthread_cond_timedwait`, `man gettimeofday` — POSIX threads reference.
- Wikipedia: *Dining philosophers problem*, *Earliest deadline first
  scheduling*, *Coffman conditions* — for the theoretical background on
  deadlock and real-time scheduling.

### AI usage

An AI assistant (Claude) was used throughout this project as a learning aid,
following the guidelines in the subject:

- **Concept explanations**: understanding what threads, mutexes, condition
  variables, deadlocks and starvation are, and how `pthread_create`,
  `pthread_join`, `pthread_mutex_lock/unlock`, `pthread_cond_wait`,
  `pthread_cond_broadcast` and `pthread_cond_timedwait` work, before writing
  any code that used them.
- **Debugging concurrency issues**: identifying the cause of deadlocks
  (circular wait on dongles), missed wake-ups after a dongle release during
  cooldown, and a race condition where a coder could start (and log) an
  extra compile cycle after the simulation should have stopped.
- **Documentation**: drafting and translating the in-code comments and this
  README.

All AI-suggested code was written, tested, and understood step by step by
the author, with every concurrency bug reproduced and explained before being
fixed.

## Usage examples

```
$ ./codexion 3 800 200 200 200 5 0 fifo
```
Runs cleanly: all 3 coders compile 5 times each, then the simulation stops
on its own with no burnout.

A burnout, caused by `time_to_compile` (500ms) being larger than
`time_to_burnout` (200ms) — nobody can compile in time:

```
$ ./codexion 3 200 500 200 200 5 0 fifo
0 1 has taken a dongle
1 1 has taken a dongle
1 1 is compiling
200 2 burned out
```
Coder 1 manages to take both dongles and start compiling, but coder 2 is
still waiting for a dongle when its own `time_to_burnout` (200ms) elapses,
so it burns out first and the simulation stops immediately.

The same tight parameters, but with `edf`: the scheduler always wakes the
coder closest to burning out first, which buys enough time for the first
full cycle to complete — though with such a tight margin a later coder can
still burn out once cooldown delays accumulate:

```
$ ./codexion 3 800 200 200 200 5 100 edf
0 1 has taken a dongle
1 1 has taken a dongle
1 1 is compiling
201 1 is debugging
301 2 has taken a dongle
301 2 has taken a dongle
301 2 is compiling
401 1 is refactoring
501 2 is debugging
601 3 has taken a dongle
601 3 has taken a dongle
601 3 is compiling
701 2 is refactoring
801 3 is debugging
801 1 burned out
```
With looser parameters (e.g. a larger `time_to_burnout` or a smaller
`dongle_cooldown`), EDF prevents any burnout entirely, which is the
guarantee the subject asks for when parameters are viable.

The single-coder edge case: only one dongle exists, so the coder takes it
as its "left" dongle but can never acquire the same dongle again as its
"right" one. It blocks until `time_to_burnout` elapses and burns out —
no deadlock, no crash:

```
$ ./codexion 1 800 200 200 200 5 0 fifo
800 1 burned out
```