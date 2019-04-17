#ifndef __PROGRESS__
#define __PROGRESS__

#ifdef __cplusplus
extern "C" {
#endif

struct step {
	unsigned int current;
	unsigned int total;
	char *description;
};

typedef void (*progress_fn_t)(char *step_description, unsigned int current_step, unsigned int total_steps, int percentage);
typedef void (*start_fn_t)(const char *op);
typedef void (*end_fn_t)(const char *op, int status);

/*
 * Sets the format the progress functions should use to
 * print their output.
 */
void progress_set_format(progress_fn_t, start_fn_t, end_fn_t);

/*
 * Initializes a process (a series of steps to complete a given task),
 * this function should be used first so progress in steps is reported
 * correctly.
 */
void progress_init_steps(char *, int);

/*
 * Finishes the reporting of a process (as defined above). If the
 * format used, needs a special closing statement, this function will
 * print it.
 */
void progress_finish_steps(char *, int status);

/*
 * Sets a step of the process to be tracked/reported.
 */
void progress_set_step(unsigned int, char *);

/*
 * Increases the current step by one and assigns the provided description to it.
 * Useful when you don't know the number of the current step.
 */
void progress_set_next_step(char *);

/*
 * Gets the current step defined.
 */
struct step progress_get_step(void);

/*
 * Marks the current step as completed (progress = 100%).
 */
void progress_complete_step(void);

/*
 * It reports the partial progress of the step. Useful in long running steps.
 */
void progress_report(double, double);

#ifdef __cplusplus
}
#endif
#endif
