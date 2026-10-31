/*
 * SpinPOV - persistence-of-vision display firmware
 * ESP32 DevKit (30-pin) + APA102/SK9822 LED strip + A3144 Hall sensor
 *
 * Wiring (see the KiCad schematic):
 *   LED data  (DI) -> GPIO23 (SPI MOSI)
 *   LED clock (CI) -> GPIO18 (SPI SCK)
 *   Hall sensor OUT -> GPIO27 (internal pull-up, output goes LOW at the magnet)
 *
 * How it works:
 *   The Hall sensor gives one pulse per revolution. The time between two pulses
 *   is the revolution period. In loop() the time since the last pulse is turned
 *   into a column number (0..COLS-1) and that column is sent to the strip.
 *   Because the position is calculated from the real time, the motor speed does
 *   not have to be exact - only reasonably even.
 *
 * STATUS: written in week 4, NOT TESTED ON HARDWARE YET (build is in week 9).
 */

#include <Arduino.h>
#include <SPI.h>

// ------------------------------------------------------------------ settings

const int PIN_SCK  = 18;
const int PIN_MOSI = 23;
const int PIN_HALL = 27;

const uint32_t SPI_HZ  = 4000000;  // 4 MHz: one frame (72 bytes) takes about 144 us
const int      NUM_LEDS = 16;      // LEDs used on the arm
const int      COLS     = 120;     // columns (angular slices) per revolution

// APA102 global brightness, 0..31. Start LOW (4) when testing on the battery.
const uint8_t BRIGHTNESS = 4;

// Tuning the picture:
const int  COLUMN_OFFSET     = 0;      // rotate the picture (0..COLS-1 columns)
const bool REVERSE_DIRECTION = false;  // set true if the picture is mirrored
const bool FLIP_ROWS         = false;  // set true if LED 0 is at the tip, not near the shaft

// Hall pulses closer together than this are ignored (noise). 5000 us = 12000 rpm max.
const uint32_t MIN_PULSE_GAP_US = 5000;

// Which picture to show, and automatic switching (0 = no switching).
enum Pattern { PAT_TEST, PAT_RAINBOW, PAT_RINGS, PAT_COUNT };
Pattern currentPattern = PAT_TEST;
const uint32_t AUTO_CYCLE_MS = 0;

// Print rpm and pulse count on the serial port every 500 ms (for bench tests).
const bool DEBUG_SERIAL = true;

// ------------------------------------------------------------------- frames
// One APA102 frame = 4 bytes start + 4 bytes per LED + 4 bytes end.
// All columns are prepared in advance, so loop() only has to send bytes.

const int FRAME_BYTES = 4 + NUM_LEDS * 4 + 4;
uint8_t frames[COLS][FRAME_BYTES];
uint8_t blankFrame[FRAME_BYTES];

void initFrame(uint8_t *f) {
  for (int i = 0; i < 4; i++) f[i] = 0x00;                     // start frame
  for (int led = 0; led < NUM_LEDS; led++) {
    int o = 4 + led * 4;
    f[o]     = 0xE0 | BRIGHTNESS;                              // 111 + brightness
    f[o + 1] = 0; f[o + 2] = 0; f[o + 3] = 0;                  // B, G, R = off
  }
  for (int i = 0; i < 4; i++) f[4 + NUM_LEDS * 4 + i] = 0xFF;  // end frame
}

void clearFrames() {
  for (int c = 0; c < COLS; c++) initFrame(frames[c]);
}

// row 0 = the LED closest to the shaft (unless FLIP_ROWS is set)
void setPixel(int col, int row, uint8_t r, uint8_t g, uint8_t b) {
  if (col < 0 || col >= COLS || row < 0 || row >= NUM_LEDS) return;
  int led = FLIP_ROWS ? (NUM_LEDS - 1 - row) : row;
  int o = 4 + led * 4;
  frames[col][o + 1] = b;  // APA102 order is B, G, R
  frames[col][o + 2] = g;
  frames[col][o + 3] = r;
}

// Hue 0..255 -> full-colour RGB
void hueToRgb(uint8_t h, uint8_t &r, uint8_t &g, uint8_t &b) {
  uint8_t region = h / 43;
  uint8_t rem = (h - region * 43) * 6;  // 0..252
  uint8_t q = 255 - rem;
  switch (region) {
    case 0:  r = 255; g = rem; b = 0;   break;
    case 1:  r = q;   g = 255; b = 0;   break;
    case 2:  r = 0;   g = 255; b = rem; break;
    case 3:  r = 0;   g = q;   b = 255; break;
    case 4:  r = rem; g = 0;   b = 255; break;
    default: r = 255; g = 0;   b = q;   break;
  }
}

// ----------------------------------------------------------------- patterns

