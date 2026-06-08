# Control Algorithms & Prototype Evolution

AVIV runs three custom-tuned algorithms on the ESP32: **PID** keeps it on the line,
an **S-curve motion profile** keeps the payload from spilling, and a **5-step serving
algorithm** plans the most efficient multi-table route. This document also walks through how
the design evolved across four prototypes.

---

## 1. PID line following

A PID (Proportional–Integral–Derivative) controller continuously corrects the robot's
position by minimizing the error between where it is and the center of the line.

```
correction = Kp·e(t) + Ki·∫e(t)dt + Kd·de(t)/dt
```

**Why it's needed:** without feedback, tiny steering errors accumulate until the robot
drifts off the line and misses turns. PID makes the response smooth *and* sharp.

**Controller comparison**

| Controller | Steady-state error | Overshoot | Settling time | Noise sensitivity |
|---|---|---|---|---|
| Open loop | Large | None | N/A | None |
| P only | Some | Low | Fast | Low |
| PI | Zero | Moderate | Moderate | Low |
| PD | Some | Low | Fast | High |
| **PID** | **Zero** | **Tunable** | **Tunable** | **Moderate** |

**Tuned values (FV4):** `Kp = 45`, `Ki = 0`, `Kd = 30`, applied to both forward and backward
arrays (the robot is mechanically symmetric, so one tuning serves both directions). Result:
**±6 cm tracking accuracy.**

---

## 2. S-curve motion profile

When a robot changes speed, three quantities move together: **velocity**, **acceleration**,
and **jerk** (the rate of change of acceleration). Sudden jerk is what makes drinks spill and
wheels slip.

| Profile | Jerk | Shock | Payload disturbance | Complexity |
|---|---|---|---|---|
| Step (abrupt) | Infinite | Severe | Severe | Trivial |
| Trapezoidal | Spikes at corners | Moderate | Moderate | Low |
| **S-curve** | **Bounded** | **Minimal** | **Minimal** | Moderate |

The S-curve ramps acceleration smoothly so jerk stays bounded, producing gentle starts and
stops. It's applied when **decelerating to a stop** (spill prevention), **accelerating from
rest** (anti-slip), and during **obstacle slow-downs**. The S-curve generates a smooth velocity
**setpoint**; PID then makes the motors follow it.

> **Result:** 0 spills across 50 test deliveries, vs. ~40% spill rate with abrupt (step) speed changes.

**FV2 obstacle-avoidance parameters:** deceleration zone 60 cm · dead zone 15 cm · sigmoid
steepness `k = 12` · stop duration 6 s (later reduced for production).

---

## 3. Table-serving algorithm (5 steps)

A custom planner for a **looped, linear 10-table track**: tables 1–5 are reached by driving
**forward** from the staff station, tables 6–10 by driving **backward**, and the track wraps
around (T10 ↔ T1).

**Single order:** table 1–5 → go forward; table 6–10 → go backward. Done.

**Multiple orders (5 steps):**
1. **Detect** more than one pending table.
2. **Count & choose side** — tally orders on the RIGHT (1–5) vs LEFT (6–10); start with the
   side that has more (tie → LEFT).
3. **Serve opportunistically** — travel to the farthest table on that side, serving every
   ordered table you pass (no skipping, no zig-zag).
4. **Switch sides with the 5F/4B rule** — to reach remaining tables, compare forward (max 5
   steps) vs backward (max 4 steps, wrap-around allowed) and take the shorter path (tie → backward).
5. **Return** — last table 1–5 → return backward; last table 6–10 → return forward.

**Worked example — orders at T3, T4, T8**
- Step 2: RIGHT has 2 (T3, T4), LEFT has 1 (T8) → start RIGHT.
- Step 3: go forward, serve T3, then T4.
- Step 4: from T4, forward to T8 = 4 steps vs backward (wrap) = 6 → go **forward**.
- Step 5: last table T8 is LEFT → return forward to staff.

**Why it works:** directional grouping avoids back-and-forth, opportunistic serving never
skips a table on the way, the 5F/4B rule bounds switching cost, and wrap-around uses the
circular track efficiently — all with simple counting, no heavy pathfinding.

A desktop simulator that mirrors this logic exactly lives at
[`../firmware/navigation/navigation_simulator.py`](../firmware/navigation/navigation_simulator.py).

---

## Prototype evolution

The final design is the product of four iterations, each driven by the failures of the last.

### FV1 — First prototype
- **Approach:** branched 3-path map; ESP32-CAM scans QR codes to identify tables; basic 2-IR
  line following, no PID.
- **Problems:** prototype motors too weak; jerky movement (no PID); missed T-junctions; and
  critically, **QR scanning was too slow** to keep up with the robot's motion.
- **Lesson:** drop the camera; use industrial motors, PID, and more sensors.

### FV2 — IR markers + PID + S-curve
- **Changes:** linear loop track (T1→T10); IR **marker counting** instead of QR; the
  Directional-Grouping + 5F/4B navigation algorithm; PID line following (3 sensors); S-curve
  obstacle avoidance; **dedicated back sensors** for backward driving (no 180° turns).
- **Problems:** yellow motors still lacked torque; L298N voltage drop; flimsy chassis;
  open-loop motors had no speed feedback.
- **Lesson:** move to high-torque motors, a MOSFET driver (BTS7960), encoders, and a rigid body.

### FV3 — IMU emergency escape
- **Changes:** added an **emergency-escape maneuver** (IMU-based 90° turns, Madgwick filter,
  9-axis fusion) so the robot could bypass a persistent obstacle instead of waiting forever.
- **Problems:** **motor EMI corrupted the magnetometer**, causing IMU drift; 90° turns
  overshot and varied with battery voltage; the robot often lost the line after escaping.
- **Lesson:** the IMU and high-current motors are fundamentally incompatible here — drop it.

### FV4 — Production-ready ✅
- **Decisions:** remove the IMU entirely and use **linear-only navigation** (sufficient for a
  fixed restaurant layout); **wait** for temporary obstacles (people) instead of complex
  escape maneuvers; add **encoder feedback** for matched wheel speeds; identical PID for
  forward/backward.
- **Hardware upgrades:** BTS7960 (43 A, 21× the L298N's capacity) · 4× HC-020K encoders ·
  6 ultrasonic (3 front + 3 back) · 8 IR (3 front + 3 back + 2 markers) · 60 × 60 cm platform ·
  130 mm all-terrain wheels.
- **State machine:** `IDLE → NAVIGATING → OBSTACLE_BLOCKED → AT_TABLE → RETURNING`.

| Problem (FV3 → FV4) | Root cause | Solution | Result |
|---|---|---|---|
| L298N overheating | 2 A limit exceeded by larger motors | BTS7960 (43 A) | Motors run cool under load |
| IMU drift | EMI from high-current driver | Removed IMU entirely | Linear nav — 100% reliable |
| Inconsistent wheel speed | Open-loop PWM | HC-020K encoders + PID | Wheel speeds matched ±5% |
| Complex 180° turns failing | IMU errors + traction | Eliminated turns | Simpler, more reliable |

---

## Final performance

| Metric | Result |
|---|---|
| Navigation success rate | **98%** |
| Path tracking accuracy | **±6 cm** |
| Obstacle detection | **100%** |
| Spills (S-curve, 50 deliveries) | **0** |
| Wheel-speed match (encoders) | **±5%** |
