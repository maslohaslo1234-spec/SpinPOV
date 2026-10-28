# SpinPOV

A persistence-of-vision (POV) display. A thin arm with 16 RGB LEDs spins around a motor shaft, and an ESP32 changes the LEDs very fast so that the eye sees a full round image floating in the air.

**Status:** design phase (Hack Club Half-Life, week 4, Design with displays). The schematic is done and the CAD is in progress. The build is planned for week 9.

## How it works

1. A small brushed DC motor spins the arm at roughly 1200-2400 RPM (20-40 revolutions per second).
2. A magnet is fixed to the stationary base. Every time the arm passes it, a Hall sensor (A3144) on the arm gives a pulse, so the ESP32 knows when a new revolution starts.
3. The ESP32 measures the time of the last revolution and divides it into 120 equal slices (columns of the image).
4. For each slice it sends one column of 16 pixels to the APA102 LED strip over SPI. APA102 LEDs have a separate clock line, so the timing stays stable even at this speed.
5. Because the arm spins faster than the eye can follow, the columns blend into one round image.

Since the speed is measured on every revolution, the motor does not need exact speed control. It only needs to spin reasonably evenly.

## Hardware

Everything on the rotor is kept as small as possible. The battery is taken off and charged externally, so there is no charger on the arm.

| Part | Purpose |
|---|---|
| ESP32 DevKit (30-pin) | Brain, SPI output to the LEDs, reads the Hall sensor |
| 16x APA102 RGB LEDs (strip) | The "pixels" of the display |
| A3144 Hall sensor + small magnet | Detects the start of each revolution |
| 1S LiPo battery + switch | Power for the rotor (also acts as a counterweight) |
| MT3608 boost module | Raises the battery voltage to 5 V for the ESP32 (VIN) and the LEDs |
| 470 uF + 100 nF capacitors | Smooth the LED current peaks and the Hall sensor supply |
| Brushed DC motor | Spins the arm (powered separately, not from the rotor) |

## Pin connections

| Signal | ESP32 pin |
|---|---|
| LED data (DI) | GPIO23 (SPI MOSI) |
| LED clock (CI) | GPIO18 (SPI SCK) |
| Hall sensor output | GPIO27 (internal pull-up, no external resistor) |

The LED strip and the Hall sensor are powered from the 5 V output of the boost module.

## Schematic

<img width="1007" height="757" alt="Zrzut ekranu 2026-10-28 182612" src="https://github.com/user-attachments/assets/6aa13ce2-c9a1-4c79-8771-ff483497afd2" />



## Mechanical design

IN PROGRESS




## PROGRESS

- [x] Concept and parts selection
- [x] KiCad schematic
- [ ] Onshape model (hub, arm, holders for ESP32, battery and magnet)
- [ ] Build (week 9)
- [ ] Firmware: column timing, image format, first test image
- [ ] Optional: change the image over Bluetooth/Wi-Fi
