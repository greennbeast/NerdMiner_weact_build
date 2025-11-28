# NerdMiner Dashboard Screenshots

This directory contains screenshots of the NerdMiner web dashboard.

## Available Screenshots

### Main Dashboard (`dashboard-main.png`)
- Real-time hash rate display (current + 30s average)
- Worker status for 3 miners
- Temperature monitoring
- Connection health indicator
- BTC price and network stats

### Full Dashboard (`dashboard-full.png`)
- Complete scrollable view
- All 20+ API fields visible
- Debug panel with raw JSON
- Pool information
- Uptime and share statistics

### Admin Panel (`admin-page.png`)
- Credential management interface
- Username/password update form
- Secure authentication controls

### Mobile View (`dashboard-mobile.png`)
- Responsive design on mobile devices
- Touch-optimized interface
- Compact stat display

## Capturing New Screenshots

Run the helper script:
```bash
./capture-screenshots.sh
```

Or manually:
1. Open http://localhost:8080/?mode=proxy (login: admin/nerd)
2. Press `Cmd+Shift+4`, then `Space`, then click browser window
3. Move screenshot from Desktop to `docs/images/`
4. Commit and push

## Usage in README

Reference screenshots using relative paths:
```markdown
![Dashboard](docs/images/dashboard-main.png)
```
