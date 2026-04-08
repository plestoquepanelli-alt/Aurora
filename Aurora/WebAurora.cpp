// ── Bibliotecas ────────────────────────────────────────────
// IMPORTANTE: <WebServer.h> = biblioteca Arduino (ESP32 HTTP server)
// "WebAurora.h"             = nosso header (renomeado de WebServer.h
//                             para evitar conflito com a biblioteca)
#include <WiFi.h>
#include <WebServer.h>   // biblioteca Arduino ESP32 HTTP Server
#include <ArduinoJson.h>
#include <SD.h>
#include "WebAurora.h"   // nosso header (declarações públicas)
#include "Config.h"
#include "Sistema.h"
#include "LEDControl.h"
#include "Clima.h"
#include "AlertaINMET.h"
#include "SDCard.h"
#include "Display.h"
#include "GeminiTypes.h" // enum GJobState, struct GeminiJob
#include "WebPanel.h"

// ── Alocador PSRAM para JsonDocuments grandes ─────────────────
// Redireciona alocações ≥1KB para PSRAM quando disponível,
// mantendo o heap interno livre para WiFi, TLS e tarefas FreeRTOS.
struct SpiRamAllocator {
    void* allocate(size_t n) {
        return (ESP.getFreePsram() > n)
            ? heap_caps_malloc(n, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)
            : malloc(n);
    }
    void deallocate(void* p) { free(p); }
    void* reallocate(void* p, size_t n) {
        return heap_caps_realloc(p, n, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    }
};
using PsramDoc = BasicJsonDocument<SpiRamAllocator>;

// ════════════════════════════════════════════════════════════
//  VARIÁVEIS
// ════════════════════════════════════════════════════════════
bool webPanelAtivo  = false;
bool webSessaoAtiva = false;   // true enquanto painel está logado

static WebServer server(80);
static bool      _sessaoOk   = false;
static uint32_t  _sessaoToken = 0;    // token simples anti-CSRF
static unsigned long _sessaoInicio = 0;
#define SESSAO_TIMEOUT_MS  1800000UL   // 30 minutos

// Flags de controle de funções (toggles do painel)
static bool _ledDesab      = false;
static bool _oledDesab     = false;
static bool _alertasDesab  = false;

// ════════════════════════════════════════════════════════════
//  HELPERS
// ════════════════════════════════════════════════════════════
static void setCORS(){
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Cache-Control", "no-cache");
}

static bool checkAuth(){
  if(!_sessaoOk){ server.send(401, "application/json", "{\"err\":\"unauthorized\"}"); return false; }
  // Timeout automático de sessão
  if(millis() - _sessaoInicio > SESSAO_TIMEOUT_MS){
    _sessaoOk   = false;
    webSessaoAtiva = false;
    server.send(401, "application/json", "{\"err\":\"session_expired\"}");
    return false;
  }
  _sessaoInicio = millis(); // renova timeout a cada request
  return true;
}

static void sendJSON(const String& json, int code=200){
  setCORS();
  server.send(code, "application/json", json);
}

// Lê corpo JSON da request
static bool parseBody(JsonDocument& doc){
  if(!server.hasArg("plain")) return false;
  return !deserializeJson(doc, server.arg("plain"));
}

// ════════════════════════════════════════════════════════════
//  ROTA RAIZ — serve o painel HTML
// ════════════════════════════════════════════════════════════
static void handleRoot(){
  setCORS();
  server.sendHeader("Content-Encoding", "identity");
  server.send_P(200, "text/html; charset=UTF-8", AURORA_HTML);
}

// ════════════════════════════════════════════════════════════
//  AUTH — /api/login  /api/logout
// ════════════════════════════════════════════════════════════
static void handleLogin(){
  DynamicJsonDocument doc(128);
  if(!parseBody(doc)){ sendJSON("{\"ok\":false,\"err\":\"bad_json\"}"); return; }
  String pin = doc["pin"] | "";
  if(pin == String(ADMIN_SENHA)){
    _sessaoOk      = true;
    webSessaoAtiva = true;
    _sessaoInicio  = millis();
    _sessaoToken   = esp_random();
    sendJSON("{\"ok\":true}");
  } else {
    sendJSON("{\"ok\":false}", 403);
  }
}

static void handleLogout(){
  _sessaoOk      = false;
  webSessaoAtiva = false;
  sendJSON("{\"ok\":true}");
}

// ════════════════════════════════════════════════════════════
//  STATUS — /api/status
// ════════════════════════════════════════════════════════════
static void handleStatus(){
  if(!checkAuth()) return;
  DynamicJsonDocument doc(512);
  float tc = lerTemperatura();
  doc["chipTemp"]  = isnan(tc) ? 0.0f : (float)((int)(tc*10))/10.0f;
  doc["heapPct"]   = heapPercentual();
  doc["heapFree"]  = (int)(ESP.getFreeHeap()/1024);
  doc["rssi"]      = (int)WiFi.RSSI();
  doc["wifiPct"]   = wifiPercentual();
  doc["uptime"]    = uptime();
  doc["questions"] = (int)perguntasFeitas;
  doc["climaTemp"] = (float)((int)(climaTemp*10))/10.0f;
  doc["ip"]        = WiFi.localIP().toString();
  doc["ssid"]      = WiFi.SSID();
  doc["modelo"]    = modeloAtivo;
  doc["cidade"]    = cidade;
  doc["sdOK"]      = sdOK;
  doc["otaAtivo"]  = otaAtivo();
  String out; serializeJson(doc, out);
  sendJSON(out);
}

// ════════════════════════════════════════════════════════════
//  CLIMA — /api/clima  /api/clima/update
// ════════════════════════════════════════════════════════════
static void handleClima(){
  if(!checkAuth()) return;
  DynamicJsonDocument doc(512);
  doc["cidade"]      = cidade;
  doc["temp"]        = (float)((int)(climaTemp*10))/10.0f;
  doc["sensTermica"] = (float)((int)(climaSensTermica*10))/10.0f;
  doc["umidade"]     = climaUmidade;
  doc["pressao"]     = climaPressao;
  doc["vento"]       = (float)((int)(climaVento*10))/10.0f;
  doc["descricao"]   = climaDescricao;
  doc["alertaAtivo"] = alertaOficialAtivo;
  doc["alertaMsg"]   = alertaOficialMsg;
  JsonObject prev    = doc.createNestedObject("prev");
  prev["h1"] = hora_prev3h; prev["t1"] = previsao3h; prev["r1"] = chuva3h;
  prev["h2"] = hora_prev6h; prev["t2"] = previsao6h; prev["r2"] = chuva6h;
  prev["h3"] = hora_prev9h; prev["t3"] = previsao9h; prev["r3"] = chuva9h;
  String out; serializeJson(doc, out);
  sendJSON(out);
}

static void handleClimaUpdate(){
  if(!checkAuth()) return;
  atualizarClima();
  verificarAlertaINMET();
  ultimoClima = millis();
  sendJSON("{\"ok\":true}");
}

// ════════════════════════════════════════════════════════════
//  SD CARD — listar / ler / escrever / deletar
// ════════════════════════════════════════════════════════════
static void handleSDList(){
  if(!checkAuth()) return;
  if(!sdOK){ sendJSON("{\"err\":\"sd_not_available\"}"); return; }

  String path = server.hasArg("path") ? server.arg("path") : "/aurora";
  // Segurança: limita ao diretório /aurora
  if(!path.startsWith("/aurora") && path != "/") path = "/aurora";

  PsramDoc doc(4096);
  JsonArray dirs  = doc.createNestedArray("dirs");
  JsonArray files = doc.createNestedArray("files");

  File root = SD.open(path.c_str());
  if(!root || !root.isDirectory()){
    sendJSON("{\"err\":\"path_not_found\"}"); return;
  }

  File entry;
  int count = 0;
  while((entry = root.openNextFile()) && count < 60){
    count++;
    String name = String(entry.name());
    // entry.name() pode retornar path completo no ESP32-Arduino SD
    int lastSlash = name.lastIndexOf('/');
    if(lastSlash >= 0) name = name.substring(lastSlash+1);

    String fullPath = path;
    if(!fullPath.endsWith("/")) fullPath += "/";
    fullPath += name;

    if(entry.isDirectory()){
      JsonObject d = dirs.createNestedObject();
      d["name"] = name;
      d["path"] = fullPath;
    } else {
      JsonObject f = files.createNestedObject();
      f["name"] = name;
      f["path"] = fullPath;
      f["size"] = (int)entry.size();
    }
    entry.close();
  }
  root.close();

  String out; serializeJson(doc, out);
  sendJSON(out);
}

static void handleSDRead(){
  if(!checkAuth()) return;
  if(!sdOK){ sendJSON("{\"err\":\"sd_not_available\"}"); return; }

  String path = server.hasArg("path") ? server.arg("path") : "";
  if(!path.startsWith("/aurora")){ sendJSON("{\"err\":\"forbidden\"}"); return; }

  File f = SD.open(path.c_str(), FILE_READ);
  if(!f){ sendJSON("{\"err\":\"file_not_found\"}"); return; }

  // Limita leitura a 8KB para não explodir a RAM
  const size_t MAX = 8192;
  String content = "";
  size_t read = 0;
  while(f.available() && read < MAX){
    char c = f.read(); content += c; read++;
  }
  bool truncated = f.available();
  f.close();

  PsramDoc doc(10240);
  doc["content"]   = content;
  doc["truncated"] = truncated;
  doc["size"]      = (int)read;
  String out; serializeJson(doc, out);
  sendJSON(out);
}

static void handleSDWrite(){
  if(!checkAuth()) return;
  if(!sdOK){ sendJSON("{\"err\":\"sd_not_available\"}"); return; }

  PsramDoc doc(10240);
  if(!parseBody(doc)){ sendJSON("{\"ok\":false,\"err\":\"bad_json\"}"); return; }

  String path    = doc["path"] | "";
  String content = doc["content"] | "";

  if(!path.startsWith("/aurora")){ sendJSON("{\"ok\":false,\"err\":\"forbidden\"}"); return; }
  if(path.length() < 8 || path.length() > 80){ sendJSON("{\"ok\":false,\"err\":\"invalid_path\"}"); return; }

  // Garante que o diretório pai existe
  int lastSlash = path.lastIndexOf('/');
  if(lastSlash > 0){
    String dir = path.substring(0, lastSlash);
    if(!SD.exists(dir.c_str())) SD.mkdir(dir.c_str());
  }

  File f = SD.open(path.c_str(), FILE_WRITE);
  if(!f){ sendJSON("{\"ok\":false,\"err\":\"write_failed\"}"); return; }
  f.print(content);
  f.close();

  sendJSON("{\"ok\":true}");
}

static void handleSDDelete(){
  if(!checkAuth()) return;
  if(!sdOK){ sendJSON("{\"err\":\"sd_not_available\"}"); return; }

  DynamicJsonDocument doc(256);
  if(!parseBody(doc)){ sendJSON("{\"ok\":false}"); return; }

  String path = doc["path"] | "";
  if(!path.startsWith("/aurora")){ sendJSON("{\"ok\":false,\"err\":\"forbidden\"}"); return; }

  bool ok = SD.remove(path.c_str());
  sendJSON(ok ? "{\"ok\":true}" : "{\"ok\":false,\"err\":\"delete_failed\"}");
}

// ════════════════════════════════════════════════════════════
//  CONFIG — /api/config (GET/POST)
// ════════════════════════════════════════════════════════════
static void handleConfigGet(){
  if(!checkAuth()) return;
  PsramDoc doc(2048);
  doc["modelo"]              = modeloAtivo;
  doc["cidade"]              = cidade;
  doc["personalidade"]       = carregarPersonalidade();
  doc["ledDesabilitado"]     = _ledDesab;
  doc["oledDesabilitado"]    = _oledDesab;
  doc["alertasDesabilitados"] = _alertasDesab;
  doc["noturnoDesabilitado"] = !modoNoturnoHabilitado;
  doc["noturnoInicio"]       = modoNoturnoInicioHora;
  doc["noturnoFim"]          = modoNoturnoFimHora;
  doc["apConfigAtivo"]       = apConfigAtivo();
  doc["apConfigNome"]        = nomeAPConfig();
  JsonObject cores = doc.createNestedObject("ledColors");
  LedColor wifi = obterCorLED("wifi");
  LedColor idle = obterCorLED("idle");
  LedColor proc = obterCorLED("processing");
  LedColor succ = obterCorLED("success");
  LedColor err  = obterCorLED("error");
  JsonObject cWifi = cores.createNestedObject("wifi");
  cWifi["r"] = wifi.r; cWifi["g"] = wifi.g; cWifi["b"] = wifi.b;
  JsonObject cIdle = cores.createNestedObject("idle");
  cIdle["r"] = idle.r; cIdle["g"] = idle.g; cIdle["b"] = idle.b;
  JsonObject cProc = cores.createNestedObject("processing");
  cProc["r"] = proc.r; cProc["g"] = proc.g; cProc["b"] = proc.b;
  JsonObject cSucc = cores.createNestedObject("success");
  cSucc["r"] = succ.r; cSucc["g"] = succ.g; cSucc["b"] = succ.b;
  JsonObject cErr = cores.createNestedObject("error");
  cErr["r"] = err.r; cErr["g"] = err.g; cErr["b"] = err.b;
  JsonObject efeitos = doc.createNestedObject("ledEffects");
  efeitos["wifi"] = obterEfeitoLED("wifi");
  efeitos["idle"] = obterEfeitoLED("idle");
  efeitos["processing"] = obterEfeitoLED("processing");
  efeitos["success"] = obterEfeitoLED("success");
  efeitos["error"] = obterEfeitoLED("error");
  String out; serializeJson(doc, out);
  sendJSON(out);
}

static void handleConfigPost(){
  if(!checkAuth()) return;
  DynamicJsonDocument doc(1024);
  if(!parseBody(doc)){ sendJSON("{\"ok\":false}"); return; }
  if(doc.containsKey("modelo")) modeloAtivo = doc["modelo"].as<String>();
  if(doc.containsKey("cidade")) cidade      = doc["cidade"].as<String>();
  if(doc.containsKey("noturnoInicio")) modoNoturnoInicioHora = constrain((int)doc["noturnoInicio"], 0, 23);
  if(doc.containsKey("noturnoFim")) modoNoturnoFimHora       = constrain((int)doc["noturnoFim"], 0, 23);
  if(doc.containsKey("ledColors")){
    JsonObject cores = doc["ledColors"].as<JsonObject>();
    const char* nomes[] = {"wifi", "idle", "processing", "success", "error"};
    for(int i=0; i<5; i++){
      if(!cores.containsKey(nomes[i])) continue;
      JsonObject c = cores[nomes[i]].as<JsonObject>();
      definirCorLED(
        nomes[i],
        constrain((int)(c["r"] | 0), 0, 255),
        constrain((int)(c["g"] | 0), 0, 255),
        constrain((int)(c["b"] | 0), 0, 255)
      );
    }
  }
  if(doc.containsKey("ledEffects")){
    JsonObject efeitos = doc["ledEffects"].as<JsonObject>();
    const char* nomes[] = {"wifi", "idle", "processing", "success", "error"};
    for(int i=0; i<5; i++){
      if(!efeitos.containsKey(nomes[i])) continue;
      definirEfeitoLED(nomes[i], efeitos[nomes[i]].as<String>());
    }
  }
  sendJSON("{\"ok\":true}");
}

static void handleWiFiState(){
  if(!checkAuth()) return;
  DynamicJsonDocument doc(384);
  doc["apConfigAtivo"] = apConfigAtivo();
  doc["apConfigNome"] = nomeAPConfig();
  doc["apIP"] = apConfigAtivo() ? WiFi.softAPIP().toString() : "";
  doc["staConnected"] = (WiFi.status() == WL_CONNECTED);
  doc["staSSID"] = WiFi.SSID();
  doc["staIP"] = (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString() : "";
  String out; serializeJson(doc, out);
  sendJSON(out);
}

static void handleWiFiScan(){
  if(!checkAuth()) return;
  int n = WiFi.scanNetworks(false, true);
  DynamicJsonDocument doc(3072);
  JsonArray redes = doc.createNestedArray("redes");
  for(int i = 0; i < n && i < 25; i++){
    String s = WiFi.SSID(i);
    if(s.isEmpty()) continue;
    JsonObject r = redes.createNestedObject();
    r["ssid"] = s;
    r["rssi"] = WiFi.RSSI(i);
    r["open"] = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN);
  }
  WiFi.scanDelete();
  doc["ok"] = true;
  String out; serializeJson(doc, out);
  sendJSON(out);
}

static void handleWiFiConnect(){
  if(!checkAuth()) return;
  DynamicJsonDocument doc(384);
  if(!parseBody(doc)){ sendJSON("{\"ok\":false,\"err\":\"bad_json\"}"); return; }
  String ssid = doc["ssid"] | "";
  String senha = doc["senha"] | "";
  if(ssid.isEmpty()){ sendJSON("{\"ok\":false,\"err\":\"ssid_required\"}"); return; }
  bool ok = conectarWiFiComCredenciais(ssid, senha, 20000UL);
  DynamicJsonDocument outDoc(256);
  outDoc["ok"] = ok;
  outDoc["ssid"] = ssid;
  outDoc["ip"] = ok ? WiFi.localIP().toString() : "";
  String out; serializeJson(outDoc, out);
  sendJSON(out, ok ? 200 : 400);
}

// ════════════════════════════════════════════════════════════
//  PERSONALIDADE — /api/personalidade
// ════════════════════════════════════════════════════════════
static void handlePersonalidade(){
  if(!checkAuth()) return;
  DynamicJsonDocument doc(2048);
  if(!parseBody(doc)){ sendJSON("{\"ok\":false}"); return; }
  String texto = doc["texto"] | "";
  if(texto.length() < 5){ sendJSON("{\"ok\":false,\"err\":\"too_short\"}"); return; }
  salvarPersonalidade(texto);
  sendJSON("{\"ok\":true}");
}

// ════════════════════════════════════════════════════════════
//  TOGGLE — /api/toggle
// ════════════════════════════════════════════════════════════
static void handleToggle(){
  if(!checkAuth()) return;
  DynamicJsonDocument doc(256);
  if(!parseBody(doc)){ sendJSON("{\"ok\":false}"); return; }
  String name   = doc["name"]    | "";
  bool   enabled = doc["enabled"] | true;

  if(name == "led"){
    _ledDesab = !enabled;
    ledHabilitado = enabled;
    if(!enabled){ pixel.clear(); pixel.show(); }
  }
  else if(name == "oled"){
    _oledDesab = !enabled;
    // Sinaliza para o loop principal via variável global
    extern bool oledLigado;
    oledLigado = enabled;
    if(!enabled){ display.clearDisplay(); display.display(); }
  }
  else if(name == "alertas") _alertasDesab = !enabled;
  else if(name == "noturno") modoNoturnoHabilitado = enabled;

  sendJSON("{\"ok\":true}");
}

// ════════════════════════════════════════════════════════════
//  COMANDOS — /api/cmd/*
// ════════════════════════════════════════════════════════════
static void handleCmdReset(){
  if(!checkAuth()) return;
  sendJSON("{\"ok\":true}");
  // sem delay — resposta já enviada antes do restart
  ESP.restart();
}

static void handleCmdOTA(){
  if(!checkAuth()) return;
  ativarOTA();
  DynamicJsonDocument doc(128);
  doc["ok"] = true;
  doc["ip"] = WiFi.localIP().toString();
  String out; serializeJson(doc, out);
  sendJSON(out);
}

static void handleCmdRelatorio(){
  if(!checkAuth()) return;
  String rel = gerarRelatorio();
  PsramDoc doc(6144);
  doc["relatorio"] = rel;
  String out; serializeJson(doc, out);
  sendJSON(out);
}

static void handleCmdLimparIA(){
  if(!checkAuth()) return;
  limparHistoricoIA();
  sendJSON("{\"ok\":true}");
}

static void handleCmdLimparLogs(){
  if(!checkAuth()) return;
  limparLogs();
  sendJSON("{\"ok\":true}");
}

static void handleCmdLimparMem(){
  if(!checkAuth()) return;
  limparMemoria();
  sendJSON("{\"ok\":true}");
}

// ════════════════════════════════════════════════════════════
//  LOG — /api/log (retorna últimas linhas de erros.txt)
// ════════════════════════════════════════════════════════════
static void handleLog(){
  if(!checkAuth()) return;
  DynamicJsonDocument doc(2048);
  JsonArray lines = doc.createNestedArray("lines");

  if(sdOK && SD.exists("/aurora/logs/erros.txt")){
    File f = SD.open("/aurora/logs/erros.txt", FILE_READ);
    if(f){
      // Lê últimas 20 linhas
      String buf[20]; int n=0;
      while(f.available()){
        String l = f.readStringUntil('\n'); l.trim();
        if(l.isEmpty()) continue;
        buf[n % 20] = l; n++;
      }
      f.close();
      int start = (n > 20) ? n % 20 : 0;
      int total = min(n, 20);
      for(int i=0;i<total;i++) lines.add(buf[(start+i)%20]);
    }
  } else {
    lines.add("Sem logs disponíveis.");
  }

  String out; serializeJson(doc, out);
  sendJSON(out);
}

// ════════════════════════════════════════════════════════════
//  OLED — exibe IP de acesso (botão pino 46)
// ════════════════════════════════════════════════════════════
void exibirAcessoWebOLED(){
  if(WiFi.status() != WL_CONNECTED) return;

  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);

