#ifndef WEB_PANEL_H
#define WEB_PANEL_H

// ════════════════════════════════════════════════════════════
//  AURORA WEB PANEL — Interface HTML embutida (PROGMEM)
//  Servida pelo WebServer em WebServer.cpp
// ════════════════════════════════════════════════════════════

const char AURORA_HTML[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>AURORA · Painel de Controle</title>
<style>
  :root {
    --bg:       #0a0c10;
    --surface:  #111318;
    --border:   #1e2230;
    --border2:  #2a2f42;
    --accent:   #4fd1c5;
    --accent2:  #f6ad55;
    --accent3:  #fc8181;
    --text:     #e2e8f0;
    --muted:    #718096;
    --green:    #68d391;
    --red:      #fc8181;
    --yellow:   #f6e05e;
    --blue:     #63b3ed;
    --radius:   12px;
    --mono:     'Courier New', monospace;
    --sans:     -apple-system, BlinkMacSystemFont, 'Segoe UI', Arial, sans-serif;
  }

  * { box-sizing: border-box; margin: 0; padding: 0; }

  body {
    background: var(--bg);
    color: var(--text);
    font-family: var(--sans);
    min-height: 100vh;
    overflow-x: hidden;
  }

  /* ── GRID BACKGROUND ─────────────────────────────────────── */
  body::before {
    content: '';
    position: fixed;
    inset: 0;
    background-image:
      linear-gradient(var(--border) 1px, transparent 1px),
      linear-gradient(90deg, var(--border) 1px, transparent 1px);
    background-size: 40px 40px;
    opacity: 0.4;
    pointer-events: none;
    z-index: 0;
  }

  /* ── LOGIN SCREEN ────────────────────────────────────────── */
  #loginScreen {
    position: fixed;
    inset: 0;
    display: flex;
    align-items: center;
    justify-content: center;
    z-index: 100;
    background: var(--bg);
  }

  .login-card {
    background: var(--surface);
    border: 1px solid var(--border2);
    border-radius: 20px;
    padding: 48px 40px;
    width: 360px;
    text-align: center;
    position: relative;
    overflow: hidden;
  }

  .login-card::before {
    content: '';
    position: absolute;
    top: -60px; left: 50%; transform: translateX(-50%);
    width: 200px; height: 200px;
    background: radial-gradient(circle, rgba(79,209,197,0.12) 0%, transparent 70%);
    pointer-events: none;
  }

  .login-logo {
    font-size: 13px;
    font-weight: 600;
    letter-spacing: 6px;
    color: var(--accent);
    text-transform: uppercase;
    margin-bottom: 8px;
  }

  .login-title {
    font-size: 32px;
    font-weight: 800;
    letter-spacing: -1px;
    margin-bottom: 32px;
  }

  .login-input {
    width: 100%;
    background: var(--bg);
    border: 1px solid var(--border2);
    border-radius: var(--radius);
    padding: 14px 18px;
    color: var(--text);
    font-family: var(--mono);
    font-size: 18px;
    text-align: center;
    letter-spacing: 8px;
    outline: none;
    transition: border-color .2s;
    margin-bottom: 16px;
  }

  .login-input:focus { border-color: var(--accent); }

  .login-btn {
    width: 100%;
    background: var(--accent);
    color: #0a0c10;
    border: none;
    border-radius: var(--radius);
    padding: 14px;
    font-family: var(--sans);
    font-size: 15px;
    font-weight: 700;
    letter-spacing: 2px;
    text-transform: uppercase;
    cursor: pointer;
    transition: opacity .2s, transform .1s;
  }
  .login-btn:hover { opacity: .85; }
  .login-btn:active { transform: scale(.98); }

  .login-error {
    color: var(--red);
    font-size: 13px;
    margin-top: 12px;
    min-height: 18px;
    font-family: var(--mono);
  }

  /* ── MAIN LAYOUT ─────────────────────────────────────────── */
  #app { display: none; position: relative; z-index: 1; }

  /* ── TOPBAR ───────────────────────────────────────────────── */
  .topbar {
    position: sticky; top: 0; z-index: 50;
    background: rgba(10,12,16,0.92);
    border-bottom: 1px solid var(--border);
    padding: 0 24px;
    height: 60px;
    display: flex;
    align-items: center;
    gap: 16px;
  }

  .topbar-brand {
    font-size: 11px;
    font-weight: 700;
    letter-spacing: 5px;
    color: var(--accent);
    text-transform: uppercase;
    flex: 1;
  }

  .topbar-brand span { color: var(--muted); font-weight: 400; }

  .status-dot {
    width: 8px; height: 8px;
    border-radius: 50%;
    background: var(--green);
    box-shadow: 0 0 8px var(--green);
    animation: pulse 2s infinite;
  }

  @keyframes pulse {
    0%, 100% { opacity: 1; }
    50% { opacity: .4; }
  }

  .topbar-time {
    font-family: var(--mono);
    font-size: 13px;
    color: var(--muted);
  }

  .logout-btn {
    background: none;
    border: 1px solid var(--border2);
    border-radius: 8px;
    color: var(--muted);
    font-family: var(--sans);
    font-size: 12px;
    font-weight: 600;
    letter-spacing: 1px;
    padding: 6px 14px;
    cursor: pointer;
    text-transform: uppercase;
    transition: all .2s;
  }
  .logout-btn:hover { border-color: var(--red); color: var(--red); }

  /* ── NAV TABS ─────────────────────────────────────────────── */
  .nav {
    display: flex;
    gap: 2px;
    padding: 12px 24px 0;
    border-bottom: 1px solid var(--border);
    overflow-x: auto;
  }

  .nav-tab {
    padding: 10px 20px;
    font-size: 13px;
    font-weight: 600;
    letter-spacing: .5px;
    color: var(--muted);
    cursor: pointer;
    border: none;
    background: none;
    border-bottom: 2px solid transparent;
    transition: all .2s;
    white-space: nowrap;
    font-family: var(--sans);
    text-transform: uppercase;
  }
  .nav-tab:hover { color: var(--text); }
  .nav-tab.active { color: var(--accent); border-bottom-color: var(--accent); }

  /* ── CONTENT ──────────────────────────────────────────────── */
  .content {
    padding: 24px;
    max-width: 1200px;
    margin: 0 auto;
  }

  .tab-panel { display: none; }
  .tab-panel.active { display: block; }

  /* ── CARDS ────────────────────────────────────────────────── */
  .card {
    background: var(--surface);
    border: 1px solid var(--border);
    border-radius: var(--radius);
    padding: 20px;
    margin-bottom: 16px;
  }

  .card-title {
    font-size: 10px;
    font-weight: 700;
    letter-spacing: 3px;
    text-transform: uppercase;
    color: var(--muted);
    margin-bottom: 16px;
    display: flex;
    align-items: center;
    gap: 8px;
  }

  .card-title::after {
    content: '';
    flex: 1;
    height: 1px;
    background: var(--border);
  }

  /* ── STAT GRID ────────────────────────────────────────────── */
  .stat-grid {
    display: grid;
    grid-template-columns: repeat(auto-fill, minmax(160px, 1fr));
    gap: 12px;
    margin-bottom: 16px;
  }

  .stat {
    background: var(--bg);
    border: 1px solid var(--border);
    border-radius: var(--radius);
    padding: 16px;
    position: relative;
    overflow: hidden;
  }

  .stat::before {
    content: '';
    position: absolute;
    bottom: 0; left: 0; right: 0;
    height: 2px;
    background: var(--accent);
    opacity: .4;
  }

  .stat-label {
    font-size: 11px;
    font-weight: 600;
    letter-spacing: 2px;
    text-transform: uppercase;
    color: var(--muted);
    margin-bottom: 8px;
  }

  .stat-value {
    font-family: var(--mono);
    font-size: 22px;
    font-weight: 500;
    color: var(--text);
    line-height: 1;
  }

  .stat-value.ok  { color: var(--green); }
  .stat-value.warn { color: var(--yellow); }
  .stat-value.bad  { color: var(--red); }
  .stat-unit { font-size: 13px; color: var(--muted); margin-left: 4px; }

  /* ── PROGRESS BAR ─────────────────────────────────────────── */
  .progress-wrap { margin: 8px 0; }
  .progress-label {
    display: flex;
    justify-content: space-between;
    font-size: 12px;
    color: var(--muted);
    margin-bottom: 6px;
    font-family: var(--mono);
  }
  .progress-bar {
    height: 6px;
    background: var(--border);
    border-radius: 3px;
    overflow: hidden;
  }
  .progress-fill {
    height: 100%;
    border-radius: 3px;
    transition: width .4s ease;
    background: var(--accent);
  }
  .progress-fill.warn { background: var(--yellow); }
  .progress-fill.bad  { background: var(--red); }

  /* ── INPUT / TEXTAREA ─────────────────────────────────────── */
  .inp {
    width: 100%;
    background: var(--bg);
    border: 1px solid var(--border2);
    border-radius: 8px;
    padding: 12px 14px;
    color: var(--text);
    font-family: var(--mono);
    font-size: 14px;
    outline: none;
    transition: border-color .2s;
    margin-bottom: 12px;
  }
  .inp:focus { border-color: var(--accent); }

  textarea.inp {
    resize: vertical;
    min-height: 120px;
    font-size: 13px;
    line-height: 1.6;
  }

  /* ── BUTTONS ──────────────────────────────────────────────── */
  .btn {
    display: inline-flex;
    align-items: center;
    gap: 8px;
    padding: 10px 20px;
    border-radius: 8px;
    font-family: var(--sans);
    font-size: 12px;
    font-weight: 700;
    letter-spacing: 1px;
    text-transform: uppercase;
    cursor: pointer;
    border: none;
    transition: all .2s;
  }
  .btn-primary { background: var(--accent); color: #0a0c10; }
  .btn-primary:hover { opacity: .85; }
  .btn-secondary { background: var(--border2); color: var(--text); }
  .btn-secondary:hover { background: var(--border); }
  .btn-danger { background: rgba(252,129,129,.15); color: var(--red); border: 1px solid rgba(252,129,129,.3); }
  .btn-danger:hover { background: rgba(252,129,129,.25); }
  .btn-warn { background: rgba(246,173,85,.15); color: var(--accent2); border: 1px solid rgba(246,173,85,.3); }
  .btn-warn:hover { background: rgba(246,173,85,.25); }
  .btn-sm { padding: 6px 14px; font-size: 11px; }
  .btn:disabled { opacity: .4; cursor: not-allowed; }

  .btn-row { display: flex; flex-wrap: wrap; gap: 8px; margin-top: 12px; }

  /* ── FORM ROW ─────────────────────────────────────────────── */
  .form-row {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 12px;
  }
  @media(max-width:600px){ .form-row { grid-template-columns: 1fr; } }

  .form-label {
    font-size: 11px;
    font-weight: 600;
    letter-spacing: 2px;
    text-transform: uppercase;
    color: var(--muted);
    margin-bottom: 6px;
  }

  /* ── FILE EXPLORER ────────────────────────────────────────── */
  .file-tree { font-family: var(--mono); font-size: 13px; }

  .file-dir {
    padding: 8px 12px;
    display: flex;
    align-items: center;
    gap: 8px;
    cursor: pointer;
    border-radius: 6px;
    color: var(--accent);
    font-weight: 500;
    transition: background .15s;
  }
  .file-dir:hover { background: var(--border); }

  .file-entry {
    padding: 7px 12px 7px 36px;
    display: flex;
    align-items: center;
    gap: 8px;
    cursor: pointer;
    border-radius: 6px;
    color: var(--text);
    transition: background .15s;
    justify-content: space-between;
  }
  .file-entry:hover { background: var(--border); }

  .file-name { flex: 1; }
  .file-size { color: var(--muted); font-size: 11px; }

  .file-actions { display: flex; gap: 6px; opacity: 0; transition: opacity .15s; }
  .file-entry:hover .file-actions { opacity: 1; }

  .file-btn {
    padding: 3px 8px;
    border-radius: 4px;
    border: 1px solid var(--border2);
    background: none;
    color: var(--muted);
    font-size: 11px;
    cursor: pointer;
    font-family: var(--mono);
    transition: all .15s;
  }
  .file-btn:hover { color: var(--accent); border-color: var(--accent); }
  .file-btn.del:hover { color: var(--red); border-color: var(--red); }

  /* ── EDITOR MODAL ─────────────────────────────────────────── */
  .modal-overlay {
    position: fixed;
    inset: 0;
    background: rgba(0,0,0,.7);
    z-index: 200;
    display: none;
    align-items: center;
    justify-content: center;
    padding: 20px;
  }
  .modal-overlay.open { display: flex; }

  .modal {
    background: var(--surface);
    border: 1px solid var(--border2);
    border-radius: 16px;
    width: 100%;
    max-width: 700px;
    max-height: 80vh;
    display: flex;
    flex-direction: column;
    overflow: hidden;
  }

  .modal-header {
    padding: 16px 20px;
    border-bottom: 1px solid var(--border);
    display: flex;
    align-items: center;
    gap: 12px;
  }

  .modal-title {
    font-family: var(--mono);
    font-size: 13px;
    flex: 1;
    color: var(--accent);
  }

  .modal-body { flex: 1; overflow: auto; padding: 16px; }

  .modal textarea {
    width: 100%;
    height: 100%;
    min-height: 300px;
    background: var(--bg);
    border: 1px solid var(--border);
    border-radius: 8px;
    color: var(--text);
    font-family: var(--mono);
    font-size: 13px;
    line-height: 1.6;
    padding: 14px;
    resize: none;
    outline: none;
  }

  .modal-footer {
    padding: 12px 20px;
    border-top: 1px solid var(--border);
    display: flex;
    justify-content: flex-end;
    gap: 8px;
  }

  /* ── TOAST ────────────────────────────────────────────────── */
  #toast {
    position: fixed;
    bottom: 24px;
    right: 24px;
    z-index: 999;
    display: flex;
    flex-direction: column;
    gap: 8px;
    pointer-events: none;
  }

  .toast-item {
    background: var(--surface);
    border: 1px solid var(--border2);
    border-radius: 10px;
    padding: 12px 18px;
    font-size: 13px;
    display: flex;
    align-items: center;
    gap: 10px;
    animation: slideIn .25s ease;
    pointer-events: all;
  }
  .toast-item.success { border-left: 3px solid var(--green); }
  .toast-item.error   { border-left: 3px solid var(--red); }
  .toast-item.info    { border-left: 3px solid var(--accent); }

  @keyframes slideIn {
    from { transform: translateX(60px); opacity: 0; }
    to   { transform: translateX(0);    opacity: 1; }
  }

  /* ── LOG VIEWER ───────────────────────────────────────────── */
  .log-box {
    background: var(--bg);
    border: 1px solid var(--border);
    border-radius: 8px;
    padding: 14px;
    font-family: var(--mono);
    font-size: 12px;
    line-height: 1.7;
    max-height: 300px;
    overflow-y: auto;
    color: var(--text);
  }

  .log-line { display: block; }
  .log-line.err  { color: var(--red); }
  .log-line.warn { color: var(--yellow); }
  .log-line.ok   { color: var(--green); }

  /* ── SECTION DIVIDER ──────────────────────────────────────── */
  .section-head {
    display: flex;
    align-items: center;
    justify-content: space-between;
    margin-bottom: 16px;
    gap: 12px;
  }
  .section-title {
    font-size: 18px;
    font-weight: 700;
    letter-spacing: -0.5px;
  }

  /* ── TOGGLE ───────────────────────────────────────────────── */
  .toggle-row {
    display: flex;
    align-items: center;
    justify-content: space-between;
    padding: 14px 0;
    border-bottom: 1px solid var(--border);
  }
  .toggle-row:last-child { border-bottom: none; }
  .toggle-info { flex: 1; }
  .toggle-name { font-size: 14px; font-weight: 600; margin-bottom: 3px; }
  .toggle-desc { font-size: 12px; color: var(--muted); font-family: var(--mono); }

  .toggle {
    position: relative;
    width: 44px;
    height: 24px;
    flex-shrink: 0;
  }
  .toggle input { opacity: 0; width: 0; height: 0; }
  .slider {
    position: absolute;
    inset: 0;
    background: var(--border2);
    border-radius: 24px;
    cursor: pointer;
    transition: background .2s;
  }
  .slider::before {
    content: '';
    position: absolute;
    left: 3px; bottom: 3px;
    width: 18px; height: 18px;
    background: white;
    border-radius: 50%;
    transition: transform .2s;
  }
  .toggle input:checked + .slider { background: var(--accent); }
  .toggle input:checked + .slider::before { transform: translateX(20px); }

  /* ── MINI CHART ───────────────────────────────────────────── */
  .chart-wrap {
    position: relative;
    height: 80px;
    margin: 8px 0;
  }

  canvas.sparkline {
    width: 100%;
    height: 80px;
  }

  /* ── WEATHER GRID ─────────────────────────────────────────── */
  .weather-grid {
    display: grid;
    grid-template-columns: repeat(3, 1fr);
    gap: 10px;
    margin-top: 12px;
  }

  .weather-card {
    background: var(--bg);
    border: 1px solid var(--border);
    border-radius: var(--radius);
    padding: 14px;
    text-align: center;
  }

  .weather-hour { font-size: 11px; color: var(--muted); margin-bottom: 4px; font-family: var(--mono); }
  .weather-temp { font-size: 20px; font-weight: 700; font-family: var(--mono); }
  .weather-rain { font-size: 12px; color: var(--blue); margin-top: 4px; font-family: var(--mono); }

  /* ── SCROLLBAR ────────────────────────────────────────────── */
  ::-webkit-scrollbar { width: 6px; height: 6px; }
  ::-webkit-scrollbar-track { background: var(--bg); }
  ::-webkit-scrollbar-thumb { background: var(--border2); border-radius: 3px; }
  ::-webkit-scrollbar-thumb:hover { background: var(--muted); }

  /* ── RESPONSIVE ───────────────────────────────────────────── */
  @media(max-width:480px){
    .stat-grid { grid-template-columns: 1fr 1fr; }
    .weather-grid { grid-template-columns: 1fr; }
    .content { padding: 16px; }
    .topbar { padding: 0 16px; }
  }
  @media(max-width:1024px){
    body::before { display: none; }
  }

  /* ── ANIMATIONS ───────────────────────────────────────────── */
  .fade-in { animation: fadeIn .3s ease; }
  @keyframes fadeIn { from { opacity:0; transform:translateY(8px); } to { opacity:1; transform:translateY(0); } }

  .spinner {
    display: inline-block;
    width: 14px; height: 14px;
    border: 2px solid var(--border2);
    border-top-color: var(--accent);
    border-radius: 50%;
    animation: spin .6s linear infinite;
  }
  @keyframes spin { to { transform: rotate(360deg); } }
