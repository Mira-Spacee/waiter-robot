# Hardware & Bill of Materials

AVIV's design goal was blunt: **match commercial waiter-robot capability at ~1% of the cost.**
Every component was chosen for the best capability-per-dollar, validated across four prototypes
(FV1–FV4), and upgraded only when testing proved it necessary.

## Subsystems

### Processing & control
| Component | Spec / role | Qty | Used in |
|---|---|---|---|
| **ESP32 DevKit V4** | 240 MHz dual-core, built-in Wi-Fi, many PWM channels & GPIO, 3.3 V logic — runs PID + navigation and talks to the web app directly | 1 | FV1–FV4 |
| **38-pin expansion shield** | Breaks out 5 V / 3.3 V / GND rails for clean multi-sensor wiring | 1 | FV1–FV4 |
| **ESP32-CAM (OV2640)** | On-robot QR/vision for table ID | 1 | FV1 only |
| **External 2.4 GHz Wi-Fi antenna (3 dBi)** | Stronger signal across the floor; mounted in the body | 1 | FV4 |
| **Tablet (11")** | Touchscreen front end running the web app | 1 | FV4 |

### Motor drive
| Component | Spec / role | Qty | Used in |
|---|---|---|---|
| **BTS7960 driver** | MOSFET H-bridge, ~43 A peak, with heatsink — handles high-torque motors under load without overheating | 2 | FV4 |
| **L298N driver** | Dual H-bridge, ~2 A/ch — fine for light prototype motors only (~2 V drop at high current) | 1 | FV1–FV3 |
| **JGB37-555 12 V gear motor** | High torque, 56 RPM, gearbox reduction — safe, stable delivery under payload | 4 | FV4 |
| **Yellow DC gear motor (1:48)** | Lightweight plastic drivetrain for early testing | 4 | FV1–FV3 |
| **All-terrain wheels 130 × 59 mm** | Large diameter + rubber tread: traction, ground clearance, vibration damping (3–10 kg rated) | 4 | FV4 |
| **Yellow plastic wheels** | Small, light — matched the prototype motors | 4 | FV1–FV3 |

### Sensing & feedback
| Component | Spec / role | Qty | Used in |
|---|---|---|---|
| **IR line sensors (TIFSS0154)** | 3.3 V digital, adjustable threshold — line following + table/marker detection | 8 | FV1–FV4 |
| **Ultrasonic HC-SR04** | 2–400 cm range, 40 kHz — obstacle detection (stop/slow) front and back | 6 | FV2–FV4 |
| **HC-020K wheel encoders** | Closed-loop speed feedback so both sides match despite motor variance | 4 | FV4 |
| **3D-printed encoder disk** | Custom 20-slot disk, 12 mm shaft fit | 4 | FV4 |

### Power
| Component | Spec / role | Qty | Used in |
|---|---|---|---|
| **Sealed 12 V 17 Ah battery** | Main energy source — high capacity, low maintenance, stable under load, cost-friendly | 1 | FV4 |
| **3.4 V lithium cells** | Rechargeable power for early prototypes | 3 | FV1–FV3 |
| **DC power switch** | Master ON/OFF rated above stall surge | 1 | FV1–FV4 |
| **Jumper + copper wiring** | Modular sensor links; copper sized for motor current | — | FV1–FV4 |

### Mechanical structure
| Component | Spec / role | Qty | Used in |
|---|---|---|---|
| **CNC laser-cut foam-board body** | 60 × 60 cm, ~10 kg, **30 kg payload**, 4 internal shelves, 11" tablet mount — precise, repeatable, lightweight, eye-catching | 1 | FV3–FV4 |
| **Standard robotics chassis** | 0.645 kg plastic kit body for fast prototype iteration | 1 | FV1–FV3 |

## Cost comparison

The whole point of the project — **a 40–83× cost reduction** driven by replacing LiDAR + SLAM
with IR line-following (≈$16 in sensors vs. ≈$1,200 for LiDAR; $8 ESP32 vs. $400 Intel NUC):

| Criterion | BETA-G (academic) | BellaBot (commercial) | Dawn Avatar | **AVIV** |
|---|---|---|---|---|
| Navigation | SLAM (LiDAR) | SLAM (LiDAR + depth) | Remote human control | **Line-following + markers** |
| Sensors | LiDAR, IMU | LiDAR, depth cam, ultrasonic | Cameras only | **8× IR, 6× ultrasonic, 4× encoders** |
| Autonomy | Full | Full | Human-operated | **Full** |
| Payload | Not documented | 15 kg | ~5 kg | **30 kg** |
| Processing | Intel NUC + ROS | Embedded ARM + Linux | Cloud + operator | **ESP32 @ 240 MHz** |
| Hardware cost | $8,000–$12,000 | $15,000–$25,000 | $20,000+ | **$287** |
| Installation | Included | $2,000–$5,000 | $3,000–$5,000 | **$0** |
| Monthly cost | N/A | $200–$400 | $1,500+ | **$0** |
| 3-year total | ~$10,000 | $30,000–$40,000 | $60,000+ | **$287** |
| Setup time | Hours (mapping) | 2–3 days | 1 day | **Minutes** |
| Success rate | 87% | ~95% | 100% (human) | **98%** |
| Maintenance | ROS expertise | Vendor service | Technical team | **Basic electronics** |

**Trade-off:** AVIV needs line markers on the floor — perfectly acceptable for restaurants
with fixed layouts, which describes most small establishments.

## References

The hardware comparison and design choices draw on:

- Cheong, A., Lau, M. W. S., Foo, E., & Hedley, J. (2016). *Development of a robotic waiter system.* IFAC-PapersOnLine, 49(21), 681–686.
- Asahi Shimbun (2021). *Robot waiters help Japan's restaurants cope with labor shortages.*
- Siegwart, R., Nourbakhsh, I. R., & Scaramuzza, D. (2011). *Introduction to Autonomous Mobile Robots* (2nd ed.). MIT Press.
- Thrun, S., Burgard, W., & Fox, D. (2005). *Probabilistic Robotics.* MIT Press.
- Şahin, F. (2005). *PID controller design for mobile robots.* Turkish J. of Electrical Engineering & CS, 13(1).
