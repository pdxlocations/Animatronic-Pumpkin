# Animatronic Pumpkin

Three positional servos on a Seeed XIAO RP2040 sweep about individually
adjustable centers using a 480-entry sine-based position table. For each
half-sweep, normalized position is `p = t - sin(2*PI*t)/(2*PI)`: slow at both
ends and fastest at the center. Position updates occur at uniform intervals
for each servo; endpoint easing comes entirely from the position curve.

## Adjustments

- `biasOffset = {20, 0, 0}`: horn alignment correction in degrees for servos
  on D0, D1, and D2. For example, `{0, -3, 2}` sets centers to 90°, 87°, 92°.
- `SWEEP_AMPLITUDE = 45`: each servo moves ±45° around its corrected center.
- `TABLE_SIZE = 480`: 240 position steps per endpoint-to-endpoint sweep.
  This doubles the previous resolution, approximately halving angular steps.
- `speedOffset = {1.0, 1.15, 0.85}`: larger values move faster; zero or negative
  values hold the initial position. These configuration values are read at startup.
- `STEP_DELAY_MS = 20`: base table-step interval throughout the sweep.
  At speed 1.0, one half-sweep takes approximately 4.8 seconds and a full
  cycle takes 9.6 seconds. The extra half-speed endpoint timing was removed.
  Increase this for slower motion. Intervals are rounded to whole milliseconds.
  Delayed loop iterations slow the sweep instead of skipping positions.

Centers are limited so the entire sweep stays inside the commanded 0–180°
range. Check the linkage's actual travel before increasing amplitude or bias.
The first command moves to the lower endpoint; movement from an unknown
power-on position is not eased because this sketch has no position feedback.

The calculated angle is mapped to `writeMicroseconds()` commands, rounding
only the final pulse width. The explicit 1000–2000 microsecond mapping matches
the previously installed Arduino-Pico 6.1.1 defaults to preserve alignment.
Actual angles and motion depend on the servo and linkage. Pulse quantization,
servo deadband, and gear backlash can still affect visible smoothness.

## Wiring schematic

Use a regulated supply rated for the servos' voltage and combined current,
including startup/stall demand. For servos rated for 5 V:

```text
XIAO RP2040                         Three positional servos
  D0 ----------------------------> Servo 1 signal
  D1 ----------------------------> Servo 2 signal
  D2 ----------------------------> Servo 3 signal

External regulated 5 V + ---------- Servo 1 V+
                          +------- Servo 2 V+
                          +------- Servo 3 V+

External supply GND --------------- Servo 1 GND
                          +------- Servo 2 GND
                          +------- Servo 3 GND
                          +------- XIAO GND

USB ------------------------------ XIAO power
```

Do not power the servos from the XIAO 3V3 pin. With USB powering the XIAO,
keep the external supply positive connected only to servo power. Grounds
must be shared. Confirm the servo accepts a 3.3 V signal; otherwise use a
suitable signal level shifter. This sketch expects positional servos, not
continuous-rotation servos.

## Validation

Compiled for `rp2040:rp2040:seeed_xiao_rp2040` with Arduino-Pico core 6.1.1.
Numerical checks verified the finer position steps, unchanged underlying
curve, symmetric reversals, and biased pulse bounds. The maximum angular step
is approximately 0.75 degrees, compared with 1.50 degrees at 240 samples.
Hardware motion has not been tested.
