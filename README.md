# SpinPOV

A persistence-of-vision (POV) display. A thin arm with 16 RGB LEDs spins around a motor shaft, and an ESP32 changes the LEDs very fast so that the eye sees a full round image floating in the air. The whole display is screwed to the side of a wardrobe, so it does not need a stand.

**Status:** design phase (Hack Club Half-Life, week 4, Design with displays). The schematic and the CAD model are done. The build is planned for week 9.

## How it works

1. A small brushed DC motor spins the arm at about 1200-1800 RPM (20-30 revolutions per second). A bench test of the motor is in the section "Motor speed test".
2. A magnet is fixed to the stationary bracket. Every time the arm passes it, a Hall sensor (A3144) on the arm gives a pulse, so the ESP32 knows when a new revolution starts.
3. The ESP32 measures the time of the last revolution and divides it into 120 equal slices (columns of the image).
4. For each slice it sends one column of 16 pixels to the APA102 LED strip over SPI. APA102 LEDs have a separate clock line, so the timing stays stable even at this speed.
5. Because the arm spins faster than the eye can follow, the columns blend into one round image.

At 1500 RPM there are 3000 columns per second, so each column has about 330 us. Sending one frame of 16 LEDs at 4 MHz SPI takes about 144 us, so there is enough time.

Since the speed is measured on every revolution, the motor does not need exact speed control. It only needs to spin reasonably evenly.

## Hardware

Everything on the rotor is kept as small as possible. The battery is taken off and charged externally, so there is no charger on the arm. There is no custom PCB: the parts are mounted on a 3D-printed arm.

| Part | Purpose |
|---|---|
| ESP32 DevKit (30-pin) | Brain, SPI output to the LEDs, reads the Hall sensor. Screwed into a pocket in the arm |
| APA102 / SK9822 LED strip, 144 LED/m, 5 V, IP30 (16 LEDs used, about 111 mm) | The "pixels" of the display |
| A3144 Hall sensor + 4 mm thick neodymium magnet | Detects the start of each revolution |
| 1S LiPo battery + switch | Power for the rotor, hangs under the arm (also acts as a counterweight) |
| MT3608 boost module | Raises the battery voltage to 5 V for the ESP32 (VIN) and the LEDs |
| 470 uF + 100 nF capacitors | Smooth the LED current peaks and the Hall sensor supply |
| Brushed DC motor | Spins the arm (powered separately, not from the rotor) |
| M3 bolts and nuts | Ballast weights for balancing the arm |
| Cable ties, hot glue, wood screws | Mounting the battery, the magnet and the bracket |

## Pin connections

| Signal | ESP32 pin |
|---|---|
| LED data (DI) | GPIO23 (SPI MOSI) |
| LED clock (CI) | GPIO18 (SPI SCK) |
| Hall sensor output | GPIO27 (internal pull-up, no external resistor) |

The LED strip and the Hall sensor are powered from the 5 V output of the boost module. The A3144 has an open-collector output, so the ESP32 internal pull-up to 3.3 V is safe.

The APA102 data pins formally need a high level of about 3.5 V at a 5 V supply, and the ESP32 gives 3.3 V. It usually works. If the strip flickers during the tests, the plan is to add a diode in series with the 5 V supply of the strip, which lowers the threshold.

## Schematic

<img width="1007" height="757" alt="Zrzut ekranu 2026-10-28 182612" src="https://github.com/user-attachments/assets/6aa13ce2-c9a1-4c79-8771-ff483497afd2" />

The schematic is drawn in KiCad. The motor is a separate block, because it is powered from a bench supply and does not spin with the arm.

## Mechanical design

The model is made in Onshape  (STEP and STL files are in the `cad/` folder).
Onshape link: https://cad.onshape.com/documents/4df1f19cbef029885b42195c/w/202e8609e3d3d323b0620ab0/e/e5f794b9e71c9bec7b36c113

