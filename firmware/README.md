# 🛰️ AVIV Firmware (ESP32)

All embedded code for the robot, organized by function. Sketches are written for the
**ESP32 DevKit V4** and flashed with the **Arduino IDE** (or `arduino-cli`).

> **Before flashing:** open the sketch and set your network at the top:
> ```cpp
> const char* WIFI_SSID = "YOUR_WIFI_SSID";
> const char* WIFI_PASS = "YOUR_WIFI_PASSWORD";
> ```

## Layout

```
firmware/
├── navigation/    Core driving + table-serving logic (the robot's "brain")
├── camera-qr/     FV1 ESP32-CAM QR experiments (deprecated approach)
├── notification/  Receives table notifications from the web app
└── prototypes/    Early throwaway test sketches
```

## `navigation/` — the production navigation stack (FV2 → FV4)

| File | Role | Prototype |
|---|---|---|
| `line_following_forward.ino` | Forward line following with 3 IR sensors + PID | FV1–FV4 |
| `line_following_backward.ino` | Backward line following with dedicated back sensors (no 180° turns) | FV2–FV4 |
| `table_navigation.ino` | Marker-counting table navigation + 5-step serving algorithm | FV4 |
| `table_navigation_test.ino` | Same as above with manual test-order injection (edit `setup()`) | FV4 |
| `imu_turn_fv3.ino` | IMU (MPU-9250 + Madgwick) 90°/180° turns — **abandoned** in FV4 due to motor EMI | FV3 |
| `navigation_simulator.py` | Desktop Python simulator that mirrors the ESP32 serving algorithm exactly — great for testing route logic without hardware | — |
| `restaurant_map.png` | The looped 10-table track layout the algorithm targets | — |

**Tuned PID values (FV4):** `Kp = 45`, `Ki = 0`, `Kd = 30` (same for forward and backward — the robot is symmetric).

**Serving algorithm (5 steps):** directional grouping + a 5-forward / 4-backward rule on a ring track. See [`../docs/ALGORITHMS.md`](../docs/ALGORITHMS.md).

## `camera-qr/` — FV1 vision experiments (deprecated)

The first prototype identified tables by scanning QR codes with an ESP32-CAM. It was
**dropped after FV1** because image processing couldn't keep up with the robot's speed —
replaced by simple, reliable IR marker counting. Kept here for completeness.

| File | Role |
|---|---|
| `esp32cam_qr_server.ino` | ESP32-CAM web server streaming frames for QR detection |
| `esp32cam_wifi_test.ino` | Minimal ESP32-CAM Wi-Fi connectivity test |
| `qr_reader.py` | Desktop Python client that reads QR codes from the camera stream (set `url` to the ESP32-CAM's IP) |

## `notification/`

| File | Role |
|---|---|
| `table_notification.ino` | Runs on an ESP32 that receives table-number notifications over HTTP from the web app (drive an LED, buzzer, or display) |

## `prototypes/`

| File | Role |
|---|---|
| `car_prototype.ino` | Early differential-drive + ultrasonic test platform used to validate basic motion |
