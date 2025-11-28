# Hardware Configuration

## Board Information

**Model:** WeAct ESP32-D0WDQ6 with 0.96" OLED Display  
**Product Link:** https://www.amazon.com/dp/B0FMXNSSVS

### Specifications

- **Chip:** ESP32-D0WD-V3 (revision v3.1)
- **CPU:** Dual-core Xtensa LX6, 240MHz
- **RAM:** 320KB SRAM
- **Flash:** 4MB
- **Display:** 0.96" OLED (I2C)
- **WiFi:** 802.11 b/g/n
- **Bluetooth:** BT 4.2 BR/EDR and BLE
- **Features:** VRef calibration in efuse

### Pin Configuration

- **Button:** GPIO 0
- **LED:** GPIO 22 (Active LOW)
- **OLED:** I2C interface

### PlatformIO Configuration

The board is configured in `platformio.ini` with:
- `ESP32_WEACT_096_OLED=1` for OLED display support
- 240MHz CPU frequency
- 3 worker threads for optimal thermal performance
- Hardware SHA256 acceleration enabled

### Performance

- **Hash Rate:** ~300-350 KH/s (3 workers)
- **Thermal Limit:** 75°C with automatic throttling
- **Power Consumption:** ~500mA @ 5V typical

### Optimizations Applied

- Aggressive compiler optimizations (-Ofast, -funroll-loops, -ffast-math)
- Loop unrolling and vectorization
- 32K SW batch size, 128K HW batch size
- 30-second rolling average hash rate tracking
- Thermal protection with 1-second pause when >= 75°C
