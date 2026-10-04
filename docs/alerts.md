# Alert System

## State machine

Each numeric resource (CPU, memory, disk, load) has an independent state
machine:

```
                    metric >= critical
   +------+   (escalation)    +----------+
   |WARN  | ----------------->| CRITICAL |
   +------+                   +----------+
     ^  ^                        |
     |  | metric >= warning      | metric < warning
     |  +------------------------+ (recovery)
     |                           v
   +--------+  <--- metric < warning   +-----------+
   | NORMAL |                          | (emits    |
   +--------+                          |  RECOVERY)|
```

States: `NORMAL`, `WARNING`, `CRITICAL`. Transitions emit events:
`ALERT_EVENT_WARNING`, `ALERT_EVENT_CRITICAL`, `ALERT_EVENT_RECOVERY`, or
`ALERT_EVENT_NONE` (no notification).

## Avoiding alert flooding

Two mechanisms prevent log flooding:

1. **Transition-only events** — an alert fires when the *state changes*, not
   on every cycle above threshold. Steady-state CRITICAL emits nothing.

2. **Cooldown** — even for a persistent state, a re-alert is emitted only if
   `ALERT_COOLDOWN` seconds (default 300) have elapsed since the last
   notification, measured on the monotonic clock.

## Example

With `CPU_WARNING=70`, `CPU_CRITICAL=90`:

```
cycle 1: 65%  -> NORMAL   (no alert)
cycle 2: 75%  -> WARNING  (alert "exceeded 70%")
cycle 3: 92%  -> CRITICAL (alert "exceeded 90%")
cycle 4: 93%  -> CRITICAL (no alert: same state, within cooldown)
cycle 5: 50%  -> RECOVERY (alert "returned to normal")
```

## Load average vs percentage

Load average thresholds are **absolute** values (e.g. 4.0), not percentages.
The alert messages for load are worded accordingly (no `%` sign) to avoid the
conceptual error of comparing load directly to a percentage.
