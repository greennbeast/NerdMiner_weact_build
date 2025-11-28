const puppeteer = require('puppeteer');
const path = require('path');
const fs = require('fs');

(async () => {
  console.log('🚀 Starting screenshot capture...\n');
  
  const browser = await puppeteer.launch({
    headless: true,
    args: ['--no-sandbox', '--disable-setuid-sandbox']
  });
  
  const page = await browser.newPage();
  
  // Set auth credentials
  await page.authenticate({
    username: 'admin',
    password: 'nerd'
  });
  
  const imagesDir = path.join(__dirname, 'docs', 'images');
  
  try {
    // Desktop view - Main dashboard
    console.log('📸 Capturing main dashboard (desktop)...');
    await page.setViewport({ width: 1440, height: 900 });
    await page.goto('http://localhost:8080/?mode=proxy', { waitUntil: 'networkidle0' });
    await new Promise(resolve => setTimeout(resolve, 4000)); // Wait for stats to load
    await page.screenshot({ 
      path: path.join(imagesDir, 'dashboard-main.png'),
      fullPage: false
    });
    console.log('✅ dashboard-main.png saved');
    
    // Full page view
    console.log('📸 Capturing full dashboard...');
    await page.screenshot({ 
      path: path.join(imagesDir, 'dashboard-full.png'),
      fullPage: true
    });
    console.log('✅ dashboard-full.png saved');
    
    // Admin page
    console.log('📸 Capturing admin panel...');
    await page.goto('http://localhost:8080/admin', { waitUntil: 'networkidle0' });
    await new Promise(resolve => setTimeout(resolve, 1000));
    await page.screenshot({ 
      path: path.join(imagesDir, 'admin-page.png'),
      fullPage: false
    });
    console.log('✅ admin-page.png saved');
    
    // Mobile view
    console.log('📸 Capturing mobile view...');
    await page.setViewport({ width: 375, height: 812 });
    await page.goto('http://localhost:8080/?mode=proxy', { waitUntil: 'networkidle0' });
    await new Promise(resolve => setTimeout(resolve, 4000));
    await page.screenshot({ 
      path: path.join(imagesDir, 'dashboard-mobile.png'),
      fullPage: true
    });
    console.log('✅ dashboard-mobile.png saved');
    
  } catch (error) {
    console.error('❌ Error capturing screenshots:', error.message);
    process.exit(1);
  } finally {
    await browser.close();
  }
  
  console.log('\n✨ All screenshots captured successfully!');
  console.log('📁 Location: docs/images/');
  console.log('\n💾 Ready to commit:');
  console.log('   git add docs/images/*.png');
  console.log('   git commit -m "Add dashboard screenshots"');
  console.log('   git push');
})();