<img width="1319" height="853" alt="image" src="https://github.com/user-attachments/assets/4607846a-345e-4803-b16e-0738d93b25b0" />

### Wall bracket

The bracket is one printed part. The display is screwed to the side of a wardrobe, the motor shaft is horizontal and the picture is visible from the room.

-  **front plate** (50 x 50 x 4 mm) with a hole for the shaft, holes for the motor screws and a recess for the magnet,
-  **back plate** (70 x 50 x 4 mm) with two 4.5 mm holes for wood screws into the wardrobe,
- **two side walls** (50 x 40 x 3 mm) connecting them, so the motor sits inside, between the plates.

<img width="987" height="944" alt="Zrzut ekranu 2026-10-31 155026" src="https://github.com/user-attachments/assets/774294d2-6401-4d1d-974d-a61b2cb1522e" />

### Spinning arm

The arm (240 x 38 x 4 mm) is one part with a hub that sits on the 3 mm shaft.

-  **pocket for the LED strip** on the top (111 x 13.5 mm),
-  **pocket for the ESP32** (52 x 29 mm) with four screw holes and cut-outs for the RF shield and the USB connector, so the board sits flat. The ESP32 is 15 mm from the shaft,
-  **slot for the Hall sensor**, 13 mm from the shaft,
-  **pocket for the battery** on the underside of the arm, under the LED side, 40 mm from the shaft (so it does not touch the corners of the bracket), with slots for cable ties that run along the arm next to the strip,
- **16 ballast holes** (M3, 8 at each end of the arm) for bolts and nuts.

<img width="1540" height="930" alt="Zrzut ekranu 2026-10-31 160210" src="https://github.com/user-attachments/assets/2eace092-f801-49f0-bd61-b78694b8bce0" />


The arm is 240 mm long and does not fit on my printer bed, so it will be cut into two halves and joined with screws at the hub.

### Hall sensor and magnet

The A3144 sits in the arm and a 4 mm thick magnet sits in a shallow pocket in the front plate of the bracket. The pocket is shallow on purpose: the magnet sticks out about 2.5 mm, so the gap to the sensor is only 2-3 mm. A3144 is not very sensitive and reacts only to one magnetic pole, so the gap must be small and the magnet may have to be turned over. The magnet is glued with hot glue, which is fine because it does not move.

### Balancing

The arm spins fast, so it must be balanced: mass x distance from the shaft must be equal on both sides. Estimated masses: LED strip about 5 g, ESP32 about 10 g, battery about 10-12 g, boost converter about 2 g. The battery and the converter are under the LED side, which gives a smaller imbalance than putting them on the ESP32 side. The rest is fixed with ballast bolts in the holes at the ends of the arm. The real parts will be weighed in week 9.

## Motor speed test

I tested the motor with a bench power supply, an IR sensor and the ESP32, which counted the revolutions. The speed without any load:

| Voltage | Speed |
|---|---|
| 4 V | 2500 rpm |
| 5 V | 3250 rpm |
| 6 V | 4100 rpm |

The speed grows almost linearly with the voltage, about 800 rpm per volt. For the display about 1200-1800 rpm is enough, so the tests with the arm will start at about 3 V. The speed will be lower with the arm attached. A lower speed also means a smaller centrifugal force on the battery and more time to send each column to the LEDs.

## Safety notes

- Set the MT3608 to 5 V with a multimeter before connecting it to the ESP32.
- Switch the rotor off before connecting USB to the ESP32, so that the USB and the boost module do not power the VIN pin at the same time.

## Progress

- [x] Concept and parts selection
- [x] KiCad schematic
- [x] Onshape model (motor, wall bracket, arm with holders for ESP32, battery, magnet and sensor)
- [x] Motor speed test on the bench
- [ ] Order the parts (before week 9)
- [ ] Build (week 9): print the bracket and the arm , assemble, balance
- [ ] Firmware: column timing, image format, first test image
- [ ] Optional: change the image over Bluetooth/Wi-Fi
