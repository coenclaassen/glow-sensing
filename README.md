# Glow Sensing

PlatformIO project for an Adafruit Feather M0 that reads an Adafruit LTR390 UV light sensor and controls (or reads) an Adafruit AW9523 GPIO expander.

## What is in this repo
- `platformio.ini` — PlatformIO environment (Adafruit Feather M0)
- `src/main.cpp` — example sketch that initializes LTR390 and AW9523 and demonstrates reading/toggling
- `include/`, `lib/` — headers and libraries

## Wiring
- LTR390: connect to I2C (SDA, SCL), 3V and GND.
- AW9523: connect to I2C (SDA, SCL), 3V and GND. Use library methods (`pinMode`, `digitalWrite`, etc.) to control pins.

## Build & Upload
- Build: `pio run --environment adafruit_feather_m0`
- Upload: `pio run --environment adafruit_feather_m0 --target upload` or use PlatformIO: Upload from the UI
- Serial monitor: `pio device monitor -b 115200` (or use PlatformIO: Monitor)

## Notes
- This project uses PlatformIO (recommended) rather than the Arduino extension for building and uploading.
- If you want this repo pushed to GitHub, I can create a private repo and push it when you confirm (you already told me the repo slug will be `glow-sensing`).

---
