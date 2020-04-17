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
	/** @brief Temetry severity DEBUG */
	TELEMETRY_DEBG = 1,
	/** @brief Temetry severity INFO */
	TELEMETRY_INFO,
	/** @brief Temetry severity WARN */
	TELEMETRY_WARN,
	/** @brief Temetry severity CRIT */
	TELEMETRY_CRIT
};

/**
 * @brief Create a telemetry record to be reported if telemetrics is enabled.
 */
void telemetry(enum telemetry_severity level, const char *class, const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif
