require('dotenv').config();
const fs = require('fs');
const path = require('path');
const express = require('express');
const { createProxyMiddleware } = require('http-proxy-middleware');

const app = express();
const PORT = process.env.PORT || 8080;
const MINER_IP = process.env.MINER_IP || '192.168.0.185';
let CURRENT_USER = process.env.DASH_USER || '';
let CURRENT_PASS = process.env.DASH_PASS || '';
const MINER_BASE = `http://${MINER_IP}`;

// Simple logging
app.use((req, _res, next) => {
  console.log(`[${new Date().toISOString()}] ${req.method} ${req.url}`);
  next();
});

// Optional Basic Auth for all routes when creds are set
app.use((req, res, next) => {
  if (!CURRENT_USER && !CURRENT_PASS) return next(); // auth disabled
  const hdr = req.headers['authorization'] || '';
  if (!hdr.startsWith('Basic ')) {
    res.set('WWW-Authenticate', 'Basic realm="NerdMiner"');
    return res.status(401).send('Authentication required');
  }
  const decoded = Buffer.from(hdr.slice(6), 'base64').toString('utf8');
  const idx = decoded.indexOf(':');
  const user = idx >= 0 ? decoded.slice(0, idx) : decoded;
  const pass = idx >= 0 ? decoded.slice(idx + 1) : '';
  if (user === CURRENT_USER && pass === CURRENT_PASS) return next();
  res.set('WWW-Authenticate', 'Basic realm="NerdMiner"');
  return res.status(401).send('Invalid credentials');
});

// Body parsers for admin forms/JSON
app.use(express.urlencoded({ extended: false }));
app.use(express.json());

// Helper: persist credentials to .env in-place (best-effort)
function persistEnv(updates) {
  try {
    const envPath = path.join(__dirname, '.env');
    let lines = [];
    if (fs.existsSync(envPath)) {
      lines = fs.readFileSync(envPath, 'utf8').split(/\r?\n/);
    }
    const map = {};
    for (const line of lines) {
      const m = /^\s*([A-Za-z_][A-Za-z0-9_]*)\s*=\s*(.*)\s*$/.exec(line);
      if (m) map[m[1]] = m[2];
    }
    for (const [k, v] of Object.entries(updates)) {
      map[k] = String(v);
    }
    const out = Object.entries(map)
      .map(([k, v]) => `${k}=${v}`)
      .join('\n') + '\n';
    fs.writeFileSync(envPath, out, 'utf8');
    console.log('Updated .env with new settings');
  } catch (e) {
    console.warn('Failed to persist .env:', e.message);
  }
}

// Admin UI to change credentials
app.get('/admin', (_req, res) => {
  const masked = CURRENT_PASS ? '•'.repeat(Math.min(CURRENT_PASS.length, 12)) : '';
  res.set('Content-Type', 'text/html; charset=utf-8');
  res.send(`<!DOCTYPE html><html><head><meta charset="utf-8"><title>NerdMiner Admin</title></head>
  <body style="font-family: system-ui, sans-serif; max-width: 640px; margin: 2rem auto;">
    <h2>NerdMiner Dashboard Admin</h2>
    <p>Update Basic Auth credentials used to protect the dashboard and API.</p>
    <form method="post" action="/admin/password" style="display:grid;gap:8px;max-width:420px;">
      <label>New Username <input name="user" required value="${CURRENT_USER}" /></label>
      <label>New Password <input type="password" name="pass" required placeholder="••••••" /></label>
      <label>Confirm Password <input type="password" name="confirm" required placeholder="••••••" /></label>
      <button type="submit">Update Credentials</button>
    </form>
    <p style="color:#555;margin-top:1rem">Current user: <code>${CURRENT_USER || '(not set)'}</code> • Current pass: <code>${masked || '(not set)'}</code></p>
    <p><a href="/logout">Logout</a> (prompts for credentials again)</p>
  </body></html>`);
});

app.get('/logout', (_req, res) => {
  res.set('WWW-Authenticate', 'Basic realm="NerdMiner"');
  return res.status(401).send('Logged out');
});

app.post('/admin/password', (req, res) => {
  const { user, pass, confirm } = req.body || {};
  if (!user || !pass) return res.status(400).send('User and password are required');
  if (pass !== confirm) return res.status(400).send('Passwords do not match');
  if (String(pass).length < 8) return res.status(400).send('Password must be at least 8 characters');
  CURRENT_USER = String(user);
  CURRENT_PASS = String(pass);
  persistEnv({ DASH_USER: CURRENT_USER, DASH_PASS: CURRENT_PASS });
  // Force re-auth on next request
  res.set('WWW-Authenticate', 'Basic realm="NerdMiner"');
  return res.status(200).send('Credentials updated. Please re-authenticate.');
});

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