</style>
</head>
<body>

<!-- ════════════════════ LOGIN ═══════════════════════════ -->
<div id="loginScreen">
  <div class="login-card">
    <div class="login-logo">Sistema</div>
    <div class="login-title">AURORA</div>
    <input class="login-input" type="password" id="loginPin" maxlength="6"
           placeholder="····" autocomplete="off"
           onkeydown="if(event.key==='Enter') doLogin()">
    <button class="login-btn" onclick="doLogin()">ENTRAR</button>
    <div class="login-error" id="loginErr"></div>
  </div>
</div>

<!-- ════════════════════ APP ════════════════════════════ -->
<div id="app">

  <!-- TOPBAR -->
  <div class="topbar">
    <div class="topbar-brand">AURORA <span>· ESP32-S3</span></div>
    <div class="status-dot" id="connDot" title="Conexão"></div>
    <div class="topbar-time" id="topTime">--:--</div>
    <button class="logout-btn" onclick="doLogout()">Sair</button>
  </div>

  <!-- NAV TABS -->
  <div class="nav">
    <button class="nav-tab active" onclick="showTab('dashboard')">Dashboard</button>
    <button class="nav-tab" onclick="showTab('clima')">Clima</button>
    <button class="nav-tab" onclick="showTab('sdcard')">SD Card</button>
    <button class="nav-tab" onclick="showTab('config')">Configurações</button>
    <button class="nav-tab" onclick="showTab('controle')">Controle</button>
  </div>

  <!-- ═══════════════ DASHBOARD ══════════════════════════ -->
  <div class="content">
  <div id="tab-dashboard" class="tab-panel active fade-in">

    <div class="section-head">
      <div class="section-title">Visão Geral</div>
      <button class="btn btn-secondary btn-sm" onclick="refreshDash()">
        <span id="refreshIcon">↻</span> Atualizar
      </button>
    </div>

    <div class="stat-grid">
      <div class="stat">
        <div class="stat-label">Temperatura Chip</div>
        <div class="stat-value" id="statChipTemp">--<span class="stat-unit">°C</span></div>
      </div>
      <div class="stat">
        <div class="stat-label">Heap Livre</div>
        <div class="stat-value" id="statHeap">--<span class="stat-unit">%</span></div>
      </div>
      <div class="stat">
        <div class="stat-label">WiFi</div>
        <div class="stat-value" id="statWifi">--<span class="stat-unit">dBm</span></div>
      </div>
      <div class="stat">
        <div class="stat-label">Uptime</div>
        <div class="stat-value" id="statUptime" style="font-size:16px">--</div>
      </div>
      <div class="stat">
        <div class="stat-label">Perguntas IA</div>
        <div class="stat-value" id="statQuestions">--</div>
      </div>
      <div class="stat">
        <div class="stat-label">Clima Ext.</div>
        <div class="stat-value" id="statClimaTemp">--<span class="stat-unit">°C</span></div>
      </div>
    </div>

    <div class="card">
      <div class="card-title">Recursos do Sistema</div>
      <div class="progress-wrap">
        <div class="progress-label">
          <span>Heap RAM</span>
          <span id="heapPct">--%</span>
        </div>
        <div class="progress-bar">
          <div class="progress-fill" id="heapBar" style="width:0%"></div>
        </div>
      </div>
      <div class="progress-wrap" style="margin-top:12px">
        <div class="progress-label">
          <span>Sinal WiFi</span>
          <span id="wifiPct">--%</span>
        </div>
        <div class="progress-bar">
          <div class="progress-fill" id="wifiBar" style="width:0%"></div>
        </div>
      </div>
    </div>

    <div class="card">
      <div class="card-title">Info do Sistema</div>
      <div style="font-family:var(--mono);font-size:13px;line-height:2;color:var(--muted)">
        <div>IP: <span id="infoIP" style="color:var(--accent)">--</span></div>
        <div>SSID: <span id="infoSSID" style="color:var(--text)">--</span></div>
        <div>Modelo IA: <span id="infoModelo" style="color:var(--text)">--</span></div>
        <div>Cidade: <span id="infoCidade" style="color:var(--text)">--</span></div>
        <div>SD Card: <span id="infoSD" style="color:var(--text)">--</span></div>
        <div>OTA: <span id="infoOTA" style="color:var(--text)">--</span></div>
      </div>
    </div>

  </div>

  <!-- ═══════════════ CLIMA ═══════════════════════════════ -->
  <div id="tab-clima" class="tab-panel fade-in">

    <div class="section-head">
      <div class="section-title">Clima &amp; Previsão</div>
      <button class="btn btn-secondary btn-sm" onclick="atualizarClima()">↻ Forçar Update</button>
    </div>

    <div class="card">
      <div class="card-title">Agora em <span id="climaCidade" style="color:var(--accent)">--</span></div>
      <div class="stat-grid">
        <div class="stat">
          <div class="stat-label">Temperatura</div>
          <div class="stat-value" id="cTemp">--<span class="stat-unit">°C</span></div>
        </div>
        <div class="stat">
          <div class="stat-label">Sensação</div>
          <div class="stat-value" id="cSens">--<span class="stat-unit">°C</span></div>
        </div>
        <div class="stat">
          <div class="stat-label">Umidade</div>
          <div class="stat-value" id="cHum">--<span class="stat-unit">%</span></div>
        </div>
        <div class="stat">
          <div class="stat-label">Pressão</div>
          <div class="stat-value" id="cPressao" style="font-size:18px">--<span class="stat-unit">hPa</span></div>
        </div>
        <div class="stat">
          <div class="stat-label">Vento</div>
          <div class="stat-value" id="cVento" style="font-size:18px">--<span class="stat-unit">km/h</span></div>
        </div>
        <div class="stat">
          <div class="stat-label">Condição</div>
          <div id="cDesc" style="font-size:13px;margin-top:8px;color:var(--text);font-family:var(--mono)">--</div>
        </div>
      </div>
    </div>

    <div class="card">
      <div class="card-title">Previsão Próximas Horas</div>
      <div class="weather-grid">
        <div class="weather-card">
          <div class="weather-hour" id="ph1">+3h</div>
          <div class="weather-temp" id="pt1">--°</div>
          <div class="weather-rain" id="pr1">--%</div>
        </div>
        <div class="weather-card">
          <div class="weather-hour" id="ph2">+6h</div>
          <div class="weather-temp" id="pt2">--°</div>
          <div class="weather-rain" id="pr2">--%</div>
        </div>
        <div class="weather-card">
          <div class="weather-hour" id="ph3">+9h</div>
          <div class="weather-temp" id="pt3">--°</div>
          <div class="weather-rain" id="pr3">--%</div>
        </div>
      </div>
    </div>

    <div class="card" id="alertaCard" style="display:none;border-color:rgba(252,129,129,.4)">
      <div class="card-title" style="color:var(--red)">⚠ Alerta Ativo</div>
      <div id="alertaMsg" style="font-family:var(--mono);font-size:13px;line-height:1.6;color:var(--red)"></div>
    </div>

  </div>

  <!-- ═══════════════ SD CARD ═════════════════════════════ -->
  <div id="tab-sdcard" class="tab-panel fade-in">

    <div class="section-head">
      <div class="section-title">Gerenciador SD</div>
      <button class="btn btn-secondary btn-sm" onclick="loadFiles('/')">↻ Recarregar</button>
    </div>

    <div class="card">
      <div class="card-title">Explorador de Arquivos</div>
      <div id="fileTree" class="file-tree">
        <div style="color:var(--muted);font-family:var(--mono);font-size:13px;padding:12px">
          Carregando...
        </div>
      </div>
    </div>

    <div class="card">
      <div class="card-title">Criar Novo Arquivo</div>
      <div class="form-label">Caminho completo</div>
      <input class="inp" id="newFilePath" placeholder="/aurora/agenda/nota.txt">
      <div class="form-label">Conteúdo</div>
      <textarea class="inp" id="newFileContent" placeholder="Conteúdo do arquivo..." style="min-height:80px"></textarea>
      <button class="btn btn-primary" onclick="criarArquivo()">Criar Arquivo</button>
    </div>

  </div>

  <!-- ═══════════════ CONFIGURAÇÕES ═══════════════════════ -->
  <div id="tab-config" class="tab-panel fade-in">

    <div class="section-title" style="margin-bottom:20px">Configurações</div>

    <div class="card">
      <div class="card-title">Parâmetros Gerais</div>
      <div class="form-row">
        <div>
          <div class="form-label">Modelo Gemini</div>
          <select class="inp" id="cfgModelo" style="cursor:pointer">
            <option value="gemini-2.5-flash">gemini-2.5-flash</option>
            <option value="gemini-1.5-pro">gemini-1.5-pro</option>
            <option value="gemini-1.5-flash">gemini-1.5-flash</option>
          </select>
        </div>
        <div>
          <div class="form-label">Cidade (OpenWeather)</div>
          <input class="inp" id="cfgCidade" placeholder="Muriae,BR">
        </div>
      </div>
      <button class="btn btn-primary" onclick="salvarConfig()">Salvar Configurações</button>
    </div>

    <div class="card">
      <div class="card-title">Personalidade da Aurora</div>
      <div style="font-size:12px;color:var(--muted);margin-bottom:12px;font-family:var(--mono)">
        Texto que define o comportamento e personalidade da IA
      </div>
      <textarea class="inp" id="cfgPersonalidade" placeholder="Carregando..." style="min-height:160px"></textarea>
      <button class="btn btn-primary" onclick="salvarPersonalidade()">Salvar Personalidade</button>
    </div>

    <div class="card">
      <div class="card-title">Funções do Sistema</div>
      <div class="toggle-row">
        <div class="toggle-info">
          <div class="toggle-name">LED NeoPixel</div>
          <div class="toggle-desc">Indicador visual de estado do sistema</div>
        </div>
        <label class="toggle">
          <input type="checkbox" id="togLED" checked onchange="toggleFunc('led', this.checked)">
          <span class="slider"></span>
        </label>
      </div>
      <div class="toggle-row">
        <div class="toggle-info">
          <div class="toggle-name">OLED Display</div>
          <div class="toggle-desc">Exibição de dados no display físico</div>
        </div>
        <label class="toggle">
          <input type="checkbox" id="togOLED" checked onchange="toggleFunc('oled', this.checked)">
          <span class="slider"></span>
        </label>
      </div>
      <div class="toggle-row">
        <div class="toggle-info">
          <div class="toggle-name">Alertas Climáticos</div>
          <div class="toggle-desc">Notificações Telegram de clima severo</div>
        </div>
        <label class="toggle">
          <input type="checkbox" id="togAlertas" checked onchange="toggleFunc('alertas', this.checked)">
          <span class="slider"></span>
        </label>
      </div>
      <div class="toggle-row">
        <div class="toggle-info">
          <div class="toggle-name">Modo Noturno</div>
          <div class="toggle-desc">Silencia Telegram no intervalo configurado abaixo</div>
        </div>
        <label class="toggle">
          <input type="checkbox" id="togNoturno" checked onchange="toggleFunc('noturno', this.checked)">
          <span class="slider"></span>
        </label>
      </div>
      <div class="form-row" style="margin-top:12px">
        <div>
          <div class="form-label">Início do modo noturno</div>
          <select class="inp" id="cfgNoturnoInicio"></select>
        </div>
        <div>
          <div class="form-label">Fim do modo noturno</div>
          <select class="inp" id="cfgNoturnoFim"></select>
        </div>
      </div>
      <div class="btn-row">
        <button class="btn btn-primary" onclick="salvarHorarioNoturno()">Salvar Horário Noturno</button>
      </div>
    </div>

    <div class="card">
      <div class="card-title">Cores do LED</div>
      <div class="form-row">
        <div><div class="form-label">WiFi conectando</div><input class="inp" type="color" id="ledWifi"><input class="inp" id="ledWifiHex" maxlength="7" placeholder="#0000ff"></div>
        <div><div class="form-label">Modo idle</div><input class="inp" type="color" id="ledIdle"><input class="inp" id="ledIdleHex" maxlength="7" placeholder="#00aaff"></div>
      </div>
      <div class="form-row">
        <div><div class="form-label">Processando IA</div><input class="inp" type="color" id="ledProcessing"><input class="inp" id="ledProcessingHex" maxlength="7" placeholder="#ff7700"></div>
        <div><div class="form-label">Sucesso</div><input class="inp" type="color" id="ledSuccess"><input class="inp" id="ledSuccessHex" maxlength="7" placeholder="#00ff00"></div>
      </div>
      <div>
        <div class="form-label">Erro</div>
        <input class="inp" type="color" id="ledError"><input class="inp" id="ledErrorHex" maxlength="7" placeholder="#ff0000">
      </div>
      <button class="btn btn-primary" onclick="salvarCoresLED()">Salvar Cores do LED</button>
    </div>

  </div>

  <!-- ═══════════════ CONTROLE ════════════════════════════ -->
  <div id="tab-controle" class="tab-panel fade-in">

    <div class="section-title" style="margin-bottom:20px">Painel de Controle</div>

    <div class="card">
      <div class="card-title">Ações do Sistema</div>
      <div class="btn-row">
        <button class="btn btn-warn" onclick="confirmar('Reiniciar ESP32?', cmdReset)">
          ↺ Reiniciar
        </button>
        <button class="btn btn-secondary" onclick="cmdOTA()">
          📡 Ativar OTA
        </button>
        <button class="btn btn-secondary" onclick="cmdRelatorio()">
          📊 Gerar Relatório
        </button>
      </div>
    </div>

    <div class="card">
      <div class="card-title">Limpeza de Dados</div>
      <div class="btn-row">
        <button class="btn btn-danger btn-sm" onclick="confirmar('Apagar histórico da IA?', cmdLimparIA)">
          🗑 Histórico IA
        </button>
        <button class="btn btn-danger btn-sm" onclick="confirmar('Apagar todos os logs?', cmdLimparLogs)">
          🗑 Logs SD
        </button>
        <button class="btn btn-danger btn-sm" onclick="confirmar('Apagar memória de perguntas?', cmdLimparMem)">
          🗑 Memória Perguntas
        </button>
      </div>
    </div>

    <div class="card">
      <div class="card-title">Console Serial (últimas linhas)</div>
      <div class="log-box" id="logBox">
        <span class="log-line ok">Sistema pronto.</span>
      </div>
      <div class="btn-row">
        <button class="btn btn-secondary btn-sm" onclick="fetchLog()">↻ Atualizar</button>
        <button class="btn btn-secondary btn-sm" onclick="document.getElementById('logBox').innerHTML=''">Limpar</button>
      </div>
    </div>

    <div class="card">
      <div class="card-title">Relatório Gerado</div>
      <div id="relatorioBox" class="log-box" style="min-height:60px;white-space:pre-wrap;display:none"></div>
    </div>

  </div>
  </div><!-- /content -->

