/*
 * src/alert.c
 *
 * Alert state machine implementation.
 */

#include "alert.h"
#include "utils.h"

void alert_init(AlertState *st)
{
    if (st == NULL) {
        return;
    }
    st->current = ALERT_NORMAL;
    st->previous = ALERT_NORMAL;
    st->last_notify_time = 0.0;
    st->initialized = 1;
}

const char *alert_severity_name(AlertSeverity s)
{
    switch (s) {
    case ALERT_NORMAL:   return "NORMAL";
    case ALERT_WARNING:  return "WARNING";
    case ALERT_CRITICAL: return "CRITICAL";
    default:             return "UNKNOWN";
    }
}

AlertEvent alert_evaluate(AlertState *st, double value,
                          double warning_threshold, double critical_threshold,
                          double cooldown_seconds)
{
    AlertSeverity new_severity;
    double now;
    AlertEvent event = ALERT_EVENT_NONE;

    if (st == NULL) {
        return ALERT_EVENT_NONE;
    }
    if (!st->initialized) {
        alert_init(st);
    }

    /* Determine severity from thresholds (critical checked first so a value
     * above critical is not mis-classified as merely warning). */
    if (value >= critical_threshold) {
        new_severity = ALERT_CRITICAL;
    } else if (value >= warning_threshold) {
        new_severity = ALERT_WARNING;
    } else {
        new_severity = ALERT_NORMAL;
    }

    now = monotonic_seconds();

    /* Handle recovery: return to NORMAL from a non-normal state. */
    if (new_severity == ALERT_NORMAL && st->current != ALERT_NORMAL) {
        event = ALERT_EVENT_RECOVERY;
        st->last_notify_time = now;
    }
    /* Handle escalation / initial alert / re-alert after cooldown. */
    else if (new_severity != ALERT_NORMAL) {
        int cooldown_elapsed =
            (now - st->last_notify_time) >= cooldown_seconds;

        /* Notify if severity changed, or cooldown expired (re-alert). */
        if (new_severity != st->current || cooldown_elapsed) {
            if (new_severity == ALERT_CRITICAL) {
                event = ALERT_EVENT_CRITICAL;
            } else {
                event = ALERT_EVENT_WARNING;
            }
            st->last_notify_time = now;
        }
    }

    st->previous = st->current;
    st->current = new_severity;

    return event;
}
