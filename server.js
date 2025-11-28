require('dotenv').config();
const express = require('express');
const path = require('path');
const { createProxyMiddleware } = require('http-proxy-middleware');

const app = express();
const PORT = process.env.PORT || 8080;
const MINER_IP = process.env.MINER_IP || '192.168.0.185';
const DASH_USER = process.env.DASH_USER || '';
const DASH_PASS = process.env.DASH_PASS || '';
const MINER_BASE = `http://${MINER_IP}`;

// Simple logging
app.use((req, _res, next) => {
  console.log(`[${new Date().toISOString()}] ${req.method} ${req.url}`);
  next();
});

// Optional Basic Auth for all routes when creds are set
if (DASH_USER && DASH_PASS) {
  app.use((req, res, next) => {
    const hdr = req.headers['authorization'] || '';
    if (!hdr.startsWith('Basic ')) {
      res.set('WWW-Authenticate', 'Basic realm="NerdMiner"');
      return res.status(401).send('Authentication required');
    }
    const decoded = Buffer.from(hdr.slice(6), 'base64').toString('utf8');
    const idx = decoded.indexOf(':');
    const user = idx >= 0 ? decoded.slice(0, idx) : decoded;
    const pass = idx >= 0 ? decoded.slice(idx + 1) : '';
    if (user === DASH_USER && pass === DASH_PASS) return next();
    res.set('WWW-Authenticate', 'Basic realm="NerdMiner"');
    return res.status(401).send('Invalid credentials');
  });
}

// Proxy API to the ESP32 miner (match any /api/* path explicitly)
// Special case: stats endpoint via fetch to avoid odd proxy behavior on some firmwares
app.get('/api/stats', async (_req, res) => {
  try {
    const r = await fetch(`${MINER_BASE}/api/stats`, { headers: { 'Cache-Control': 'no-store', 'Accept': 'application/json' } });
    const text = await r.text();
    res.status(r.status);
    res.set('Content-Type', r.headers.get('content-type') || 'application/json');
    res.set('Cache-Control', 'no-store');
    return res.send(text);
  } catch (e) {
    console.error('stats proxy error', e);
    return res.status(502).json({ error: 'Bad gateway', detail: String(e) });
  }
});

app.use('/api/*', createProxyMiddleware({
  target: MINER_BASE,
  changeOrigin: true,
  xfwd: true,
  followRedirects: true,
  // keep path as-is (no rewrite)
  pathRewrite: (path) => path,
  onProxyReq: (proxyReq, req) => {
    proxyReq.setHeader('Cache-Control', 'no-store');
    console.log(`-> proxy ${req.method} ${req.url} to ${MINER_BASE}${req.url}`);
  },
  onProxyRes: (proxyRes, req) => {
    console.log(`<- proxy ${req.method} ${req.url} status ${proxyRes.statusCode}`);
  }
}));

// Serve the dashboard at root
app.get('/', (_req, res) => {
  res.sendFile(path.join(__dirname, 'nerdminer-dashboard.html'));
});

// Fallback for other assets if needed (do not 404, allow fallthrough)
app.use(express.static(__dirname, { fallthrough: true }));

app.listen(PORT, () => {
  console.log(`Dashboard server running on http://localhost:${PORT}`);
  console.log(`Proxying /api to ${MINER_BASE}`);
});