  // Header
  display.fillRect(0, 0, 128, 12, SH110X_WHITE);
  display.setTextColor(SH110X_BLACK);
  display.setCursor(30, 2);
  display.setTextSize(1);
  display.print("PAINEL WEB");
  display.setTextColor(SH110X_WHITE);

  // IP
  display.setTextSize(1);
  display.setCursor(4, 18);
  display.print("Acesse no browser:");
  display.setCursor(4, 30);
  display.setTextSize(1);
  display.print("http://");
  display.print(WiFi.localIP().toString());

  // Porta
  display.setCursor(4, 42);
  display.print("Senha: " ADMIN_SENHA);

  // Rodapé
  display.drawFastHLine(0, 54, 128, SH110X_WHITE);
  display.setCursor(2, 56);
  display.printf("WiFi: %s", WiFi.SSID().c_str());

  display.display();
}

// ════════════════════════════════════════════════════════════
//  INIT
// ════════════════════════════════════════════════════════════
void initWebServer(){
  // Página principal
  server.on("/", HTTP_GET,  handleRoot);

  // Auth
  server.on("/api/login",  HTTP_POST, handleLogin);
  server.on("/api/logout", HTTP_POST, handleLogout);

  // Status & Clima
  server.on("/api/status",       HTTP_GET,  handleStatus);
  server.on("/api/clima",        HTTP_GET,  handleClima);
  server.on("/api/clima/update", HTTP_POST, handleClimaUpdate);

  // SD Card
  server.on("/api/sd/list",   HTTP_GET,  handleSDList);
  server.on("/api/sd/read",   HTTP_GET,  handleSDRead);
  server.on("/api/sd/write",  HTTP_POST, handleSDWrite);
  server.on("/api/sd/delete", HTTP_POST, handleSDDelete);

  // Config
  server.on("/api/config", HTTP_GET,  handleConfigGet);
  server.on("/api/config", HTTP_POST, handleConfigPost);
  server.on("/api/wifi/state", HTTP_GET, handleWiFiState);
  server.on("/api/wifi/scan", HTTP_GET, handleWiFiScan);
  server.on("/api/wifi/connect", HTTP_POST, handleWiFiConnect);
  server.on("/api/personalidade", HTTP_POST, handlePersonalidade);
  server.on("/api/toggle",        HTTP_POST, handleToggle);

  // Comandos
  server.on("/api/cmd/reset",       HTTP_POST, handleCmdReset);
  server.on("/api/cmd/ota",         HTTP_POST, handleCmdOTA);
  server.on("/api/cmd/relatorio",   HTTP_POST, handleCmdRelatorio);
  server.on("/api/cmd/limpar-ia",   HTTP_POST, handleCmdLimparIA);
  server.on("/api/cmd/limpar-logs", HTTP_POST, handleCmdLimparLogs);
  server.on("/api/cmd/limpar-mem",  HTTP_POST, handleCmdLimparMem);

  // Log
  server.on("/api/log", HTTP_GET, handleLog);

  // CORS preflight
  server.onNotFound([&](){
    setCORS();
    if(server.method() == HTTP_OPTIONS){
      server.send(204);
    } else {
      server.send(404, "application/json", "{\"err\":\"not_found\"}");
    }
  });

  server.begin();
  webPanelAtivo = true;
  Serial.println("[Web] Servidor iniciado em http://" + WiFi.localIP().toString());
}

// ════════════════════════════════════════════════════════════
//  LOOP HANDLER — chamar no loop() do Arduino
// ════════════════════════════════════════════════════════════
void handleWebServer(){
  server.handleClient();

  // Expiração automática de sessão inativa
  if(_sessaoOk && millis() - _sessaoInicio > SESSAO_TIMEOUT_MS){
    _sessaoOk      = false;
    webSessaoAtiva = false;
    Serial.println("[Web] Sessão expirada por inatividade.");
  }
}