</div><!-- /app -->

<!-- ════════════════ EDITOR MODAL ════════════════════════ -->
<div class="modal-overlay" id="editorModal">
  <div class="modal">
    <div class="modal-header">
      <div class="modal-title" id="editorTitle">/aurora/config/arquivo.txt</div>
      <button class="btn btn-secondary btn-sm" onclick="closeEditor()">✕ Fechar</button>
    </div>
    <div class="modal-body">
      <textarea id="editorArea" spellcheck="false"></textarea>
    </div>
    <div class="modal-footer">
      <button class="btn btn-danger btn-sm" onclick="deletarArquivoAtual()">🗑 Deletar</button>
      <button class="btn btn-primary" onclick="salvarArquivoEditor()">💾 Salvar</button>
    </div>
  </div>
</div>

<!-- ════════════════ TOAST ════════════════════════════════ -->
<div id="toast"></div>

<script>
var loggedIn = false;
var currentFile = '';
var refreshTimer = null;

function request(path, opts, cb){
  opts = opts || {};
  cb = cb || function(){};
  var xhr = new XMLHttpRequest();
  xhr.open(opts.method || 'GET', path, true);
  xhr.setRequestHeader('Accept', 'application/json');
  if(opts.headers){
    for(var k in opts.headers){ if(opts.headers.hasOwnProperty(k)) xhr.setRequestHeader(k, opts.headers[k]); }
  }
  xhr.onreadystatechange = function(){
    if(xhr.readyState !== 4) return;
    if(xhr.status === 401){ doLogout(); cb(null); return; }
    if(xhr.status < 200 || xhr.status >= 300){ cb(null); return; }
    try { cb(JSON.parse(xhr.responseText)); }
    catch(e){ cb(null); }
  };
  xhr.onerror = function(){ cb(null); };
  xhr.send(opts.body || null);
}

function doLogin(){
  var pin = document.getElementById('loginPin').value;
  request('/api/login', {
    method:'POST',
    headers:{'Content-Type':'application/json'},
    body: JSON.stringify({pin: pin})
  }, function(d){
    if(d && d.ok){
      loggedIn = true;
      document.getElementById('loginScreen').style.display = 'none';
      document.getElementById('app').style.display = 'block';
      startApp();
    } else {
      document.getElementById('loginErr').textContent = 'Senha incorreta.';
      document.getElementById('loginPin').value = '';
    }
  });
}

function doLogout(){
  request('/api/logout', {method:'POST'}, function(){});
  loggedIn = false;
  clearInterval(refreshTimer);
  document.getElementById('app').style.display = 'none';
  document.getElementById('loginScreen').style.display = 'flex';
  document.getElementById('loginPin').value = '';
  document.getElementById('loginErr').textContent = '';
}

function startApp(){
  preencherHoras();
  prepararCamposCor();
  refreshDash();
  loadClima();
  loadFiles('/aurora');
  loadConfig();
  refreshTimer = setInterval(function(){ if(loggedIn) refreshDash(); }, 12000);
  setInterval(function(){
    var n = new Date();
    document.getElementById('topTime').textContent = pad2(n.getHours()) + ':' + pad2(n.getMinutes());
  }, 1000);
}

