#ifndef __TELEMETRY__
#define __TELEMETRY__

/**
 * @file
 * @brief Module to handle telemetry report entries
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Severity of a telemetry record
 */
enum telemetry_severity {
	/** @brief Temetry severity Low */
	TELEMETRY_LOW = 1,
	/** @brief Temetry severity Medium */
	TELEMETRY_MED = 2,
	/** @brief Temetry severity High */
	TELEMETRY_HIGH = 3,
	/** @brief Temetry severity Critical */
	TELEMETRY_CRIT = 4,
};

/**
 * @brief Disables telemetry reports.
 */
void telemetry_disable(void);

/**
 * @brief Create a telemetry record to be reported if telemetrics is enabled.
 */
void telemetry(enum telemetry_severity level, const char *class, const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif
