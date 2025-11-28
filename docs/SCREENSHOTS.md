# Dashboard Screenshots Guide

To add screenshots to this repository:

## Using macOS Screenshot Tools

### Option 1: Full Window Capture
1. Open the dashboard at `http://localhost:8080/?mode=proxy`
2. Press `Cmd + Shift + 4`, then press `Space`
3. Click on the browser window
4. Screenshot saved to Desktop
5. Move to `docs/images/dashboard-main.png`

### Option 2: Selection Capture
1. Open the dashboard
2. Press `Cmd + Shift + 4`
3. Drag to select area
4. Screenshot saved to Desktop
5. Rename and move to `docs/images/`

## Recommended Screenshots

1. **dashboard-main.png** - Main dashboard view showing:
   - Current hash rate
   - 30s average hash rate
   - Worker status
   - Temperature
   - Connection indicator

2. **dashboard-stats.png** - Stats section with:
   - BTC price
   - Network difficulty
   - Current block
   - Pool information

3. **dashboard-debug.png** - Debug panel showing:
   - Raw JSON payload
   - All 20+ API fields

4. **admin-login.png** - Admin page at `/admin`

5. **dashboard-mobile.png** - Mobile responsive view (resize browser)

## Quick Command to Move Screenshots
```bash
# If screenshots are on Desktop
mv ~/Desktop/Screen*.png /Users/anthonykollman/NerdMiner_weact_build/docs/images/
cd /Users/anthonykollman/NerdMiner_weact_build
git add docs/images/*.png
git commit -m "Docs: add dashboard screenshots"
git push
```

## Automated Screenshot (Requires Playwright)
```bash
npm install -D playwright
npx playwright screenshot http://localhost:8080/?mode=proxy docs/images/dashboard-main.png
```
