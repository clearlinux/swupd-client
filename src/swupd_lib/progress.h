#ifndef __PROGRESS__
#define __PROGRESS__

/**
 * @file
 * @brief Swupd progress bar.
 */

#include "swupd_lib/swupd_curl.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief The progress report type.
 */
enum progress_type {
	/** @brief Progress is undefined so a spinner should be displayed if possible */
	PROGRESS_UNDEFINED,
	/** @brief Progress is defined so a progress bar can be used */
	PROGRESS_BAR,
};

/**
 * @brief Progress format callback.
 * @see progress_set_format().
 */
typedef void (*progress_fn_t)(const char *step_description, int current_step, int total_steps, int percentage);

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
void progress_init_steps(const char *title, int steps_in_process);

/**
 * @brief Finishes the reporting of a process (as defined above). If the
 * format used, needs a special closing statement, this function will
 * print it.
 */
void progress_finish_steps(int status);

/**
 * @brief Increases the current step by one and assigns the provided description to it.
 * @param step_title The title of this step
 * @param type The type of the step progress (undefined or bar).
 */
void progress_next_step(const char *desc, enum progress_type type);

/**
 * @brief Report the current progress of the step.
 * @param count The number of units processed
 * @param max The total units to process.
 *
 * So the progress in percentage will be count/max.
 */
void progress_report(double count, double max);

/**
 * @brief Enable or diable the progress bar.
 * @param enable If the progress bar should be displayed or not.
 *
 * Default value is enabled.
 */
void progress_set_enabled(bool enabled);

/**
 * @brief Cleans a spinner from the screen.
 */
void clean_spinner(void);

#ifdef __cplusplus
}
#endif
#endif
