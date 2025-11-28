const { chromium } = require('playwright');
const path = require('path');

(async () => {
  const browser = await chromium.launch();
  const context = await browser.newContext({
    httpCredentials: {
      username: 'admin',
      password: 'nerd'
    },
    viewport: { width: 1280, height: 800 }
  });
  const page = await context.newPage();
  
  const imagesDir = path.join(__dirname, 'docs', 'images');
  
  try {
    // Main dashboard view
    await page.goto('http://localhost:8080/?mode=proxy');
    await page.waitForTimeout(3000); // Wait for data to load
    await page.screenshot({ path: path.join(imagesDir, 'dashboard-main.png'), fullPage: false });
    console.log('✓ Captured dashboard-main.png');
    
    // Scroll to show more stats
    await page.evaluate(() => window.scrollTo(0, 300));
    await page.waitForTimeout(1000);
    await page.screenshot({ path: path.join(imagesDir, 'dashboard-stats.png'), fullPage: false });
    console.log('✓ Captured dashboard-stats.png');
    
    // Full page view
    await page.screenshot({ path: path.join(imagesDir, 'dashboard-full.png'), fullPage: true });
    console.log('✓ Captured dashboard-full.png');
    
    // Admin page
    await page.goto('http://localhost:8080/admin');
    await page.waitForTimeout(1000);
    await page.screenshot({ path: path.join(imagesDir, 'admin-page.png'), fullPage: false });
    console.log('✓ Captured admin-page.png');
    
    // Mobile view
    await context.setViewportSize({ width: 375, height: 667 });
    await page.goto('http://localhost:8080/?mode=proxy');
    await page.waitForTimeout(3000);
    await page.screenshot({ path: path.join(imagesDir, 'dashboard-mobile.png'), fullPage: true });
    console.log('✓ Captured dashboard-mobile.png');
    
  } catch (error) {
    console.error('Error capturing screenshots:', error);
  }
  
  await browser.close();
  console.log('\n✅ All screenshots captured successfully!');
})();
