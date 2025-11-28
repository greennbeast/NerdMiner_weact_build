# NerdMiner WeAct Build

A fork of [NerdMiner_v2](https://github.com/BitMaker-hub/NerdMiner_v2) optimized for ESP32 WeAct boards with a custom web dashboard.

## Features

### Hardware
- **ESP32 WeAct 0.96" OLED** support
- 4 mining workers with hardware SHA-256 acceleration
- ~350-420 KH/s hash rate
- Temperature monitoring

### Firmware Optimizations
- Aggressive compiler optimizations (`-Ofast`, `-funroll-loops`, `-ffast-math`)
- Vectorization enabled for +112% performance boost
- 240MHz dual-core operation
- Hardware SHA-256 acceleration

### Custom Web Dashboard
- **20 API fields** including:
  - Real-time hash rate per worker
  - Temperature, BTC price, network difficulty
  - Current block, global hashrate, mining pool
  - Connection health indicator
  - Debug panel with raw JSON payload
- Auto-connect with saved miners
- CORS support for cross-origin requests
- Responsive terminal-style UI

## Quick Start

### Building Firmware
```bash
pio run -t upload
```

### Running Dashboard
```bash
./start-dashboard.command
# or
python3 -m http.server 8000
```

Open `http://localhost:8000/nerdminer-dashboard.html`

## Hardware Configuration

**ESP32 WeAct 0.96" OLED:**
- Button 1: GPIO 0
- LED: GPIO 22
- Display: 0.96" OLED (I2C)

## API Endpoints

### GET /api/stats
Returns mining statistics in JSON format:
```json
{
  "hashrate": 360.81,
  "workers": [98.4, 65.6, 98.4, 98.4],
  "shares": 0,
  "valids": 0,
  "templates": 4,
  "mhashes": 6,
  "uptime": "0d 00:00:35",
  "bestdiff": "0.014",
  "blocks": 0,
  "btcprice": 92677,
  "currentblock": "925566",
  "time": "16:27:41",
  "globalhash": "1071 EH/s",
  "difficulty": "149.30T",
  "mediumfee": 3,
  "halving": 124434,
  "chipid": "80C426E81F84",
  "temp": 67.2,
  "pool": "pool.tazmining.ch:33333",
  "netok": 1
}
```

## Build Stats
- Flash: 37.9% (1,192,309 bytes)
- RAM: 16.9% (55,216 bytes)

## Credits

This project is a fork of [NerdMiner_v2](https://github.com/BitMaker-hub/NerdMiner_v2) by BitMaker-hub.

### Original NerdMiner_v2 Features
- Multiple ESP32 board support
- Stratum V1 mining protocol
- WiFi configuration portal
- OLED/TFT display support
- Over-the-air updates

### Modifications in This Fork
- Custom web dashboard with 20 API fields
- Enhanced CORS support
- Connection health monitoring
- Debug panel with JSON payload
- Pool name display
- Render tick lock architecture for stable UI updates

## Dashboard Proxy (Public Access)

Use the included Node.js proxy to serve the dashboard locally and safely expose it on the web.

### Requirements
- PlatformIO (VS Code extension) for firmware build/upload
- Node.js (macOS: `brew install node`)
- Optional: ngrok for public URL (`brew install ngrok`)

### Firmware First
Build and upload firmware to your ESP32 before running the proxy:
```bash
pio run -t upload
```

### Setup
1. Install deps:
   ```bash
   npm install
   ```
2. Create `.env` from example and edit values:
   ```bash
   cp .env.example .env
   # MINER_IP=<esp32-ip>
   # DASH_USER=<username>
   # DASH_PASS=<password>
   # PORT=8080
   ```

### Run Locally
```bash
npm start
# Open:
# http://localhost:8080/?mode=proxy
```

### Change Credentials
- Visit `/admin` (e.g., `http://localhost:8080/admin`) to update username/password.
- Changes persist to `.env`. Use `/logout` to force re-auth.

### Expose Publicly (ngrok)
```bash
ngrok config add-authtoken <YOUR_NGROK_TOKEN>
ngrok http 8080
# Share:
# https://<subdomain>.ngrok-free.dev/?mode=proxy
```

### Notes
- `.env` is git-ignored; secrets are never committed.
- If the miner’s IP changes, update `MINER_IP` and restart the proxy.
- If `/api/stats` returns WiFiManager HTML, ensure you are using the proxy URL with `?mode=proxy`.

## License

Follows the same license as the original NerdMiner_v2 project.
