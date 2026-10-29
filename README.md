# Animatronic Pumpkin

Two positional servos on a Seeed XIAO RP2040 sweep about individually
adjustable centers using a 480-entry sine-based position table. For each
half-sweep, normalized position is `p = t - sin(2*PI*t)/(2*PI)`: slow at both
ends and fastest at the center. Position updates occur at uniform intervals
for each servo; endpoint easing comes entirely from the position curve.

A 38-pixel WS2812 strip on D2 glows with animated red/orange embers and
amber/yellow flame tips. Heat rises from pixel 0 (the DIN end) toward pixel 37.
The flame updates every 30 ms without delays, alongside the servo motion.
Install the **Adafruit NeoPixel** library using Arduino Library Manager.

## Adjustments

- `biasOffset = {10, 0}`: horn alignment correction in degrees for servos
  on D0 and D1. For example, `{0, -3}` sets centers to 90° and 87°.
- `sweepAmplitude = {50, 45}`: servo 1 moves ±50° and servo 2 moves ±45°
  around their corrected centers.
- `TABLE_SIZE = 480`: 240 position steps per endpoint-to-endpoint sweep.
  This doubles the previous resolution, approximately halving angular steps.
- `speedOffset = {1.0, 1.15}`: larger values move faster; zero or negative
  values hold the initial position. These configuration values are read at startup.
- `STEP_DELAY_MS = 20`: base table-step interval throughout the sweep.
  At speed 1.0, one half-sweep takes approximately 4.8 seconds and a full
  cycle takes 9.6 seconds. The extra half-speed endpoint timing was removed.
  Increase this for slower motion. Intervals are rounded to whole milliseconds.
  Delayed loop iterations slow the sweep instead of skipping positions.

Flame controls near the top of the sketch:

- `FLAME_BRIGHTNESS = 153`: 60% overall brightness, on a scale of 0 to 255.
- `FLAME_COOLING = 45`: increase for shorter flames; decrease for taller flames.
- `FLAME_SPARKING = 150`: chance out of 256 of new heat each frame.
- `FLAME_INTERVAL_MS = 30`: time between flame frames.

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
including startup/stall demand and the strip current. For servos rated for 5 V:

```text
XIAO RP2040                         Two positional servos
  D0 ----------------------------> Servo 1 signal
  D1 ----------------------------> Servo 2 signal
  D2 --> 5 V logic level shifter
         --> 330-470 ohm resistor -> WS2812 DIN (38 LEDs)

External regulated 5 V + ---------- Servo 1 V+
                          +------- Servo 2 V+
                          +------- WS2812 5V

External supply GND --------------- Servo 1 GND
                          +------- Servo 2 GND
                          +------- WS2812 GND
                          +------- XIAO GND

USB ------------------------------ XIAO power
```

Do not power the servos or strip from the XIAO 3V3 pin. With USB powering the XIAO,
keep the external supply positive connected only to servo and strip power. Grounds
must be shared. Confirm the servo accepts a 3.3 V signal; otherwise use a
suitable signal level shifter. This sketch expects positional servos, not
continuous-rotation servos.

Use a 3.3 V-compatible 5 V logic buffer (such as a 74AHCT125) for WS2812 data.
Place the series resistor near DIN and a 500–1000 µF capacitor across the
strip supply near the first LED. Allow up to 2.3 A for 38 RGB pixels at full
white, plus servo demand. The configured brightness reduces normal draw but
is not a current limiter.

## Validation

Compiled for `rp2040:rp2040:seeed_xiao_rp2040` with Arduino-Pico core 6.1.1
and the installed Adafruit NeoPixel library. Servo 1's sweep is widened to
±50°; servo 2 remains at ±45°. Hardware motion and flame appearance have not
been tested.