function showTab(name){
  var i;
  var panels = document.querySelectorAll('.tab-panel');
  for(i=0;i<panels.length;i++) panels[i].style.display = 'none';
  var tabs = document.querySelectorAll('.nav-tab');
  for(i=0;i<tabs.length;i++) tabs[i].className = tabs[i].className.replace(' active', '');
  var tab = document.getElementById('tab-' + name);
  if(tab) tab.style.display = 'block';
  if(window.event && window.event.currentTarget) window.event.currentTarget.className += ' active';
  if(name==='clima') loadClima();
  if(name==='sdcard') loadFiles('/aurora');
  if(name==='config') loadConfig();
}

function refreshDash(){
  var el = document.getElementById('refreshIcon');
  el.textContent = '⟳';
  request('/api/status', {}, function(d){
    el.textContent = '↻';
    if(!d) return;
    var t = Number(d.chipTemp || 0);
    var tEl = document.getElementById('statChipTemp');
    tEl.innerHTML = t.toFixed(1) + '<span class="stat-unit">°C</span>';
    tEl.className = 'stat-value' + (t > 75 ? ' bad' : t > 55 ? ' warn' : ' ok');

    var hp = Number(d.heapPct || 0);
    var hEl = document.getElementById('statHeap');
    hEl.innerHTML = hp + '<span class="stat-unit">%</span>';
    hEl.className = 'stat-value' + (hp < 20 ? ' bad' : hp < 40 ? ' warn' : ' ok');
    document.getElementById('heapPct').textContent = hp + '%';
    var hBar = document.getElementById('heapBar');
    hBar.style.width = hp + '%';

    var rssi = Number(d.rssi || 0);
    var wp = Number(d.wifiPct || 0);
    var wEl = document.getElementById('statWifi');
    wEl.innerHTML = rssi + '<span class="stat-unit">dBm</span>';
    wEl.className = 'stat-value' + (wp < 30 ? ' bad' : wp < 60 ? ' warn' : ' ok');
    document.getElementById('wifiPct').textContent = wp + '%';
    var wBar = document.getElementById('wifiBar');
    wBar.style.width = wp + '%';

    document.getElementById('statUptime').textContent = d.uptime || '--';
    document.getElementById('statQuestions').textContent = d.questions || '0';
    document.getElementById('statClimaTemp').innerHTML = Number(d.climaTemp || 0).toFixed(1) + '<span class="stat-unit">°C</span>';
    document.getElementById('infoIP').textContent = d.ip || '--';
    document.getElementById('infoSSID').textContent = d.ssid || '--';
    document.getElementById('infoModelo').textContent = d.modelo || '--';
    document.getElementById('infoCidade').textContent = d.cidade || '--';
    document.getElementById('infoSD').textContent = d.sdOK ? '✓ OK' : '✗ Falha';
    document.getElementById('infoOTA').textContent = d.otaAtivo ? '● Ativo' : 'Inativo';
  });
}