// Test picture for the first run:
//   - white line at column 0 (this is where the magnet is)
//   - red line at 1/4, green at 1/2, blue at 3/4 of the revolution
//   - dim white ring made by LED 0 (shows which end of the strip is the centre)
void buildTest() {
  clearFrames();
  for (int c = 0; c < COLS; c++) setPixel(c, 0, 40, 40, 40);
  for (int r = 0; r < NUM_LEDS; r++) {
    setPixel(0,            r, 255, 255, 255);
    setPixel(COLS / 4,     r, 255, 0,   0);
    setPixel(COLS / 2,     r, 0,   255, 0);
    setPixel(3 * COLS / 4, r, 0,   0,   255);
  }
}

// Rainbow that changes with the angle
void buildRainbow() {
  clearFrames();
  for (int c = 0; c < COLS; c++) {
    uint8_t r, g, b;
    hueToRgb((uint8_t)(c * 256 / COLS), r, g, b);
    for (int row = 0; row < NUM_LEDS; row++) setPixel(c, row, r, g, b);
  }
}

// Concentric rings, every ring (LED) has its own colour
void buildRings() {
  clearFrames();
  for (int row = 0; row < NUM_LEDS; row++) {
    uint8_t r, g, b;
    hueToRgb((uint8_t)(row * 256 / NUM_LEDS), r, g, b);
    for (int c = 0; c < COLS; c++) setPixel(c, row, r, g, b);
  }
}

void buildPattern() {
  switch (currentPattern) {
    case PAT_TEST:    buildTest();    break;
    case PAT_RAINBOW: buildRainbow(); break;
    case PAT_RINGS:   buildRings();   break;
    default:          buildTest();    break;
  }
}

// ------------------------------------------------------------- Hall sensor

volatile uint32_t lastPulseUs = 0;  // time of the last magnet pass
volatile uint32_t periodUs    = 0;  // time of the last full revolution
volatile uint32_t pulseCount  = 0;

void IRAM_ATTR onHall() {
  uint32_t now = micros();
  uint32_t dt  = now - lastPulseUs;
  if (pulseCount > 0 && dt < MIN_PULSE_GAP_US) return;  // noise
  if (pulseCount > 0) periodUs = dt;
  lastPulseUs = now;
  pulseCount++;
}

// ------------------------------------------------------------------- setup

void setup() {
  if (DEBUG_SERIAL) Serial.begin(115200);

  initFrame(blankFrame);
  buildPattern();

  pinMode(PIN_HALL, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_HALL), onHall, FALLING);

  // The SPI port is used only for the LEDs, so the transaction stays open.
  SPI.begin(PIN_SCK, -1, PIN_MOSI, -1);
  SPI.beginTransaction(SPISettings(SPI_HZ, MSBFIRST, SPI_MODE0));
  SPI.writeBytes(blankFrame, FRAME_BYTES);  // all LEDs off at start
}

// -------------------------------------------------------------------- loop

int      lastIdx      = -2;
uint32_t lastDebugMs  = 0;
uint32_t lastCycleMs  = 0;

void loop() {
  // Read the values from the interrupt as one consistent set.
  noInterrupts();
  uint32_t last   = lastPulseUs;
  uint32_t period = periodUs;
  uint32_t count  = pulseCount;
  interrupts();

  uint32_t elapsed = micros() - last;

  if (count >= 2 && period > 0 && elapsed < 2 * period) {
    // Spinning: which column is under the strip right now?
    uint32_t col = (uint32_t)(((uint64_t)elapsed * COLS) / period);
    if (col >= COLS) col = COLS - 1;

    int c = REVERSE_DIRECTION ? (COLS - 1 - (int)col) : (int)col;
    int idx = (c + COLUMN_OFFSET) % COLS;

    if (idx != lastIdx) {            // send each column only once
      SPI.writeBytes(frames[idx], FRAME_BYTES);
      lastIdx = idx;
    }
  } else if (lastIdx != -1) {
    // Not spinning (or the magnet signal was lost): switch the LEDs off.
    SPI.writeBytes(blankFrame, FRAME_BYTES);
    lastIdx = -1;
  }

  uint32_t ms = millis();

  if (AUTO_CYCLE_MS > 0 && ms - lastCycleMs >= AUTO_CYCLE_MS) {
    lastCycleMs = ms;
    currentPattern = (Pattern)((currentPattern + 1) % PAT_COUNT);
    buildPattern();
  }

  if (DEBUG_SERIAL && ms - lastDebugMs >= 500) {
    lastDebugMs = ms;
    if (count >= 2 && period > 0 && elapsed < 2 * period) {
      Serial.printf("rpm=%.0f pulses=%lu\n", 60000000.0 / period, (unsigned long)count);
    } else {
      Serial.printf("not spinning, pulses=%lu\n", (unsigned long)count);
    }
  }
}
