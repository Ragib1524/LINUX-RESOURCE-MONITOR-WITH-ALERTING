/*
 * tests/test_alert.c
 *
 * Unit tests for the alert state machine: transitions, recovery, cooldown.
 */

#include <stdio.h>

#include "alert.h"

static int g_failures = 0;

#define CHECK(cond, msg)                                     \
    do {                                                     \
        if (cond) {                                          \
            printf("  PASS: %s\n", msg);                     \
        } else {                                             \
            printf("  FAIL: %s\n", msg);                     \
            g_failures++;                                    \
        }                                                    \
    } while (0)

int main(void)
{
    AlertState st;
    AlertEvent ev;

    printf("test_alert: NORMAL -> WARNING\n");
    alert_init(&st);
    ev = alert_evaluate(&st, 75.0, 70.0, 90.0, 300.0);
    CHECK(ev == ALERT_EVENT_WARNING, "first breach -> WARNING event");
    CHECK(st.current == ALERT_WARNING, "state is WARNING");

    printf("test_alert: WARNING -> CRITICAL\n");
    ev = alert_evaluate(&st, 92.0, 70.0, 90.0, 300.0);
    CHECK(ev == ALERT_EVENT_CRITICAL, "escalation emits CRITICAL");
    CHECK(st.current == ALERT_CRITICAL, "state is CRITICAL");

    printf("test_alert: CRITICAL stays CRITICAL (no repeat)\n");
    ev = alert_evaluate(&st, 93.0, 70.0, 90.0, 300.0);
    CHECK(ev == ALERT_EVENT_NONE, "no repeat within cooldown");
    CHECK(st.current == ALERT_CRITICAL, "still CRITICAL");

    printf("test_alert: CRITICAL -> NORMAL (recovery)\n");
    ev = alert_evaluate(&st, 50.0, 70.0, 90.0, 300.0);
    CHECK(ev == ALERT_EVENT_RECOVERY, "recovery emitted");
    CHECK(st.current == ALERT_NORMAL, "state is NORMAL");

    printf("test_alert: cooldown allows re-alert\n");
    alert_init(&st);
    /* Zero cooldown => every exceeding cycle notifies. */
    alert_evaluate(&st, 80.0, 70.0, 90.0, 0.0);
    ev = alert_evaluate(&st, 81.0, 70.0, 90.0, 0.0);
    CHECK(ev == ALERT_EVENT_WARNING, "zero cooldown re-alerts");

    printf("test_alert: critical checked before warning\n");
    alert_init(&st);
    ev = alert_evaluate(&st, 99.0, 70.0, 90.0, 300.0);
    CHECK(ev == ALERT_EVENT_CRITICAL, "99 -> CRITICAL not WARNING");

    printf("test_alert: NULL safe\n");
    CHECK(alert_evaluate(NULL, 99.0, 70.0, 90.0, 300.0) == ALERT_EVENT_NONE,
          "NULL state -> NONE");

    printf("\n%d failure(s)\n", g_failures);
    return g_failures == 0 ? 0 : 1;
}
