#ifndef WEB_PANEL_H
#define WEB_PANEL_H

const char AURORA_HTML[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1">
<meta name="apple-mobile-web-app-capable" content="yes">
<meta name="theme-color" content="#0d1117">
<title>Aurora</title>
<style>
*{-webkit-box-sizing:border-box;box-sizing:border-box;margin:0;padding:0}
:root{
  --bg:#0d1117;--s1:#161b22;--s2:#21262d;
  --bd:#30363d;--bd2:#3d444d;
  --ac:#58a6ff;--ac2:#3fb950;--ac3:#f78166;--ac4:#d29922;--ac5:#bc8cff;
  --tx:#e6edf3;--mu:#8b949e;
  --r:10px;--mono:'SFMono-Regular',Consolas,'Liberation Mono',monospace;
  --sans:-apple-system,BlinkMacSystemFont,'Segoe UI',Helvetica,Arial,sans-serif;
}
html{-webkit-text-size-adjust:100%;scroll-behavior:smooth}
body{background:var(--bg);color:var(--tx);font-family:var(--sans);font-size:14px;line-height:1.5;min-height:100vh;overflow-x:hidden}

/* ── LOGIN ───────────────────────────────── */
#ls{position:fixed;inset:0;top:0;right:0;bottom:0;left:0;display:-webkit-flex;display:flex;-webkit-align-items:center;align-items:center;-webkit-justify-content:center;justify-content:center;background:var(--bg);z-index:200}
.lc{background:var(--s1);border:1px solid var(--bd);border-radius:16px;padding:40px 32px;width:320px;max-width:92vw;text-align:center}
.lc .ico{font-size:40px;margin-bottom:16px}
.lc h1{font-size:22px;font-weight:700;color:var(--tx);margin-bottom:4px}
.lc p{color:var(--mu);font-size:13px;margin-bottom:24px}
.lc input{width:100%;background:var(--s2);border:1px solid var(--bd);border-radius:var(--r);padding:12px;color:var(--tx);font-family:var(--mono);font-size:20px;text-align:center;letter-spacing:6px;outline:none;-webkit-transition:border-color .2s;transition:border-color .2s;margin-bottom:12px;-webkit-appearance:none;appearance:none}
.lc input:focus{border-color:var(--ac)}
.lc .eb{color:var(--ac3);font-size:12px;min-height:18px;margin-top:8px}

/* ── LAYOUT ──────────────────────────────── */
#app{display:none}
.topbar{position:-webkit-sticky;position:sticky;top:0;background:rgba(13,17,23,.95);border-bottom:1px solid var(--bd);height:52px;display:-webkit-flex;display:flex;-webkit-align-items:center;align-items:center;padding:0 16px;gap:10px;z-index:50;backdrop-filter:blur(8px);-webkit-backdrop-filter:blur(8px)}
.brand{font-size:15px;font-weight:700;color:var(--tx);-webkit-flex:1;flex:1;display:-webkit-flex;display:flex;-webkit-align-items:center;align-items:center;gap:8px}
.brand span{color:var(--mu);font-weight:400;font-size:12px}
.dot{width:8px;height:8px;border-radius:50%;background:var(--ac2);-webkit-animation:pulse 2s infinite;animation:pulse 2s infinite}
.ttime{font-family:var(--mono);font-size:12px;color:var(--mu)}
.tbtn{background:none;border:1px solid var(--bd);border-radius:6px;color:var(--mu);font-size:11px;font-weight:600;padding:5px 10px;cursor:pointer;text-transform:uppercase;letter-spacing:.5px;white-space:nowrap;font-family:var(--sans)}
.tbtn:hover{border-color:var(--bd2);color:var(--tx)}
@-webkit-keyframes pulse{0%,100%{opacity:1}50%{opacity:.3}}
@keyframes pulse{0%,100%{opacity:1}50%{opacity:.3}}

/* ── NAV ─────────────────────────────────── */
.nav{display:-webkit-flex;display:flex;border-bottom:1px solid var(--bd);overflow-x:auto;-webkit-overflow-scrolling:touch;scrollbar-width:none}
.nav::-webkit-scrollbar{display:none}
.nt{background:none;border:none;border-bottom:2px solid transparent;color:var(--mu);font-family:var(--sans);font-size:13px;font-weight:500;padding:12px 16px;cursor:pointer;white-space:nowrap;-webkit-transition:color .15s,border-color .15s;transition:color .15s,border-color .15s;-webkit-flex-shrink:0;flex-shrink:0}
.nt:hover{color:var(--tx)}
.nt.on{color:var(--ac);border-bottom-color:var(--ac)}

/* ── CONTENT ─────────────────────────────── */
.wrap{padding:20px;max-width:1100px;margin:0 auto}
.tp{display:none}.tp.on{display:block}
.sh{display:-webkit-flex;display:flex;-webkit-align-items:center;align-items:center;-webkit-justify-content:space-between;justify-content:space-between;margin-bottom:16px;gap:8px;-webkit-flex-wrap:wrap;flex-wrap:wrap}
.st{font-size:17px;font-weight:600}

/* ── CARDS ───────────────────────────────── */
.card{background:var(--s1);border:1px solid var(--bd);border-radius:var(--r);padding:16px;margin-bottom:12px}
.ct{font-size:11px;font-weight:600;letter-spacing:2px;text-transform:uppercase;color:var(--mu);margin-bottom:14px;display:-webkit-flex;display:flex;-webkit-align-items:center;align-items:center;gap:8px}
.ct::after{content:'';-webkit-flex:1;flex:1;height:1px;background:var(--bd)}

/* ── STAT GRID ───────────────────────────── */
.sg{display:-webkit-flex;display:flex;-webkit-flex-wrap:wrap;flex-wrap:wrap;gap:8px;margin-bottom:12px}
.sv{background:var(--s2);border:1px solid var(--bd);border-radius:var(--r);padding:14px;-webkit-flex:1 1 140px;flex:1 1 140px;min-width:120px;position:relative;overflow:hidden}
.sv::after{content:'';position:absolute;bottom:0;left:0;right:0;height:2px;background:var(--ac);opacity:.5}
.sl{font-size:11px;font-weight:600;letter-spacing:1.5px;text-transform:uppercase;color:var(--mu);margin-bottom:6px}
.sn{font-family:var(--mono);font-size:20px;font-weight:600;color:var(--tx);line-height:1}
.sn.ok{color:var(--ac2)}.sn.warn{color:var(--ac4)}.sn.bad{color:var(--ac3)}
.su{font-size:11px;color:var(--mu);margin-left:3px}

/* ── PROGRESS ────────────────────────────── */
.pw{margin:8px 0}
.pl{display:-webkit-flex;display:flex;-webkit-justify-content:space-between;justify-content:space-between;font-size:11px;color:var(--mu);margin-bottom:5px;font-family:var(--mono)}
.pb{height:5px;background:var(--s2);border-radius:3px;overflow:hidden}
.pf{height:100%;border-radius:3px;-webkit-transition:width .4s;transition:width .4s;background:var(--ac)}
.pf.w{background:var(--ac4)}.pf.b{background:var(--ac3)}

/* ── INPUTS ──────────────────────────────── */
.inp{width:100%;background:var(--s2);border:1px solid var(--bd);border-radius:8px;padding:10px 12px;color:var(--tx);font-family:var(--mono);font-size:13px;outline:none;-webkit-transition:border-color .2s;transition:border-color .2s;margin-bottom:10px;-webkit-appearance:none;appearance:none}
.inp:focus{border-color:var(--ac)}
select.inp{cursor:pointer;background-image:url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' width='10' height='6'%3E%3Cpath fill='%238b949e' d='M0 0l5 6 5-6'/%3E%3C/svg%3E");background-repeat:no-repeat;background-position:right 12px center;padding-right:30px}
textarea.inp{resize:vertical;min-height:100px;font-size:12px;line-height:1.6}
.fl{font-size:11px;font-weight:600;letter-spacing:1px;text-transform:uppercase;color:var(--mu);margin-bottom:5px}

/* ── BUTTONS ─────────────────────────────── */
.btn{display:-webkit-inline-flex;display:inline-flex;-webkit-align-items:center;align-items:center;gap:6px;padding:8px 16px;border-radius:6px;font-family:var(--sans);font-size:12px;font-weight:600;letter-spacing:.5px;cursor:pointer;border:none;-webkit-transition:opacity .15s,background .15s;transition:opacity .15s,background .15s;text-transform:uppercase;white-space:nowrap}
.btn-p{background:var(--ac);color:#0d1117}.btn-p:hover{opacity:.85}
.btn-s{background:var(--s2);color:var(--tx);border:1px solid var(--bd)}.btn-s:hover{background:var(--bd)}
.btn-d{background:rgba(247,129,102,.12);color:var(--ac3);border:1px solid rgba(247,129,102,.25)}.btn-d:hover{background:rgba(247,129,102,.22)}
.btn-w{background:rgba(210,153,34,.12);color:var(--ac4);border:1px solid rgba(210,153,34,.25)}.btn-w:hover{background:rgba(210,153,34,.22)}
.btn-g{background:rgba(63,185,80,.12);color:var(--ac2);border:1px solid rgba(63,185,80,.25)}.btn-g:hover{background:rgba(63,185,80,.22)}
.btn-sm{padding:5px 11px;font-size:11px}
.btn:disabled{opacity:.35;cursor:not-allowed}
.br{display:-webkit-flex;display:flex;-webkit-flex-wrap:wrap;flex-wrap:wrap;gap:6px;margin-top:10px}

/* ── FORM GRID ───────────────────────────── */
.fg{display:-webkit-flex;display:flex;-webkit-flex-wrap:wrap;flex-wrap:wrap;gap:10px}
.fg>*{-webkit-flex:1 1 180px;flex:1 1 180px;min-width:0}

/* ── TOGGLE ──────────────────────────────── */
.tr{display:-webkit-flex;display:flex;-webkit-align-items:center;align-items:center;-webkit-justify-content:space-between;justify-content:space-between;padding:12px 0;border-bottom:1px solid var(--bd)}
.tr:last-child{border:none}
.ti{-webkit-flex:1;flex:1;min-width:0}
.tn{font-size:13px;font-weight:600;margin-bottom:2px}
.td{font-size:11px;color:var(--mu);font-family:var(--mono)}
.tg{position:relative;width:40px;height:22px;-webkit-flex-shrink:0;flex-shrink:0}
.tg input{opacity:0;width:0;height:0;position:absolute}
.tslider{position:absolute;top:0;right:0;bottom:0;left:0;background:var(--bd2);border-radius:22px;cursor:pointer;-webkit-transition:background .2s;transition:background .2s}
.tslider::before{content:'';position:absolute;left:3px;bottom:3px;width:16px;height:16px;background:#fff;border-radius:50%;-webkit-transition:-webkit-transform .2s;transition:transform .2s}
.tg input:checked+.tslider{background:var(--ac2)}
.tg input:checked+.tslider::before{-webkit-transform:translateX(18px);transform:translateX(18px)}

/* ── FILE TREE ───────────────────────────── */
.ft{font-family:var(--mono);font-size:12px}
.fd,.fe{padding:8px 10px;border-radius:6px;cursor:pointer;display:-webkit-flex;display:flex;-webkit-align-items:center;align-items:center;gap:6px;-webkit-transition:background .1s;transition:background .1s}
.fd{color:var(--ac)}.fd:hover,.fe:hover{background:var(--s2)}
.fe{padding-left:28px;color:var(--tx);-webkit-justify-content:space-between;justify-content:space-between}
.fa{display:-webkit-flex;display:flex;gap:4px;opacity:0;-webkit-transition:opacity .1s;transition:opacity .1s}
.fe:hover .fa{opacity:1}
@media(hover:none){.fa{opacity:1}}
.fb{padding:3px 8px;border-radius:4px;border:1px solid var(--bd);background:none;color:var(--mu);font-size:11px;cursor:pointer;font-family:var(--mono)}
.fb:hover{color:var(--ac);border-color:var(--ac)}
.fb.del:hover{color:var(--ac3);border-color:var(--ac3)}
.fsz{color:var(--mu);font-size:11px;margin-left:auto;margin-right:8px}

/* ── WEATHER ─────────────────────────────── */
.wg{display:-webkit-flex;display:flex;-webkit-flex-wrap:wrap;flex-wrap:wrap;gap:8px;margin-top:10px}
.wc{background:var(--s2);border:1px solid var(--bd);border-radius:var(--r);padding:12px;text-align:center;-webkit-flex:1 1 90px;flex:1 1 90px;min-width:80px}
.wh{font-size:11px;color:var(--mu);margin-bottom:4px;font-family:var(--mono)}
.wt{font-size:20px;font-weight:700;font-family:var(--mono)}
.wr{font-size:12px;color:var(--ac);margin-top:4px;font-family:var(--mono)}

/* ── MODAL ───────────────────────────────── */
.mo{position:fixed;top:0;right:0;bottom:0;left:0;background:rgba(0,0,0,.7);z-index:300;display:none;-webkit-align-items:center;align-items:center;-webkit-justify-content:center;justify-content:center;padding:16px}
.mo.on{display:-webkit-flex;display:flex}
.md{background:var(--s1);border:1px solid var(--bd);border-radius:14px;width:100%;max-width:680px;max-height:88vh;display:-webkit-flex;display:flex;-webkit-flex-direction:column;flex-direction:column;overflow:hidden}
.mh{padding:14px 16px;border-bottom:1px solid var(--bd);display:-webkit-flex;display:flex;-webkit-align-items:center;align-items:center;gap:10px}
.mt{font-family:var(--mono);font-size:12px;-webkit-flex:1;flex:1;color:var(--ac)}
.mb{-webkit-flex:1;flex:1;overflow:auto;padding:14px;-webkit-overflow-scrolling:touch}
.mb textarea{width:100%;min-height:260px;background:var(--s2);border:1px solid var(--bd);border-radius:8px;color:var(--tx);font-family:var(--mono);font-size:12px;line-height:1.6;padding:12px;resize:none;outline:none;-webkit-appearance:none;appearance:none}
.mf{padding:10px 16px;border-top:1px solid var(--bd);display:-webkit-flex;display:flex;-webkit-justify-content:flex-end;justify-content:flex-end;gap:8px}

/* ── LOG BOX ─────────────────────────────── */
.lb{background:var(--s2);border:1px solid var(--bd);border-radius:8px;padding:12px;font-family:var(--mono);font-size:11px;line-height:1.8;max-height:240px;overflow-y:auto;color:var(--tx);-webkit-overflow-scrolling:touch}
.ll{display:block}.ll.e{color:var(--ac3)}.ll.w{color:var(--ac4)}.ll.k{color:var(--ac2)}

/* ── ALERT BANNER ────────────────────────── */
.ab{background:rgba(247,129,102,.08);border:1px solid rgba(247,129,102,.3);border-radius:var(--r);padding:12px 14px;font-family:var(--mono);font-size:12px;line-height:1.6;color:var(--ac3);display:none}

/* ── INFO GRID ───────────────────────────── */
.ig{font-family:var(--mono);font-size:12px;line-height:2}
.ig .k{color:var(--mu)}.ig .v{color:var(--tx)}.ig .va{color:var(--ac)}

/* ── TOAST ───────────────────────────────── */
#toast{position:fixed;bottom:16px;right:16px;z-index:999;display:-webkit-flex;display:flex;-webkit-flex-direction:column;flex-direction:column;gap:6px;max-width:calc(100vw - 32px);pointer-events:none}
.ti2{background:var(--s1);border:1px solid var(--bd);border-radius:8px;padding:10px 14px;font-size:12px;pointer-events:all;-webkit-animation:sli .2s;animation:sli .2s}
.ti2.s{border-left:3px solid var(--ac2)}.ti2.e{border-left:3px solid var(--ac3)}.ti2.i{border-left:3px solid var(--ac)}.ti2.w{border-left:3px solid var(--ac4)}
@-webkit-keyframes sli{from{-webkit-transform:translateX(40px);opacity:0}to{-webkit-transform:none;opacity:1}}
@keyframes sli{from{transform:translateX(40px);opacity:0}to{transform:none;opacity:1}}

/* ── SCROLLBAR ───────────────────────────── */
::-webkit-scrollbar{width:4px;height:4px}
::-webkit-scrollbar-track{background:var(--bg)}
::-webkit-scrollbar-thumb{background:var(--bd2);border-radius:2px}

/* ── RESPONSIVE ──────────────────────────── */
@media(max-width:480px){.wrap{padding:12px}.topbar{padding:0 12px}.sv{-webkit-flex-basis:calc(50% - 4px);flex-basis:calc(50% - 4px)}}
</style>
</head>
<body>

<!-- LOGIN -->
<div id="ls">
<div class="lc">
  <div class="ico">🤖</div>
  <h1>Aurora</h1>
  <p>ESP32-S3 · AI Assistant</p>
  <input type="password" id="lpin" maxlength="8" placeholder="••••" autocomplete="off"
         onkeydown="if(event.key==='Enter'||event.keyCode===13)doLogin()">
  <button class="btn btn-p" style="width:100%" onclick="doLogin()">Entrar</button>
  <div class="eb" id="lerr"></div>
</div>
</div>

<!-- APP -->
<div id="app">
<div class="topbar">
  <div class="brand">Aurora <span>· ESP32-S3</span></div>
  <div class="dot" id="cdot"></div>
  <div class="ttime" id="ttm">--:--</div>
  <button class="tbtn" id="mlbtn" onclick="toggleLeve()">Leve</button>
  <button class="tbtn" onclick="doLogout()">Sair</button>
</div>

<div class="nav">
  <button class="nt on" onclick="showTab('dash',this)">Dashboard</button>
  <button class="nt" onclick="showTab('clima',this)">Clima</button>
  <button class="nt" onclick="showTab('sd',this)">SD Card</button>
  <button class="nt" onclick="showTab('cfg',this)">Config</button>
  <button class="nt" onclick="showTab('ctrl',this)">Controle</button>
</div>

<div class="wrap">

<!-- DASHBOARD -->
<div id="tab-dash" class="tp on">
  <div class="sh">
    <div class="st">Visão Geral</div>
    <button class="btn btn-s btn-sm" onclick="rdash()"><span id="rico">↻</span> Atualizar</button>
  </div>
  <div class="sg">
    <div class="sv"><div class="sl">Chip</div><div class="sn" id="schip">--<span class="su">°C</span></div></div>
    <div class="sv"><div class="sl">Heap</div><div class="sn" id="sheap">--<span class="su">%</span></div></div>
    <div class="sv"><div class="sl">WiFi</div><div class="sn" id="swifi">--<span class="su">dBm</span></div></div>
    <div class="sv"><div class="sl">Uptime</div><div class="sn" id="sut" style="font-size:15px">--</div></div>
    <div class="sv"><div class="sl">IA Pergs.</div><div class="sn" id="sqst">--</div></div>
    <div class="sv"><div class="sl">Clima</div><div class="sn" id="scli">--<span class="su">°C</span></div></div>
  </div>
  <div class="card">
    <div class="ct">Recursos</div>
    <div class="pw"><div class="pl"><span>Heap RAM</span><span id="hpct">--%</span></div><div class="pb"><div class="pf" id="hbar" style="width:0"></div></div></div>
    <div class="pw" style="margin-top:10px"><div class="pl"><span>Sinal WiFi</span><span id="wpct">--%</span></div><div class="pb"><div class="pf" id="wbar" style="width:0"></div></div></div>
  </div>
  <div class="card">
    <div class="ct">Sistema</div>
    <div class="ig">
      <div><span class="k">IP: </span><span class="va" id="iip">--</span></div>
      <div><span class="k">SSID: </span><span class="v" id="issid">--</span></div>
      <div><span class="k">Modelo IA: </span><span class="v" id="imod">--</span></div>
      <div><span class="k">Cidade: </span><span class="v" id="icid">--</span></div>
      <div><span class="k">SD Card: </span><span class="v" id="isd">--</span></div>
      <div><span class="k">OTA: </span><span class="v" id="iota">--</span></div>
    </div>
  </div>
</div>

<!-- CLIMA -->
<div id="tab-clima" class="tp">
  <div class="sh">
    <div class="st">Clima &amp; Previsão</div>
    <button class="btn btn-s btn-sm" onclick="atuClima()">↻ Update</button>
  </div>
  <div class="card">
    <div class="ct">Agora em <span id="ccid" style="color:var(--ac)">--</span></div>
    <div class="sg">
      <div class="sv"><div class="sl">Temp</div><div class="sn" id="ct_">--<span class="su">°C</span></div></div>
      <div class="sv"><div class="sl">Sensação</div><div class="sn" id="cs_">--<span class="su">°C</span></div></div>
      <div class="sv"><div class="sl">Umidade</div><div class="sn" id="ch_">--<span class="su">%</span></div></div>
      <div class="sv"><div class="sl">Pressão</div><div class="sn" id="cp_" style="font-size:16px">--<span class="su">hPa</span></div></div>
      <div class="sv"><div class="sl">Vento</div><div class="sn" id="cv_" style="font-size:16px">--<span class="su">km/h</span></div></div>
      <div class="sv"><div class="sl">Condição</div><div id="cd_" style="font-size:12px;margin-top:6px;color:var(--tx);font-family:var(--mono)">--</div></div>
    </div>
  </div>
  <div class="card">
    <div class="ct">Previsão</div>
    <div class="wg">
      <div class="wc"><div class="wh" id="ph1">+3h</div><div class="wt" id="pt1">--</div><div class="wr" id="pr1">--%</div></div>
      <div class="wc"><div class="wh" id="ph2">+6h</div><div class="wt" id="pt2">--</div><div class="wr" id="pr2">--%</div></div>
      <div class="wc"><div class="wh" id="ph3">+9h</div><div class="wt" id="pt3">--</div><div class="wr" id="pr3">--%</div></div>
    </div>
  </div>
  <div class="ab" id="alerb"><strong>⚠ Alerta ativo</strong><br><span id="alerm"></span></div>
</div>

<!-- SD CARD -->
<div id="tab-sd" class="tp">
  <div class="sh">
    <div class="st">Gerenciador SD</div>
    <button class="btn btn-s btn-sm" onclick="loadFiles('/aurora')">↻ Recarregar</button>
  </div>
  <div class="card">
    <div class="ct">Arquivos</div>
    <div id="ftree" class="ft"><span style="color:var(--mu)">Carregando...</span></div>
  </div>
  <div class="card">
    <div class="ct">Novo Arquivo</div>
    <div class="fl">Caminho</div>
    <input class="inp" id="nfp" placeholder="/aurora/agenda/nota.txt">
    <div class="fl">Conteúdo</div>
    <textarea class="inp" id="nfc" placeholder="Conteúdo..." style="min-height:70px"></textarea>
    <button class="btn btn-p" onclick="criarArq()">Criar</button>
  </div>
</div>

<!-- CONFIG -->
<div id="tab-cfg" class="tp">
  <div class="st" style="margin-bottom:16px">Configurações</div>
  <div class="card">
    <div class="ct">Parâmetros</div>
    <div class="fg">
      <div><div class="fl">Modelo Gemini</div>
        <select class="inp" id="cfmod">
          <option value="gemini-2.5-flash">gemini-2.5-flash</option>
          <option value="gemini-1.5-pro">gemini-1.5-pro</option>
          <option value="gemini-1.5-flash">gemini-1.5-flash</option>
        </select>
      </div>
      <div><div class="fl">Cidade (OpenWeather)</div>
        <input class="inp" id="cfcid" placeholder="Muriae,BR">
      </div>
    </div>
    <button class="btn btn-p" onclick="salvCfg()">Salvar</button>
  </div>
  <div class="card">
    <div class="ct">Personalidade da IA</div>
    <textarea class="inp" id="cfpers" placeholder="Carregando..." style="min-height:130px"></textarea>
    <button class="btn btn-p" onclick="salvPers()">Salvar Personalidade</button>
  </div>
  <div class="card">
    <div class="ct">Funções</div>
    <div class="tr"><div class="ti"><div class="tn">LED NeoPixel</div><div class="td">Indicador de estado</div></div>
      <label class="tg"><input type="checkbox" id="tled" checked onchange="togFunc('led',this.checked)"><span class="tslider"></span></label></div>
    <div class="tr"><div class="ti"><div class="tn">OLED Display</div><div class="td">Display físico</div></div>
      <label class="tg"><input type="checkbox" id="toled" checked onchange="togFunc('oled',this.checked)"><span class="tslider"></span></label></div>
    <div class="tr"><div class="ti"><div class="tn">Alertas Climáticos</div><div class="td">Notificações Telegram</div></div>
      <label class="tg"><input type="checkbox" id="taler" checked onchange="togFunc('alertas',this.checked)"><span class="tslider"></span></label></div>
    <div class="tr"><div class="ti"><div class="tn">Modo Noturno</div><div class="td">Silencia Telegram no horário abaixo</div></div>
      <label class="tg"><input type="checkbox" id="tnot" checked onchange="togFunc('noturno',this.checked)"><span class="tslider"></span></label></div>
    <div class="fg" style="margin-top:12px">
      <div><div class="fl">Início noturno</div><select class="inp" id="cni"></select></div>
      <div><div class="fl">Fim noturno</div><select class="inp" id="cnf"></select></div>
    </div>
    <div class="br"><button class="btn btn-p" onclick="salvNot()">Salvar Horário Noturno</button></div>
  </div>
  <div class="card">
    <div class="ct">LED por Estado</div>
    <div id="leds"></div>
    <div class="br"><button class="btn btn-p" onclick="salvLED()">Salvar Cores</button></div>
  </div>
  <div class="card">
    <div class="ct">Wi-Fi</div>
    <div class="ig" style="margin-bottom:10px" id="wfst"><span class="k">Carregando...</span></div>
    <div class="br" style="margin-bottom:10px">
      <button class="btn btn-s btn-sm" onclick="scanWifi()">Escanear redes</button>
    </div>
    <div class="fg">
      <div><div class="fl">Rede disponível</div><select class="inp" id="wscan"><option value="">-- selecione --</option></select></div>
      <div><div class="fl">SSID manual</div><input class="inp" id="wssid" placeholder="Nome da rede"></div>
    </div>
    <div class="fl">Senha</div><input class="inp" id="wpass" type="password" placeholder="Senha">
    <button class="btn btn-p" onclick="conWifi()">Conectar</button>
  </div>
</div>

<!-- CONTROLE -->
<div id="tab-ctrl" class="tp">
  <div class="st" style="margin-bottom:16px">Controle</div>
  <div class="card">
    <div class="ct">Ações</div>
    <div class="br">
      <button class="btn btn-w" onclick="conf('Reiniciar ESP32?',cmdRst)">↺ Reiniciar</button>
      <button class="btn btn-s" onclick="cmdOTA()">📡 OTA</button>
      <button class="btn btn-s" onclick="cmdRel()">📊 Relatório</button>
    </div>
  </div>
  <div class="card">
    <div class="ct">Limpeza</div>
    <div class="br">
      <button class="btn btn-d btn-sm" onclick="conf('Apagar histórico IA?',cmdIA)">Histórico IA</button>
      <button class="btn btn-d btn-sm" onclick="conf('Apagar logs?',cmdLogs)">Logs SD</button>
      <button class="btn btn-d btn-sm" onclick="conf('Apagar memória?',cmdMem)">Memória</button>
    </div>
  </div>
  <div class="card">
    <div class="ct">Console</div>
    <div class="lb" id="logb"><span class="ll k">Sistema pronto.</span></div>
    <div class="br">
      <button class="btn btn-s btn-sm" onclick="fetchLog()">↻ Atualizar</button>
      <button class="btn btn-s btn-sm" onclick="document.getElementById('logb').innerHTML=''">Limpar</button>
    </div>
  </div>
  <div class="card" id="relbox" style="display:none">
    <div class="ct">Relatório</div>
    <div id="relc" class="lb" style="min-height:50px;white-space:pre-wrap"></div>
  </div>
</div>

</div><!-- /wrap -->
</div><!-- /app -->

<!-- MODAL EDITOR -->
<div class="mo" id="emod">
<div class="md">
  <div class="mh"><div class="mt" id="etit">/aurora/config/arquivo.txt</div>
    <button class="btn btn-s btn-sm" onclick="closeEd()">✕</button></div>
  <div class="mb"><textarea id="eare" spellcheck="false"></textarea></div>
  <div class="mf">
    <button class="btn btn-d btn-sm" onclick="delCur()">Deletar</button>
    <button class="btn btn-p" onclick="salvEd()">Salvar</button>
  </div>
</div>
</div>

<div id="toast"></div>

<script>
var LI=false, CF='', RT=null, CK=null, AT='dash', ML=true;
var AL={wifi:'WiFi',idle:'Idle',processing:'IA',success:'Sucesso',error:'Erro'};
var CL=[{id:'vermelho',n:'Vermelho',h:'#ff3333'},{id:'verde',n:'Verde',h:'#3fb950'},
        {id:'azul',n:'Azul',h:'#58a6ff'},{id:'amarelo',n:'Amarelo',h:'#d29922'},
        {id:'laranja',n:'Laranja',h:'#ff8c00'},{id:'roxo',n:'Roxo',h:'#bc8cff'},
        {id:'rosa',n:'Rosa',h:'#ff69b4'},{id:'ciano',n:'Ciano',h:'#39d0d0'},
        {id:'branco',n:'Branco',h:'#ffffff'}];
var EF=[{id:'respiracao',n:'Respiração'},{id:'strobo_rapido',n:'Strobo rápido'},
        {id:'strobo_medio',n:'Strobo médio'},{id:'arco-iris',n:'Arco-íris'}];
var PC={s:null,c:null,g:null}, PT={s:0,c:0,g:0}, PL={s:6000,c:20000,g:30000};

function req(u,o,cb){
  o=o||{};cb=cb||function(){};
  var x=new XMLHttpRequest();
  x.open(o.m||'GET',u,true);x.timeout=12000;
  x.setRequestHeader('Accept','application/json');
  if(o.h)for(var k in o.h)if(o.h.hasOwnProperty(k))x.setRequestHeader(k,o.h[k]);
  x.onreadystatechange=function(){
    if(x.readyState!==4)return;
    if(x.status===401){doLogout();cb(null);return;}
    if(x.status<200||x.status>=300){cb(null);return;}
    try{cb(JSON.parse(x.responseText));}catch(e){cb(null);}
  };
  x.onerror=x.ontimeout=function(){cb(null);};
  x.send(o.b||null);
}

function doLogin(){
  var p=document.getElementById('lpin').value;
  req('/api/login',{m:'POST',h:{'Content-Type':'application/json'},b:JSON.stringify({pin:p})},function(d){
    if(d&&d.ok){LI=true;document.getElementById('ls').style.display='none';
      document.getElementById('app').style.display='block';startApp();}
    else{document.getElementById('lerr').textContent='Senha incorreta.';
      document.getElementById('lpin').value='';}
  });
}
function doLogout(){
  req('/api/logout',{m:'POST'},function(){});LI=false;
  clearInterval(RT);clearInterval(CK);AT='dash';
  document.getElementById('app').style.display='none';
  document.getElementById('ls').style.display='flex';
  document.getElementById('lpin').value='';
  document.getElementById('lerr').textContent='';
}

function startApp(){
  fillHours();buildLED();
  resetTabs();updML();rdash(1);startRT();
  clearInterval(CK);
  CK=setInterval(function(){
    var d=new Date();
    document.getElementById('ttm').textContent=p2(d.getHours())+':'+p2(d.getMinutes());
  },1000);
}
function startRT(){
  clearInterval(RT);
  RT=setInterval(function(){
    if(!LI)return;
    if(AT==='dash')rdash();
    else if(!ML&&AT==='clima')loadClima(1);
    else if(!ML&&AT==='ctrl')fetchLog();
  },ML?25000:12000);
}
function updML(){
  var b=document.getElementById('mlbtn');
  b.textContent=ML?'Leve':'Detalhado';
  b.style.color=ML?'var(--ac2)':'var(--ac4)';
}
function toggleLeve(){ML=!ML;updML();startRT();if(AT==='dash')rdash(1);}
function resetTabs(){
  var ts=document.querySelectorAll('.nt'),ps=document.querySelectorAll('.tp');
  for(var i=0;i<ts.length;i++)ts[i].className='nt';
  for(var i=0;i<ps.length;i++)ps[i].style.display='none';
  if(ts.length)ts[0].className='nt on';
  var d=document.getElementById('tab-dash');if(d)d.style.display='block';
}
function showTab(n,b){
  AT=n;
  var ts=document.querySelectorAll('.nt'),ps=document.querySelectorAll('.tp');
  for(var i=0;i<ts.length;i++)ts[i].className='nt';
  for(var i=0;i<ps.length;i++)ps[i].style.display='none';
  var p=document.getElementById('tab-'+n);if(p)p.style.display='block';
  if(b)b.className='nt on';
  if(n==='dash')rdash(1);
  else if(n==='clima')loadClima(1);
  else if(n==='sd')loadFiles('/aurora');
  else if(n==='cfg')loadCfg(1);
}

// DASHBOARD
function applyDash(d){
  var t=+(d.chipTemp||0),el=document.getElementById('schip');
  el.innerHTML=t.toFixed(1)+'<span class="su">°C</span>';
  el.className='sn'+(t>75?' bad':t>55?' warn':' ok');
  var h=+(d.heapPct||0),he=document.getElementById('sheap');
  he.innerHTML=h+'<span class="su">%</span>';
  he.className='sn'+(h<20?' bad':h<40?' warn':' ok');
  document.getElementById('hpct').textContent=h+'%';
  document.getElementById('hbar').style.width=h+'%';
  var r=+(d.rssi||0),w=+(d.wifiPct||0),we=document.getElementById('swifi');
  we.innerHTML=r+'<span class="su">dBm</span>';
  we.className='sn'+(w<30?' bad':w<60?' warn':' ok');
  document.getElementById('wpct').textContent=w+'%';
  document.getElementById('wbar').style.width=w+'%';
  document.getElementById('sut').textContent=d.uptime||'--';
  document.getElementById('sqst').textContent=d.questions||'0';
  document.getElementById('scli').innerHTML=+(d.climaTemp||0).toFixed(1)+'<span class="su">°C</span>';
  document.getElementById('iip').textContent=d.ip||'--';
  document.getElementById('issid').textContent=d.ssid||'--';
  document.getElementById('imod').textContent=d.modelo||'--';
  document.getElementById('icid').textContent=d.cidade||'--';
  document.getElementById('isd').textContent=d.sdOK?'OK':'Falha';
  document.getElementById('iota').textContent=d.otaAtivo?'Ativo':'Inativo';
}
function rdash(f){
  f=!!f;var ic=document.getElementById('rico');
  if(!f&&PC.s&&(Date.now()-PT.s)<PL.s){applyDash(PC.s);return;}
  ic.textContent='…';
  req('/api/status',{},function(d){
    ic.textContent='↻';if(!d)return;
    PC.s=d;PT.s=Date.now();applyDash(d);
  });
}

// CLIMA
function applyClima(d){
  document.getElementById('ccid').textContent=d.cidade||'--';
  document.getElementById('ct_').innerHTML=+(d.temp||0).toFixed(1)+'<span class="su">°C</span>';
  document.getElementById('cs_').innerHTML=+(d.sensTermica||0).toFixed(1)+'<span class="su">°C</span>';
  document.getElementById('ch_').innerHTML=+(d.umidade||0)+'<span class="su">%</span>';
  document.getElementById('cp_').innerHTML=+(d.pressao||0)+'<span class="su">hPa</span>';
  document.getElementById('cv_').innerHTML=+(d.vento||0).toFixed(1)+'<span class="su">km/h</span>';
  document.getElementById('cd_').textContent=d.descricao||'--';
  if(d.prev){
    document.getElementById('ph1').textContent=d.prev.h1||'+3h';
    document.getElementById('ph2').textContent=d.prev.h2||'+6h';
    document.getElementById('ph3').textContent=d.prev.h3||'+9h';
    document.getElementById('pt1').textContent=Math.round(d.prev.t1||0)+'°';
    document.getElementById('pt2').textContent=Math.round(d.prev.t2||0)+'°';
    document.getElementById('pt3').textContent=Math.round(d.prev.t3||0)+'°';
    document.getElementById('pr1').textContent=Math.round(d.prev.r1||0)+'%';
    document.getElementById('pr2').textContent=Math.round(d.prev.r2||0)+'%';
    document.getElementById('pr3').textContent=Math.round(d.prev.r3||0)+'%';
  }
  var ab=document.getElementById('alerb');
  ab.style.display=d.alertaAtivo?'block':'none';
  document.getElementById('alerm').textContent=d.alertaMsg||'';
}
function loadClima(f){
  if(!f&&PC.c&&(Date.now()-PT.c)<PL.c){applyClima(PC.c);return;}
  req('/api/clima',{},function(d){if(!d)return;PC.c=d;PT.c=Date.now();applyClima(d);});
}
function atuClima(){
  toast('Atualizando...','i');
  req('/api/clima/update',{m:'POST'},function(d){
    if(d&&d.ok){toast('Clima atualizado!','s');loadClima(1);}else toast('Erro.','e');
  });
}

// SD
function loadFiles(path){
  var t=document.getElementById('ftree');t.innerHTML='<span style="color:var(--mu)">Carregando...</span>';
  req('/api/sd/list?path='+encodeURIComponent(path),{},function(d){
    if(!d){t.innerHTML='<span style="color:var(--ac3)">Erro ao ler SD</span>';return;}
    var h='',i,f;
    if(path!=='/'){
      var par=path.lastIndexOf('/')>0?path.substring(0,path.lastIndexOf('/')):'/';
      h+='<div class="fd" data-p="'+esc(par)+'">↩ voltar</div>';
    }
    if(d.dirs)for(i=0;i<d.dirs.length;i++)
      h+='<div class="fd" data-p="'+esc(d.dirs[i].path)+'">📁 '+esc(d.dirs[i].name)+'</div>';
    if(d.files)for(i=0;i<d.files.length;i++){
      f=d.files[i];
      h+='<div class="fe" data-f="'+esc(f.path)+'"><span>📄 '+esc(f.name)+'</span>'+
         '<span class="fsz">'+fb(f.size)+'</span>'+
         '<div class="fa"><button class="fb" data-a="e">Editar</button>'+
         '<button class="fb del" data-a="d">Del</button></div></div>';
    }
    t.innerHTML=h||'<span style="color:var(--mu)">Pasta vazia.</span>';
    var ds=t.querySelectorAll('.fd[data-p]');
    for(i=0;i<ds.length;i++)(function(el){el.onclick=function(){loadFiles(el.getAttribute('data-p'));}})(ds[i]);
    var fs=t.querySelectorAll('.fe[data-f]');
    for(i=0;i<fs.length;i++)(function(el){el.onclick=function(ev){
      var e=ev||window.event,tg=e.target||e.srcElement,pf=el.getAttribute('data-f'),a=tg?tg.getAttribute('data-a'):null;
      if(a==='e'){if(e.stopPropagation)e.stopPropagation();editArq(pf);}
      else if(a==='d'){if(e.stopPropagation)e.stopPropagation();conf('Deletar arquivo?',function(){delArq(pf);});}
    };})(fs[i]);
  });
}
function editArq(p){
  document.getElementById('etit').textContent=p;document.getElementById('eare').value='…';
  CF=p;document.getElementById('emod').className='mo on';
  req('/api/sd/read?path='+encodeURIComponent(p),{},function(d){
    document.getElementById('eare').value=d?(d.content||''):'(erro)';});
}
function salvEd(){
  req('/api/sd/write',{m:'POST',h:{'Content-Type':'application/json'},
    b:JSON.stringify({path:CF,content:document.getElementById('eare').value})},function(d){
    if(d&&d.ok){toast('Salvo!','s');closeEd();}else toast('Erro.','e');});
}
function delCur(){conf('Deletar permanentemente?',function(){closeEd();delArq(CF);});}
function delArq(p){
  req('/api/sd/delete',{m:'POST',h:{'Content-Type':'application/json'},b:JSON.stringify({path:p})},function(d){
    var dir=p.lastIndexOf('/')>0?p.substring(0,p.lastIndexOf('/')):'/';
    if(d&&d.ok){toast('Deletado.','s');loadFiles(dir);}else toast('Erro.','e');});
}
function criarArq(){
  var p=document.getElementById('nfp').value.replace(/^\s+|\s+$/g,'');
  var c=document.getElementById('nfc').value;
  if(!p){toast('Informe o caminho.','e');return;}
  req('/api/sd/write',{m:'POST',h:{'Content-Type':'application/json'},b:JSON.stringify({path:p,content:c})},function(d){
    if(d&&d.ok){toast('Criado!','s');document.getElementById('nfp').value='';document.getElementById('nfc').value='';
      loadFiles(p.substring(0,p.lastIndexOf('/'))||'/');}else toast('Erro.','e');});
}
function closeEd(){document.getElementById('emod').className='mo';}

// CONFIG
function applyCfg(d){
  document.getElementById('cfmod').value=d.modelo||'gemini-2.5-flash';
  document.getElementById('cfcid').value=d.cidade||'Muriae,BR';
  document.getElementById('cfpers').value=d.personalidade||'';
  document.getElementById('tled').checked=!d.ledDesabilitado;
  document.getElementById('toled').checked=!d.oledDesabilitado;
  document.getElementById('taler').checked=!d.alertasDesabilitados;
  document.getElementById('tnot').checked=!d.noturnoDesabilitado;
  document.getElementById('cni').value=String(d.noturnoInicio!=null?d.noturnoInicio:22);
  document.getElementById('cnf').value=String(d.noturnoFim!=null?d.noturnoFim:8);
  fillLED('wifi',d.ledColors&&d.ledColors.wifi,d.ledEffects&&d.ledEffects.wifi);
  fillLED('idle',d.ledColors&&d.ledColors.idle,d.ledEffects&&d.ledEffects.idle);
  fillLED('processing',d.ledColors&&d.ledColors.processing,d.ledEffects&&d.ledEffects.processing);
  fillLED('success',d.ledColors&&d.ledColors.success,d.ledEffects&&d.ledEffects.success);
  fillLED('error',d.ledColors&&d.ledColors.error,d.ledEffects&&d.ledEffects.error);
  applyWfSt(d);
}
function loadCfg(f){
  if(!f&&PC.g&&(Date.now()-PT.g)<PL.g){applyCfg(PC.g);return;}
  req('/api/config',{},function(d){if(!d)return;PC.g=d;PT.g=Date.now();applyCfg(d);});
}
function salvCfg(cb){
  var p={modelo:document.getElementById('cfmod').value,cidade:document.getElementById('cfcid').value,
    noturnoInicio:parseInt(document.getElementById('cni').value,10),
    noturnoFim:parseInt(document.getElementById('cnf').value,10),
    ledColors:colLED(),ledEffects:efLED()};
  req('/api/config',{m:'POST',h:{'Content-Type':'application/json'},b:JSON.stringify(p)},function(d){
    if(d&&d.ok)PT.g=0;
    if(cb)cb(d&&d.ok);else toast(d&&d.ok?'Salvo!':'Erro.',d&&d.ok?'s':'e');
  });
}
function salvNot(){salvCfg(function(ok){toast(ok?'Horário noturno salvo!':'Erro.',ok?'s':'e');});}
function salvLED(){salvCfg(function(ok){toast(ok?'Cores salvas!':'Erro.',ok?'s':'e');});}
function salvPers(){
  req('/api/personalidade',{m:'POST',h:{'Content-Type':'application/json'},
    b:JSON.stringify({texto:document.getElementById('cfpers').value})},function(d){
    toast(d&&d.ok?'Personalidade salva!':'Erro.',d&&d.ok?'s':'e');});
}
function togFunc(n,v){
  req('/api/toggle',{m:'POST',h:{'Content-Type':'application/json'},b:JSON.stringify({name:n,enabled:v})},
    function(){toast((v?'ON: ':'OFF: ')+n,'i');});
}
function applyWfSt(c){
  var el=document.getElementById('wfst');if(!el)return;
  var on=!!(c&&c.apConfigAtivo),nm=(c&&c.apConfigNome)||'Aurora-Setup';
  el.innerHTML=on?('<span class="k">AP ativo: </span><span class="va">'+nm+'</span><span class="k"> (192.168.4.1)</span>')
                 :'<span class="k">AP inativo. Duplo clique no botão WEB para ligar.</span>';
}
function scanWifi(){
  req('/api/wifi/scan',{},function(d){
    if(!d){toast('Scan falhou.','e');return;}
    if(d.scanning){toast('Escaneando...','i');setTimeout(function(){
      req('/api/wifi/scan',{},function(d2){if(d2&&d2.done)fillScan(d2);else toast('Aguardando scan...','i');});
    },3000);return;}
    if(d.done)fillScan(d);
  });
}
function fillScan(d){
  var s=document.getElementById('wscan');if(!s)return;
  s.innerHTML='<option value="">-- selecione --</option>';
  if(d.redes)for(var i=0;i<d.redes.length;i++){
    var r=d.redes[i],o=document.createElement('option');
    o.value=r.ssid;o.text=r.ssid+' ('+r.rssi+'dBm'+(r.open?', aberta':'')+')';s.appendChild(o);
  }
  toast('Scan: '+(d.redes?d.redes.length:0)+' rede(s).','i');
}
function conWifi(){
  var s=document.getElementById('wscan').value||document.getElementById('wssid').value.replace(/^\s+|\s+$/g,'');
  var p=document.getElementById('wpass').value;
  if(!s){toast('Informe o SSID.','e');return;}
  toast('Conectando...','i');
  req('/api/wifi/connect',{m:'POST',h:{'Content-Type':'application/json'},b:JSON.stringify({ssid:s,senha:p})},
    function(d){if(d&&d.ok){toast('Conectado! IP: '+(d.ip||'--'),'s');PT.s=0;loadCfg(1);rdash(1);}
      else toast('Falha ao conectar.','e');});
}

// CONTROLE
function cmdRst(){req('/api/cmd/reset',{m:'POST'},function(){});toast('Reiniciando...','w');}
function cmdOTA(){req('/api/cmd/ota',{m:'POST'},function(d){toast(d&&d.ok?'OTA ativo. IP: '+d.ip:'Erro OTA.',d&&d.ok?'s':'e');});}
function cmdRel(){req('/api/cmd/relatorio',{m:'POST'},function(d){
  var b=document.getElementById('relbox'),c=document.getElementById('relc');
  if(d&&d.relatorio){b.style.display='block';c.textContent=d.relatorio;toast('Relatório gerado!','s');}
  else toast('Erro.','e');});}
function cmdIA(){req('/api/cmd/limpar-ia',{m:'POST'},function(d){toast(d&&d.ok?'Histórico IA apagado.':'Erro.',d&&d.ok?'s':'e');});}
function cmdLogs(){req('/api/cmd/limpar-logs',{m:'POST'},function(d){toast(d&&d.ok?'Logs apagados.':'Erro.',d&&d.ok?'s':'e');});}
function cmdMem(){req('/api/cmd/limpar-mem',{m:'POST'},function(d){toast(d&&d.ok?'Memória apagada.':'Erro.',d&&d.ok?'s':'e');});}
function fetchLog(){req('/api/log',{},function(d){
  if(!d||!d.lines)return;
  var h=[];
  for(var i=0;i<d.lines.length;i++){var l=d.lines[i]||'';
    var cl=(l.indexOf('Erro')>=0||l.indexOf('erro')>=0)?'e':(l.indexOf('OK')>=0||l.indexOf('ok')>=0)?'k':'';
    h.push('<span class="ll '+cl+'">'+esc(l)+'</span>');}
  var b=document.getElementById('logb');b.innerHTML=h.join('\n');b.scrollTop=b.scrollHeight;
});}

// LED
function buildLED(){
  var b=document.getElementById('leds');if(!b||b.getAttribute('dr'))return;
  var h='',k=Object.keys(AL);
  for(var i=0;i<k.length;i++){var s=k[i];
    h+='<div class="fg" style="margin-bottom:8px">'+
      '<div><div class="fl">'+AL[s]+' · Cor</div><select class="inp" id="lc_'+s+'"></select></div>'+
      '<div><div class="fl">'+AL[s]+' · Efeito</div><select class="inp" id="le_'+s+'"></select></div></div>';
  }
  b.innerHTML=h;b.setAttribute('dr','1');
  var j;for(j=0;j<k.length;j++){fillSel('lc_'+k[j],CL,'id','n');fillSel('le_'+k[j],EF,'id','n');}
}
function fillSel(id,arr,vk,tk){var s=document.getElementById(id);if(!s)return;s.innerHTML='';
  for(var i=0;i<arr.length;i++){var o=document.createElement('option');o.value=arr[i][vk];o.text=arr[i][tk];s.appendChild(o);}
}
function fillLED(st,rgb,ef){
  var h=rgb?r2h(rgb):'#58a6ff';
  var cs=document.getElementById('lc_'+st),es=document.getElementById('le_'+st);
  if(cs)cs.value=nearest(h);if(es)es.value=ef||'respiracao';
}
function colLED(){var o={};Object.keys(AL).forEach(function(s){
  var s2=document.getElementById('lc_'+s);o[s]=h2r(s2?s2.value:'#58a6ff');});return o;}
function efLED(){var o={};Object.keys(AL).forEach(function(s){
  var s2=document.getElementById('le_'+s);o[s]=s2?s2.value:'respiracao';});return o;}
function nearest(hex){var a=h2r(hex),b=CL[0].h,d=9999;
  for(var i=0;i<CL.length;i++){var c=h2r(CL[i].h),x=Math.abs(c.r-a.r)+Math.abs(c.g-a.g)+Math.abs(c.b-a.b);
    if(x<d){d=x;b=CL[i].h;}}return b;}

// HOURS
function fillHours(){var ni=document.getElementById('cni'),nf=document.getElementById('cnf');
  if(!ni||ni.options.length)return;
  for(var h=0;h<24;h++){var t=p2(h)+':00';
    var o1=document.createElement('option');o1.value=String(h);o1.text=t;ni.appendChild(o1);
    var o2=document.createElement('option');o2.value=String(h);o2.text=t;nf.appendChild(o2);}}

// UTILS
function h2r(hex){var h=String(hex||'').replace(/[^0-9a-fA-F]/g,'');
  if(h.length<6)h=(h+'000000').substring(0,6);h=h.substring(0,6);
  return{r:parseInt(h.substring(0,2),16),g:parseInt(h.substring(2,4),16),b:parseInt(h.substring(4,6),16)};}
function r2h(c){return'#'+th(c.r||0)+th(c.g||0)+th(c.b||0);}
function th(v){var s=Number(v).toString(16);return s.length<2?'0'+s:s;}
function p2(v){return v<10?'0'+v:String(v);}
function fb(b){if(b<1024)return b+'B';if(b<1048576)return(b/1024).toFixed(1)+'KB';return(b/1048576).toFixed(1)+'MB';}
function esc(s){s=String(s||'');return s.replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;').replace(/"/g,'&quot;');}
function conf(m,cb){if(window.confirm(m))cb();}
function toast(m,t){t=t||'i';
  var w=document.getElementById('toast'),el=document.createElement('div');
  el.className='ti2 '+t;el.appendChild(document.createTextNode(m));w.appendChild(el);
  setTimeout(function(){if(el.parentNode)el.parentNode.removeChild(el);},3200);}
</script>
</body>
</html>
)rawhtml";
#endif
