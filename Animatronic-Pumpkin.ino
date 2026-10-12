#include <Servo.h>
#include <math.h>

// ---------- Configuration ----------
#define NUM_SERVOS      3
#define SERVO_CENTER    90    // degrees (center position)
#define SWEEP_AMPLITUDE 45    // +/- degrees around the biased center
#define TABLE_SIZE      480   // position samples per full back-and-forth cycle
#define STEP_DELAY_MS   20    // 240 steps x 20 ms = 4.8 seconds per half-sweep

static_assert(TABLE_SIZE > 0 && TABLE_SIZE % 2 == 0, "TABLE_SIZE must be positive and even");

// Match the installed Arduino-Pico Servo defaults to preserve alignment.
const int SERVO_MIN_US = 1000;
const int SERVO_MAX_US = 2000;

// Pins for XIAO RP2040 (adjust as needed for your wiring)
const uint8_t servoPins[NUM_SERVOS] = {D0, D1, D2};

// Horn alignment in degrees: positive shifts the entire sweep upward.
const int biasOffset[NUM_SERVOS] = {10, 0, 0};

// Speed multiplier per servo: >1 faster, <1 slower, <=0 holds position.
const float speedOffset[NUM_SERVOS] = {1.0, 1.15, 0.85};

Servo servos[NUM_SERVOS];

float sineTable[TABLE_SIZE];
uint16_t phaseIndex[NUM_SERVOS] = {0, 0, 0};
unsigned long stepInterval[NUM_SERVOS] = {0, 0, 0};

// Track last update time per servo (non-blocking timing)
unsigned long lastUpdate[NUM_SERVOS] = {0, 0, 0};

void buildSineTable() {
  const int halfSize = TABLE_SIZE / 2;
  for (int i = 0; i < TABLE_SIZE; i++) {
    float t = (float)(i % halfSize) / halfSize;
    // Symmetric sine-based profile: slow ends, fastest at center.
    float progress = t - sin(2.0f * PI * t) / (2.0f * PI);
    float offset = SWEEP_AMPLITUDE * (2.0f * progress - 1.0f);
    sineTable[i] = (i < halfSize) ? offset : -offset;
  }
}

unsigned long intervalForServo(int i) {
  if (speedOffset[i] <= 0.0f) return 0;
  unsigned long interval = (unsigned long)lroundf(
      STEP_DELAY_MS / speedOffset[i]);
  return interval < 1 ? 1 : interval;
}

void writePosition(int i) {
  int center = constrain(SERVO_CENTER + biasOffset[i],
                         SWEEP_AMPLITUDE, 180 - SWEEP_AMPLITUDE);
  float angle = center + sineTable[phaseIndex[i]];
  int pulseUs = lroundf(SERVO_MIN_US +
                       angle * (SERVO_MAX_US - SERVO_MIN_US) / 180.0f);
  servos[i].writeMicroseconds(pulseUs);
}

void setup() {
  buildSineTable();
  for (int i = 0; i < NUM_SERVOS; i++) {
    servos[i].attach(servoPins[i], SERVO_MIN_US, SERVO_MAX_US);
    writePosition(i);
    stepInterval[i] = intervalForServo(i);
    lastUpdate[i] = millis();
  }
}

void loop() {
  unsigned long now = millis();

  for (int i = 0; i < NUM_SERVOS; i++) {
    unsigned long interval = stepInterval[i];
    if (interval == 0) continue;

    if (now - lastUpdate[i] >= interval) {
      lastUpdate[i] = now;
      phaseIndex[i] = (phaseIndex[i] + 1) % TABLE_SIZE;
      writePosition(i);
    }
  }
}
