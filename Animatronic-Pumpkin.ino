#include <Servo.h>
#include <Adafruit_NeoPixel.h>
#include <math.h>

// ---------- Configuration ----------
#define NUM_SERVOS      2
#define SERVO_CENTER    90    // degrees (center position)
#define TABLE_SIZE      480   // position samples per full back-and-forth cycle
#define STEP_DELAY_MS   20    // 240 steps x 20 ms = 4.8 seconds per half-sweep

static_assert(TABLE_SIZE > 0 && TABLE_SIZE % 2 == 0, "TABLE_SIZE must be positive and even");

// Match the installed Arduino-Pico Servo defaults to preserve alignment.
const int SERVO_MIN_US = 1000;
const int SERVO_MAX_US = 2000;

// Pins for XIAO RP2040 (adjust as needed for your wiring)
const uint8_t servoPins[NUM_SERVOS] = {D0, D1};
const int sweepAmplitude[NUM_SERVOS] = {80, 45}; // +/- degrees per servo

const uint8_t FLAME_PIN = D2;
const uint16_t NUM_LEDS = 38;
const uint8_t FLAME_BRIGHTNESS = 50; // (0-255)
const unsigned long FLAME_INTERVAL_MS = 30;
const uint8_t FLAME_COOLING = 45;     // Higher values make shorter flames
const uint8_t FLAME_SPARKING = 50;   // 0-255, chance of a new spark per frame
Adafruit_NeoPixel strip(NUM_LEDS, FLAME_PIN, NEO_GRB + NEO_KHZ800);
uint8_t heat[NUM_LEDS] = {};
unsigned long lastFlameUpdate = 0;

// Horn alignment in degrees: positive shifts the entire sweep upward.
const int biasOffset[NUM_SERVOS] = {25, 0};

// Speed multiplier per servo: >1 faster, <1 slower, <=0 holds position.
const float speedOffset[NUM_SERVOS] = {2.0, 1.15};

Servo servos[NUM_SERVOS];

float sineTable[TABLE_SIZE];
uint16_t phaseIndex[NUM_SERVOS] = {};
unsigned long stepInterval[NUM_SERVOS] = {};

// Track last update time per servo (non-blocking timing)
unsigned long lastUpdate[NUM_SERVOS] = {};

void updateFlame(unsigned long now) {
  if (now - lastFlameUpdate < FLAME_INTERVAL_MS) return;
  lastFlameUpdate = now;

  // Cool each cell, then carry heat toward the far end of the strip.
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    int cooling = random(0, (FLAME_COOLING * 10 / NUM_LEDS) + 2);
    heat[i] = heat[i] > cooling ? heat[i] - cooling : 0;
  }
  for (int i = NUM_LEDS - 1; i >= 2; i--) {
    heat[i] = (heat[i - 1] + 2 * heat[i - 2]) / 3;
  }
  if (random(256) < FLAME_SPARKING) {
    uint8_t spark = random(7);
    int temperature = heat[spark] + random(160, 256);
    heat[spark] = temperature > 255 ? 255 : temperature;
  }

  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    // Keep a dim ember glow even between sparks. Hot areas turn amber/yellow.
    uint8_t temperature = 40 + (uint16_t(heat[i]) * 215 / 255);
    uint8_t ramp = (temperature & 0x3F) * 4;
    if (temperature < 64) {
      strip.setPixelColor(i, ramp, 0, 0);
    } else if (temperature < 128) {
      strip.setPixelColor(i, 255, ramp, 0);
    } else if (temperature < 192) {
      strip.setPixelColor(i, 255, 255, ramp / 8);
    } else {
      strip.setPixelColor(i, 255, 255, 32 + ramp / 4);
    }
  }
  strip.show();
}

void buildSineTable() {
  const int halfSize = TABLE_SIZE / 2;
  for (int i = 0; i < TABLE_SIZE; i++) {
    float t = (float)(i % halfSize) / halfSize;
    // Symmetric sine-based profile: slow ends, fastest at center.
    float progress = t - sin(2.0f * PI * t) / (2.0f * PI);
    float offset = 2.0f * progress - 1.0f;
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
  int amplitude = constrain(sweepAmplitude[i], 0, 90);
  int center = constrain(SERVO_CENTER + biasOffset[i],
                         amplitude, 180 - amplitude);
  float angle = center + amplitude * sineTable[phaseIndex[i]];
  int pulseUs = lroundf(SERVO_MIN_US +
                       angle * (SERVO_MAX_US - SERVO_MIN_US) / 180.0f);
  servos[i].writeMicroseconds(pulseUs);
}

void setup() {
  strip.begin();
  strip.setBrightness(FLAME_BRIGHTNESS);
  // Start warm, so the pumpkin glows immediately while the flame develops.
  for (uint16_t i = 0; i < NUM_LEDS; i++) heat[i] = 100;
  lastFlameUpdate = millis() - FLAME_INTERVAL_MS;
  updateFlame(millis());
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
  updateFlame(now);
}