function loadClima(){
  request('/api/clima', {}, function(d){
    if(!d) return;
    document.getElementById('climaCidade').textContent = d.cidade || '--';
    document.getElementById('cTemp').innerHTML = Number(d.temp || 0).toFixed(1) + '<span class="stat-unit">°C</span>';
    document.getElementById('cSens').innerHTML = Number(d.sensTermica || 0).toFixed(1) + '<span class="stat-unit">°C</span>';
    document.getElementById('cHum').innerHTML = Number(d.umidade || 0) + '<span class="stat-unit">%</span>';
    document.getElementById('cPressao').innerHTML = Number(d.pressao || 0) + '<span class="stat-unit">hPa</span>';
    document.getElementById('cVento').innerHTML = Number(d.vento || 0).toFixed(1) + '<span class="stat-unit">km/h</span>';
    document.getElementById('cDesc').textContent = d.descricao || '--';
    if(d.prev){
      document.getElementById('ph1').textContent = d.prev.h1 || '+3h';
      document.getElementById('ph2').textContent = d.prev.h2 || '+6h';
      document.getElementById('ph3').textContent = d.prev.h3 || '+9h';
      document.getElementById('pt1').textContent = Math.round(d.prev.t1 || 0) + '°';
      document.getElementById('pt2').textContent = Math.round(d.prev.t2 || 0) + '°';
      document.getElementById('pt3').textContent = Math.round(d.prev.t3 || 0) + '°';
      document.getElementById('pr1').textContent = Math.round(d.prev.r1 || 0) + '%';
      document.getElementById('pr2').textContent = Math.round(d.prev.r2 || 0) + '%';
      document.getElementById('pr3').textContent = Math.round(d.prev.r3 || 0) + '%';
    }
    document.getElementById('alertaCard').style.display = d.alertaAtivo ? 'block' : 'none';
    document.getElementById('alertaMsg').textContent = d.alertaMsg || '';
  });
}

