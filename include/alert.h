/*
 * include/alert.h
 *
 * Alert engine: a per-resource state machine that maps a metric against
 * configurable warning/critical thresholds and suppresses repeated alerts.
 *
 * State machine:
 *   NORMAL --metric >= warning--> WARNING --metric >= critical--> CRITICAL
 *   CRITICAL --metric < warning--> RECOVERY (then back to NORMAL)
 *
 * To avoid alert flooding, a cooldown timer suppresses re-notification of the
 * same severity until 'cooldown_seconds' have elapsed since the last emitted
 * notification. Recovery is always emitted once when the metric returns to
 * normal.
 */

#ifndef LINUX_RESOURCE_MONITOR_ALERT_H
#define LINUX_RESOURCE_MONITOR_ALERT_H

typedef enum {
    ALERT_NORMAL = 0,
    ALERT_WARNING,
    ALERT_CRITICAL
} AlertSeverity;

typedef enum {
    ALERT_EVENT_NONE = 0,      /* no notification this cycle */
    ALERT_EVENT_WARNING,       /* entered warning */
    ALERT_EVENT_CRITICAL,      /* entered critical */
    ALERT_EVENT_RECOVERY       /* recovered to normal */
} AlertEvent;

/*
 * Tracks the state of a single monitored resource (CPU, memory, disk, load).
 */
typedef struct {
    AlertSeverity current;
    AlertSeverity previous;
    double last_notify_time;   /* monotonic seconds of last notification */
    int initialized;
} AlertState;

/*
 * alert_init()
 * Set the state machine to its initial (normal) state.
 */
void alert_init(AlertState *st);

/*
 * alert_evaluate()
 * Evaluate 'value' against thresholds. Returns the AlertEvent to emit this
 * cycle, applying cooldown suppression. The state is updated in place.
 *
 *   value            - current metric
 *   warning_threshold - warning trigger (inclusive)
 *   critical_threshold - critical trigger (inclusive)
 *   cooldown_seconds - minimum seconds between repeated same-severity alerts
 *
 * The current monotonic time is fetched internally via monotonic_seconds().
 */
AlertEvent alert_evaluate(AlertState *st, double value,
                          double warning_threshold, double critical_threshold,
                          double cooldown_seconds);

/*
 * alert_severity_name()
 * Return a stable string for a severity (for display/logging).
 */
const char *alert_severity_name(AlertSeverity s);

#endif /* LINUX_RESOURCE_MONITOR_ALERT_H */
