#ifndef __PROGRESS__
#define __PROGRESS__

/**
 * @file
 * @brief Swupd progress bar.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Saves a step to report progress to.
 */
struct step {
	/** @brief Current step. */
	unsigned int current;
	/** @brief Total step. */
	unsigned int total;
	/** @brief Step description . */
	char *description;
};

/**
 * @brief Progress format callback.
 * @see progress_set_format().
 */
typedef void (*progress_fn_t)(char *step_description, unsigned int current_step, unsigned int total_steps, int percentage);

/**
 * @brief Callback to print start header in progress reporting.
 * @see progress_set_format().
 */
typedef void (*start_fn_t)(const char *op);

/**
 * @brief Callback to print start header in progress reporting.
 * @see progress_set_format().
 */
typedef void (*end_fn_t)(const char *op, int status);

/**
 * @brief Sets the format the progress functions should use to
 * print their output.
 */
void progress_set_format(progress_fn_t, start_fn_t, end_fn_t);

/**
 * @brief Initializes a process (a series of steps to complete a given task),
 * this function should be used first so progress in steps is reported
 * correctly.
 */
void progress_init_steps(char *, int);

/**
 * @brief Finishes the reporting of a process (as defined above). If the
 * format used, needs a special closing statement, this function will
 * print it.
 */
void progress_finish_steps(char *, int status);

/**
 * @brief Sets a step of the process to be tracked/reported.
 */
void progress_set_step(unsigned int, char *);

/**
 * @brief Increases the current step by one and assigns the provided description to it.
 *
 * Useful when you don't know the number of the current step.
 */
void progress_set_next_step(char *);

/**
 * @brief Gets the current step defined.
 */
struct step progress_get_step(void);

/**
 * @brief Marks the current step as completed (progress = 100%).
 */
void progress_complete_step(void);

/**
 * @brief It reports the partial progress of the step. Useful in long running steps.
 */
void progress_report(double, double);

#ifdef __cplusplus
}
#endif
#endif