function atualizarClima(){
  toast('Atualizando clima...', 'info');
  request('/api/clima/update', {method:'POST'}, function(d){
    if(d && d.ok){ toast('Clima atualizado!', 'success'); loadClima(); }
    else toast('Erro ao atualizar.', 'error');
  });
}

function loadFiles(path){
  var tree = document.getElementById('fileTree');
  tree.innerHTML = '<div style="color:var(--muted);font-family:var(--mono);font-size:13px;padding:12px">Carregando...</div>';
  request('/api/sd/list?path=' + encodeURIComponent(path), {}, function(d){
    if(!d){ tree.innerHTML = '<div style="color:var(--red);padding:12px">Erro ao ler SD</div>'; return; }
    var html = '';
    if(path !== '/'){
      var parent = path.substring(0, path.lastIndexOf('/')) || '/';
      html += '<div class="file-dir" data-path="' + escHtml(parent) + '">← voltar</div>';
    }
    var i, f;
    if(d.dirs){
      for(i=0; i<d.dirs.length; i++){
        html += '<div class="file-dir" data-path="' + escHtml(d.dirs[i].path) + '">📁 ' + escHtml(d.dirs[i].name) + '</div>';
      }
    }
    if(d.files){
      for(i=0; i<d.files.length; i++){
        f = d.files[i];
        html += '<div class="file-entry" data-file="' + escHtml(f.path) + '">' +
          '<span class="file-name">📄 ' + escHtml(f.name) + '</span>' +
          '<span class="file-size">' + formatBytes(f.size) + '</span>' +
          '<div class="file-actions"><button class="file-btn" data-action="edit">Editar</button><button class="file-btn del" data-action="del">Del</button></div></div>';
      }
    }
    tree.innerHTML = html || '<div style="color:var(--muted);padding:12px">Pasta vazia.</div>';

    var dirs = tree.querySelectorAll('.file-dir[data-path]');
    for(i=0; i<dirs.length; i++) dirs[i].onclick = function(){ loadFiles(this.getAttribute('data-path')); };

    var files = tree.querySelectorAll('.file-entry[data-file]');
    for(i=0; i<files.length; i++){
      files[i].onclick = function(ev){
        var e = ev || window.event;
        var target = e.target || e.srcElement;
        var pathFile = this.getAttribute('data-file');
        if(target && target.getAttribute('data-action') === 'edit'){
          if(e.stopPropagation) e.stopPropagation();
          editarArquivo(pathFile);
        } else if(target && target.getAttribute('data-action') === 'del'){
          if(e.stopPropagation) e.stopPropagation();
          confirmar('Deletar arquivo?', function(){ deletarArquivo(pathFile); });
        }
      };
    }
  });
}

function editarArquivo(path){
  document.getElementById('editorTitle').textContent = path;
  document.getElementById('editorArea').value = 'Carregando...';
  currentFile = path;
  document.getElementById('editorModal').className = 'modal-overlay open';
  request('/api/sd/read?path=' + encodeURIComponent(path), {}, function(d){
    document.getElementById('editorArea').value = d ? (d.content || '') : '(erro ao ler arquivo)';
  });
}

function salvarArquivoEditor(){
  var content = document.getElementById('editorArea').value;
  request('/api/sd/write', {method:'POST', headers:{'Content-Type':'application/json'}, body: JSON.stringify({path: currentFile, content: content})}, function(d){
    if(d && d.ok){ toast('Arquivo salvo!', 'success'); closeEditor(); }
    else toast('Erro ao salvar.', 'error');
  });
}

function deletarArquivoAtual(){ confirmar('Deletar este arquivo permanentemente?', function(){ closeEditor(); deletarArquivo(currentFile); }); }

function deletarArquivo(path){
  request('/api/sd/delete', {method:'POST', headers:{'Content-Type':'application/json'}, body: JSON.stringify({path:path})}, function(d){
    if(d && d.ok){ toast('Arquivo deletado.', 'success'); loadFiles(path.substring(0, path.lastIndexOf('/'))||'/'); }
    else toast('Erro ao deletar.', 'error');
  });
}

function criarArquivo(){
  var path = document.getElementById('newFilePath').value.replace(/^\s+|\s+$/g, '');
  var content = document.getElementById('newFileContent').value;
  if(!path){ toast('Informe o caminho do arquivo.', 'error'); return; }
  request('/api/sd/write', {method:'POST', headers:{'Content-Type':'application/json'}, body: JSON.stringify({path:path, content:content})}, function(d){
    if(d && d.ok){
      toast('Arquivo criado!', 'success');
      document.getElementById('newFilePath').value = '';
      document.getElementById('newFileContent').value = '';
      loadFiles(path.substring(0, path.lastIndexOf('/')) || '/');
    } else toast('Erro ao criar arquivo.', 'error');
  });
}

function closeEditor(){ document.getElementById('editorModal').className = 'modal-overlay'; }

function loadConfig(){
  request('/api/config', {}, function(d){
    if(!d) return;
    document.getElementById('cfgModelo').value = d.modelo || 'gemini-2.5-flash';
    document.getElementById('cfgCidade').value = d.cidade || 'Muriae,BR';
    document.getElementById('cfgPersonalidade').value = d.personalidade || '';
    document.getElementById('togLED').checked = !d.ledDesabilitado;
    document.getElementById('togOLED').checked = !d.oledDesabilitado;
    document.getElementById('togAlertas').checked = !d.alertasDesabilitados;
    document.getElementById('togNoturno').checked = !d.noturnoDesabilitado;
    document.getElementById('cfgNoturnoInicio').value = String(d.noturnoInicio != null ? d.noturnoInicio : 22);
    document.getElementById('cfgNoturnoFim').value = String(d.noturnoFim != null ? d.noturnoFim : 8);

    preencherCor('ledWifi', 'ledWifiHex', d.ledColors && d.ledColors.wifi, '#000050');
    preencherCor('ledIdle', 'ledIdleHex', d.ledColors && d.ledColors.idle, '#0050ff');
    preencherCor('ledProcessing', 'ledProcessingHex', d.ledColors && d.ledColors.processing, '#ff7800');
    preencherCor('ledSuccess', 'ledSuccessHex', d.ledColors && d.ledColors.success, '#00b400');
    preencherCor('ledError', 'ledErrorHex', d.ledColors && d.ledColors.error, '#b40000');
  });
}

function salvarConfig(callback){
  var payload = {
    modelo: document.getElementById('cfgModelo').value,
    cidade: document.getElementById('cfgCidade').value,
    noturnoInicio: parseInt(document.getElementById('cfgNoturnoInicio').value, 10),
    noturnoFim: parseInt(document.getElementById('cfgNoturnoFim').value, 10),
    ledColors: {
      wifi: hexToRgb(corAtual('ledWifi', 'ledWifiHex')),
      idle: hexToRgb(corAtual('ledIdle', 'ledIdleHex')),
      processing: hexToRgb(corAtual('ledProcessing', 'ledProcessingHex')),
      success: hexToRgb(corAtual('ledSuccess', 'ledSuccessHex')),
      error: hexToRgb(corAtual('ledError', 'ledErrorHex'))
    }
  };
  request('/api/config', {method:'POST', headers:{'Content-Type':'application/json'}, body: JSON.stringify(payload)}, function(d){
    if(callback) callback(d && d.ok);
    if(!callback){ if(d && d.ok) toast('Configurações salvas!', 'success'); else toast('Erro ao salvar.', 'error'); }
  });
}

function salvarHorarioNoturno(){
  salvarConfig(function(ok){ toast(ok ? 'Horário noturno salvo!' : 'Erro ao salvar horário.', ok ? 'success' : 'error'); });
}

function salvarCoresLED(){
  salvarConfig(function(ok){ toast(ok ? 'Cores do LED salvas!' : 'Erro ao salvar cores.', ok ? 'success' : 'error'); });
}

function salvarPersonalidade(){
  var texto = document.getElementById('cfgPersonalidade').value;
  request('/api/personalidade', {method:'POST', headers:{'Content-Type':'application/json'}, body: JSON.stringify({texto:texto})}, function(d){
    if(d && d.ok) toast('Personalidade salva!', 'success');
    else toast('Erro ao salvar.', 'error');
  });
}

function toggleFunc(name, enabled){
  request('/api/toggle', {method:'POST', headers:{'Content-Type':'application/json'}, body: JSON.stringify({name:name, enabled:enabled})}, function(){
    toast((enabled ? '✓ ' : '✗ ') + name + ' ' + (enabled ? 'ativado' : 'desativado'), 'info');
  });
}

function cmdReset(){ request('/api/cmd/reset', {method:'POST'}, function(){}); toast('Reiniciando...', 'warn'); }
function cmdOTA(){ request('/api/cmd/ota', {method:'POST'}, function(d){ toast(d && d.ok ? 'OTA ativo. IP: ' + d.ip : 'Erro ao ativar OTA.', d && d.ok ? 'success' : 'error'); }); }
function cmdRelatorio(){ request('/api/cmd/relatorio', {method:'POST'}, function(d){ var box=document.getElementById('relatorioBox'); if(d && d.relatorio){ box.textContent=d.relatorio; box.style.display='block'; toast('Relatório gerado!', 'success'); } else toast('Erro ao gerar.', 'error'); }); }
function cmdLimparIA(){ request('/api/cmd/limpar-ia', {method:'POST'}, function(d){ toast(d&&d.ok?'Histórico IA apagado.':'Erro.', d&&d.ok?'success':'error'); }); }
function cmdLimparLogs(){ request('/api/cmd/limpar-logs', {method:'POST'}, function(d){ toast(d&&d.ok?'Logs apagados.':'Erro.', d&&d.ok?'success':'error'); }); }
function cmdLimparMem(){ request('/api/cmd/limpar-mem', {method:'POST'}, function(d){ toast(d&&d.ok?'Memória apagada.':'Erro.', d&&d.ok?'success':'error'); }); }

function fetchLog(){
  request('/api/log', {}, function(d){
    if(!d || !d.lines) return;
    var lines = [];
    for(var i=0;i<d.lines.length;i++){
      var l = d.lines[i] || '';
      var cl = (l.indexOf('Erro')>=0 || l.indexOf('erro')>=0 || l.indexOf('✗')>=0) ? 'err' :
               (l.indexOf('OK')>=0 || l.indexOf('✓')>=0 || l.indexOf('ok')>=0) ? 'ok' :
               (l.indexOf('Warn')>=0 ? 'warn' : '');
      lines.push('<span class="log-line '+cl+'">'+escHtml(l)+'</span>');
    }
    var box = document.getElementById('logBox');
    box.innerHTML = lines.join('\\n');
    box.scrollTop = box.scrollHeight;
  });
}

function confirmar(msg, cb){ if(confirm(msg)) cb(); }
function toast(msg, type){
  type = type || 'info';
  var wrap = document.getElementById('toast');
  var el = document.createElement('div');
  el.className = 'toast-item ' + type;
  el.appendChild(document.createTextNode(msg));
  wrap.appendChild(el);
  setTimeout(function(){ if(el.parentNode) el.parentNode.removeChild(el); }, 3000);
}

function preencherHoras(){
  var ini = document.getElementById('cfgNoturnoInicio');
  var fim = document.getElementById('cfgNoturnoFim');
  if(ini.options.length) return;
  for(var h=0; h<24; h++){
    var txt = pad2(h) + ':00';
    ini.options.add(new Option(txt, String(h)));
    fim.options.add(new Option(txt, String(h)));
  }
}

function prepararCamposCor(){
  vincularCor('ledWifi', 'ledWifiHex');
  vincularCor('ledIdle', 'ledIdleHex');
  vincularCor('ledProcessing', 'ledProcessingHex');
  vincularCor('ledSuccess', 'ledSuccessHex');
  vincularCor('ledError', 'ledErrorHex');
}

function vincularCor(idColor, idHex){
  var c = document.getElementById(idColor);
  var h = document.getElementById(idHex);
  if(c) c.onchange = function(){ h.value = normalizarHex(c.value); };
  if(h) h.onkeyup = function(){ var v = normalizarHex(h.value); if(v.length === 7 && c) c.value = v; };
}

function preencherCor(idColor, idHex, rgb, fallback){
  var hex = rgb ? rgbToHex(rgb) : fallback;
  document.getElementById(idColor).value = hex;
  document.getElementById(idHex).value = hex;
}

function corAtual(idColor, idHex){
  var hex = normalizarHex(document.getElementById(idHex).value || document.getElementById(idColor).value);
  document.getElementById(idHex).value = hex;
  document.getElementById(idColor).value = hex;
  return hex;
}

function normalizarHex(hex){
  var h = String(hex || '').replace(/[^0-9a-fA-F]/g, '');
  if(h.length < 6) h = (h + '000000').substring(0, 6);
  if(h.length > 6) h = h.substring(0, 6);
  return '#' + h.toLowerCase();
}

function hexToRgb(hex){
  var h = normalizarHex(hex).replace('#', '');
  return { r: parseInt(h.substring(0,2), 16), g: parseInt(h.substring(2,4), 16), b: parseInt(h.substring(4,6), 16) };
}
function rgbToHex(c){ return '#' + toHex(c.r || 0) + toHex(c.g || 0) + toHex(c.b || 0); }
function toHex(v){ var s = Number(v).toString(16); return s.length < 2 ? '0' + s : s; }
function pad2(v){ return v < 10 ? '0' + v : String(v); }
function formatBytes(b){ if(b<1024) return b+'B'; if(b<1048576) return (b/1024).toFixed(1)+'KB'; return (b/1048576).toFixed(1)+'MB'; }
function escHtml(s){ s=String(s||''); return s.replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;'); }
</script>
</body>
</html>
)rawhtml";

#endif // WEB_PANEL_H
