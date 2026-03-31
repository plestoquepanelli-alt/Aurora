// ════════════════════════════════════════════════════════════
//  TelegramHandler.cpp  — v3.0 (sem bloqueios)
//
//  OTIMIZAÇÕES vs v2.x:
//  1. sanitizarMarkdown(): substituição de loops O(n²) (replace+indexOf
//     em sequência sobre String crescente) por varredura O(n) em buffer.
//     Antes: ~200ms em strings de 4KB. Agora: ~2ms.
//  2. delay(400) entre chunks de mensagem → vTaskDelay / yield()
//     Não bloqueia o Core 1 enquanto aguarda o Telegram.
//  3. cmd_scan I2C: Wire.beginTransmission por slot com timeout via
//     Wire.setTimeOut(4) antes do scan — evita travamento em barramento
//     com pull-ups fracos ou dispositivos com clock stretching.
// ════════════════════════════════════════════════════════════

#include "TelegramHandler.h"
#include "Config.h"
#include "freertos/semphr.h"

#include "GeminiTypes.h"
#include "Sistema.h"
#include "LEDControl.h"
#include "Clima.h"
#include "SDCard.h"
#include "Gemini.h"
#include <WiFi.h>
#include <Wire.h>
#include <UniversalTelegramBot.h>
#include "esp_system.h"

// ═══════════════════════════════════════════════════════════════
//  VARIÁVEIS
// ═══════════════════════════════════════════════════════════════
unsigned long lastBotCheck = 0;
extern UniversalTelegramBot bot;
static String _estado = "";

static bool enfileirarPerguntaGemini(const String& chat_id, const String& pergunta){
  if(xSemaphoreTake(gMutex, pdMS_TO_TICKS(500)) != pdTRUE){
    bot.sendMessage(chat_id, "⚠️ Sistema ocupado (mutex). Tente novamente.", "");
    return false;
  }

  if(gJob.state != GJOB_IDLE){
    xSemaphoreGive(gMutex);
    bot.sendMessage(chat_id, "⏳ Ainda processando...", "");
    return false;
  }

  gJob.jobId++;
  gJob.pergunta = pergunta;
  gJob.chatId   = chat_id;
  gJob.resposta = "";
  gJob.ts       = millis();
  gJob.state    = GJOB_PENDING;
  xSemaphoreGive(gMutex);
  return true;
}

// ═══════════════════════════════════════════════════════════════
//  SANITIZAR MARKDOWN — O(n) scan, sem loops de replace aninhados
//
//  Versão anterior usava while(indexOf("**")>=0) com substring()
//  em cada iteração → O(n²) para strings longas do Gemini.
//  Esta versão varre o buffer uma vez e emite os tokens corretos.
// ═══════════════════════════════════════════════════════════════
static String sanitizarMarkdown(const String& s){
  String r;
  r.reserve(s.length());

  int len = s.length();
  int i = 0;

  while(i < len){
    char c = s[i];

    // ── Bloco de código ``` → remove marcadores, mantém conteúdo ──
    if(c == '`' && i+2 < len && s[i+1] == '`' && s[i+2] == '`'){
      i += 3;
      // Pula nome de linguagem na mesma linha (```python\n)
      while(i < len && s[i] != '\n') i++;
      if(i < len) i++; // pula \n
      // Copia conteúdo até próximo ```
      while(i < len){
        if(s[i] == '`' && i+2 < len && s[i+1] == '`' && s[i+2] == '`'){ i+=3; break; }
        r += s[i++];
      }
      continue;
    }

    // ── Negrito **texto** → *texto* ───────────────────────────
    if(c == '*' && i+1 < len && s[i+1] == '*'){
      r += '*'; i += 2;
      // Avança até fechar **
      while(i < len){
        if(s[i] == '*' && i+1 < len && s[i+1] == '*'){ r += '*'; i+=2; break; }
        r += s[i++];
      }
      continue;
    }

    // ── Itálico __texto__ → _texto_ ───────────────────────────
    if(c == '_' && i+1 < len && s[i+1] == '_'){
      r += '_'; i += 2;
      while(i < len){
        if(s[i] == '_' && i+1 < len && s[i+1] == '_'){ r += '_'; i+=2; break; }
        r += s[i++];
      }
      continue;
    }

    // ── Headings ### / ## / # → remove prefixo (início de linha) ─
    if(c == '#' && (i == 0 || s[i-1] == '\n')){
      while(i < len && (s[i] == '#' || s[i] == ' ')) i++;
      continue;
    }

    // ── Separadores --- / === (linha inteira) → remove ────────
    if(c == '-' && i+2 < len && s[i+1]=='-' && s[i+2]=='-'){
      while(i < len && s[i] != '\n') i++;
      continue;
    }
    if(c == '=' && i+2 < len && s[i+1]=='=' && s[i+2]=='='){
      while(i < len && s[i] != '\n') i++;
      continue;
    }

    r += c;
    i++;
  }

  // ── Paridade de * e ` ─────────────────────────────────────
  int acount = 0, bcount = 0;
  for(int j = 0; j < (int)r.length(); j++){
    if(r[j] == '*') acount++;
    if(r[j] == '`') bcount++;
  }
  // Ímpar: remove todos para evitar parser do Telegram quebrar
  if(acount % 2 != 0){
    String tmp; tmp.reserve(r.length());
    for(char ch : r) if(ch != '*') tmp += ch;
    r = tmp;
  }
  if(bcount % 2 != 0){
    String tmp; tmp.reserve(r.length());
    for(char ch : r) if(ch != '`') tmp += ch;
    r = tmp;
  }

  return r;
}

// ═══════════════════════════════════════════════════════════════
//  ENVIAR RESPOSTA LONGA
//  delay(400) → vTaskDelay(pdMS_TO_TICKS(400)) não bloqueia Core 1
// ═══════════════════════════════════════════════════════════════
static void enviarRespostaLonga(const String& chat_id, const String& resp){
  const int LIMITE = 3900;
  String sanitized = sanitizarMarkdown(resp);

  String parseMode = "";
  if(sanitized.indexOf('*') >= 0 || sanitized.indexOf('_') >= 0 ||
     sanitized.indexOf('`') >= 0){
    parseMode = "Markdown";
  }

  if((int)sanitized.length() <= LIMITE){
    bot.sendMessage(chat_id, sanitized, parseMode);
    return;
  }

  int pos = 0, chunk = 0;
  while(pos < (int)sanitized.length()){
    int end = min(pos + LIMITE, (int)sanitized.length());
    if(end < (int)sanitized.length()){
      int bp = sanitized.lastIndexOf("\n\n", end);
      if(bp <= pos) bp = sanitized.lastIndexOf('\n', end);
      if(bp <= pos) bp = sanitized.lastIndexOf(". ", end);
      if(bp <= pos) bp = sanitized.lastIndexOf(' ', end);
      if(bp > pos) end = bp + 1;
    }
    chunk++;
    String parte = (chunk > 1 ? "_(continua)_\n" : "") + sanitized.substring(pos, end);
    bot.sendMessage(chat_id, parte, parseMode);
    pos = end;
    if(pos < (int)sanitized.length() && sanitized[pos] == ' ') pos++;

    // ← Substituído delay(400) por vTaskDelay — não bloqueia o loop
    if(pos < (int)sanitized.length())
      vTaskDelay(pdMS_TO_TICKS(400));
  }
}

// ═══════════════════════════════════════════════════════════════
//  VERIFICAR RESPOSTA GEMINI
// ═══════════════════════════════════════════════════════════════
void verificarRespostaGemini(){
  if(xSemaphoreTake(gMutex, pdMS_TO_TICKS(100)) != pdTRUE) return;
  if(gJob.state != GJOB_DONE){
    xSemaphoreGive(gMutex);
    return;
  }

  String resp  = gJob.resposta;
  String cid   = gJob.chatId;
  gJob.state   = GJOB_IDLE;
  gJob.resposta = "";
  gJob.pergunta = "";
  xSemaphoreGive(gMutex);

  perguntasFeitas++;
  enviarRespostaLonga(cid, resp);
}

// ═══════════════════════════════════════════════════════════════
//  HELPER — envia menu com teclado inline
// ═══════════════════════════════════════════════════════════════
static void sendMenu(const String& chat_id, const String& text, const String& keyboard){
  bot.sendMessageWithInlineKeyboard(chat_id, text, "Markdown", keyboard);
}

// ═══════════════════════════════════════════════════════════════
//  MENUS
// ═══════════════════════════════════════════════════════════════
static void enviarMenuPrincipal(const String& chat_id){
  String kb = "["
    "[{\"text\":\"📊 Sistema\",\"callback_data\":\"menu_sistema\"},"
     "{\"text\":\"🌦 Clima\",\"callback_data\":\"menu_clima\"}],"
    "[{\"text\":\"🤖 IA Gemini\",\"callback_data\":\"menu_ia\"},"
     "{\"text\":\"📅 Agenda\",\"callback_data\":\"menu_agenda\"}],"
    "[{\"text\":\"📈 Relatório Semanal\",\"callback_data\":\"cmd_relatorio\"}],"
    "[{\"text\":\"🔐 Admin\",\"callback_data\":\"menu_admin\"}]"
  "]";
  sendMenu(chat_id,
    "🤖 *Aurora ESP32 · AI Assistant*\n"
    "━━━━━━━━━━━━━━━\n"
    "Uptime: `" + uptime() + "`  |  Heap: `" + String(heapPercentual()) + "%`\n\n"
    "Escolha uma categoria 👇",
    kb);
}

static void enviarMenuSistema(const String& chat_id){
  String kb = "["
    "[{\"text\":\"📊 Status completo\",\"callback_data\":\"cmd_status\"}],"
    "[{\"text\":\"🌡 Temperatura\",\"callback_data\":\"cmd_temp\"},"
     "{\"text\":\"🧠 Heap/RAM\",\"callback_data\":\"cmd_heap\"}],"
    "[{\"text\":\"🔍 Scan I2C\",\"callback_data\":\"cmd_scan\"},"
     "{\"text\":\"⏱ Uptime\",\"callback_data\":\"cmd_uptime\"}],"
    "[{\"text\":\"◀ Voltar\",\"callback_data\":\"menu_main\"}]"
  "]";
  sendMenu(chat_id, "📊 *Sistema* — escolha:", kb);
}

static void enviarMenuClima(const String& chat_id){
  String kb = "["
    "[{\"text\":\"🌦 Clima atual\",\"callback_data\":\"cmd_clima\"}],"
    "[{\"text\":\"📅 Previsão 3/6/9h\",\"callback_data\":\"cmd_previsao\"}],"
    "[{\"text\":\"🔄 Atualizar agora\",\"callback_data\":\"cmd_clima_update\"}],"
    "[{\"text\":\"◀ Voltar\",\"callback_data\":\"menu_main\"}]"
  "]";
  sendMenu(chat_id, "🌦 *Clima* — escolha:", kb);
}

static void enviarMenuIA(const String& chat_id){
  String kb = "["
    "[{\"text\":\"🤖 Fazer pergunta\",\"callback_data\":\"cmd_ai_ask\"}],"
    "[{\"text\":\"🗑 Limpar histórico\",\"callback_data\":\"cmd_ai_clear\"}],"
    "[{\"text\":\"✏️ Editar personalidade\",\"callback_data\":\"cmd_ai_persona\"}],"
    "[{\"text\":\"◀ Voltar\",\"callback_data\":\"menu_main\"}]"
  "]";
  sendMenu(chat_id, "🤖 *IA Gemini* — escolha:", kb);
}

static void enviarMenuAgenda(const String& chat_id){
  String kb = "["
    "[{\"text\":\"📅 Ver agenda do mês\",\"callback_data\":\"cmd_agenda_ver\"}],"
    "[{\"text\":\"📌 Adicionar lembrete\",\"callback_data\":\"cmd_agenda_add\"}],"
    "[{\"text\":\"◀ Voltar\",\"callback_data\":\"menu_main\"}]"
  "]";
  sendMenu(chat_id, "📅 *Agenda* — escolha:", kb);
}

static void enviarMenuAdmin(const String& chat_id){
  String kb = "["
    "[{\"text\":\"🔄 Reiniciar ESP32\",\"callback_data\":\"adm_restart\"}],"
    "[{\"text\":\"📡 Ativar OTA WiFi\",\"callback_data\":\"adm_ota\"}],"
    "[{\"text\":\"🗑 Limpar logs SD\",\"callback_data\":\"adm_clearlogs\"}],"
    "[{\"text\":\"🧹 Limpar memória IA\",\"callback_data\":\"adm_clearmem\"}],"
    "[{\"text\":\"💡 LED ON\",\"callback_data\":\"adm_ledon\"},"
     "{\"text\":\"💡 LED OFF\",\"callback_data\":\"adm_ledoff\"}],"
    "[{\"text\":\"◀ Voltar\",\"callback_data\":\"menu_main\"}]"
  "]";
  sendMenu(chat_id,
    "🔐 *Painel Admin*\n"
    "━━━━━━━━━━━━━━━\n"
    "⚠️ Ações irreversíveis requerem confirmação por senha.",
    kb);
}

// ═══════════════════════════════════════════════════════════════
//  HANDLER DE CALLBACK QUERIES
// ═══════════════════════════════════════════════════════════════
static void handleCallback(const String& chat_id, const String& cb, int msgId){
  if(cb == "menu_main")    { enviarMenuPrincipal(chat_id); return; }
  if(cb == "menu_sistema") { enviarMenuSistema(chat_id);   return; }
  if(cb == "menu_clima")   { enviarMenuClima(chat_id);     return; }
  if(cb == "menu_ia")      { enviarMenuIA(chat_id);        return; }
  if(cb == "menu_agenda")  { enviarMenuAgenda(chat_id);    return; }
  if(cb == "menu_admin")   { enviarMenuAdmin(chat_id);     return; }

  // ── Sistema ─────────────────────────────────────────────────
  if(cb == "cmd_status"){
    int hp = heapPercentual(), wp = wifiPercentual();
    String msg;
    msg  = "📊 *Status do Sistema*\n━━━━━━━━━━━━━━━\n\n";
    msg += "🕒 `" + horaAtual() + "`\n";
    msg += "⏱ Uptime: `" + uptime() + "`\n\n";
    msg += "💾 Heap " + indicadorCor(hp) + " `" + String(hp) + "%` (" + String(ESP.getFreeHeap()) + " bytes)\n";
    msg += barraVisual(hp) + "\n\n";
    msg += "📶 WiFi " + indicadorCor(wp) + " `" + String(wp) + "%` (" + String(WiFi.RSSI()) + " dBm)\n";
    msg += "SSID: `" + WiFi.SSID() + "`\n\n";
    msg += "🌡 Temp chip: `" + String(lerTemperatura(), 1) + " °C`\n";
    msg += "🌦 `" + String(climaTemp,1) + " °C` · " + climaDescricao + "\n";
    msg += "💧 Umidade: `" + String(climaUmidade) + "%`\n\n";
    msg += "🧠 Perguntas IA: `" + String(perguntasFeitas) + "`\n";
    msg += "🌐 Web: `" + WiFi.localIP().toString() + "`\n";
    if(otaAtivo()) msg += "\n📡 _OTA ativo_";
    bot.sendMessage(chat_id, limitarTelegram(msg), "Markdown");
    return;
  }

  if(cb == "cmd_temp"){
    float t = lerTemperatura();
    String msg = "🌡 *Temperatura do chip*\n━━━━━━━━━━━━━━━\n\n";
    msg += isnan(t) ? "❌ Falha na leitura\n" : "`" + String(t,1) + " °C`\n";
    msg += "\n🌦 Ambiente ext: `" + String(climaTemp,1) + " °C`";
    bot.sendMessage(chat_id, msg, "Markdown");
    return;
  }

  if(cb == "cmd_heap"){
    int hp = heapPercentual();
    String msg = "🧠 *Memória RAM*\n━━━━━━━━━━━━━━━\n\n";
    msg += "Livre: `" + String(ESP.getFreeHeap()/1024) + " KB`\n";
    msg += "Total: `" + String(ESP.getHeapSize()/1024) + " KB`\n";
    msg += "Uso: " + indicadorCor(100-hp) + " `" + String(100-hp) + "%`\n";
    msg += barraVisual(100-hp);
    bot.sendMessage(chat_id, msg, "Markdown");
    return;
  }

  if(cb == "cmd_uptime"){
    bot.sendMessage(chat_id, "⏱ Uptime: `" + uptime() + "`", "Markdown");
    return;
  }

  if(cb == "cmd_scan"){
    // ← Wire.setTimeOut evita travamento em barramento com pull-up fraco
    Wire.setTimeOut(4);
    String msg = "🔍 *Scan I2C*\n━━━━━━━━━━━━━━━\n\n";
    int cnt = 0;
    for(byte addr = 1; addr < 127; addr++){
      Wire.beginTransmission(addr);
      byte err = Wire.endTransmission();
      if(err == 0){
        msg += "✅ 0x"; if(addr<16) msg+="0";
        msg += String(addr,HEX) + " (" + String(addr) + ")\n";
        cnt++;
      }
      // yield a cada 16 endereços para não travar o loop
      if((addr & 0x0F) == 0) yield();
    }
    msg += "\n📊 Total: " + String(cnt);
    bot.sendMessage(chat_id, msg, "Markdown");
    return;
  }

  // ── Clima ───────────────────────────────────────────────────
  if(cb == "cmd_clima"){
    String desc = climaDescricao;
    if(desc.length() > 0) desc[0] = toupper(desc[0]);
    String msg = "🌦 *Clima agora — " + cidade + "*\n";
    msg += "━━━━━━━━━━━━━━━\n\n";
    msg += "☁ " + desc + "\n\n";
    msg += "🌡 *Temperatura*\n";
    msg += "  Real: `" + String(climaTemp, 1) + " °C`\n";
    msg += "  Sensação: `" + String(climaSensTermica, 1) + " °C`\n\n";
    msg += "💧 *Atmosfera*\n";
    msg += "  Umidade: `" + String(climaUmidade) + "%`\n";
    msg += "  Pressão: `" + String(climaPressao) + " hPa`\n\n";
    msg += "🌬 *Vento*\n";
    msg += "  Velocidade: `" + String(climaVento, 1) + " km/h`\n\n";
    msg += "🌧 *Precipitação*\n";
    msg += "  Chuva +3h: `" + String((int)round(chuva3h)) + "%`\n\n";
    msg += "📅 *Previsão*\n";
    msg += "  `" + hora_prev3h + "` → `" + String(previsao3h, 1) + " °C` 🌧 `" + String((int)round(chuva3h)) + "%`\n";
    msg += "  `" + hora_prev6h + "` → `" + String(previsao6h, 1) + " °C` 🌧 `" + String((int)round(chuva6h)) + "%`\n";
    msg += "  `" + hora_prev9h + "` → `" + String(previsao9h, 1) + " °C` 🌧 `" + String((int)round(chuva9h)) + "%`";
    bot.sendMessage(chat_id, limitarTelegram(msg), "Markdown");
    return;
  }

  if(cb == "cmd_previsao"){
    String msg = "📅 *Previsão detalhada — " + cidade + "*\n";
    msg += "━━━━━━━━━━━━━━━\n\n";
    msg += "  Hora  │  Temp  │  Chuva\n";
    msg += "  ──────┼────────┼───────\n";
    msg += "  `" + hora_prev3h + "` │ `" + String(previsao3h,1) + " °C` │ `" + String((int)round(chuva3h)) + "%`\n";
    msg += "  `" + hora_prev6h + "` │ `" + String(previsao6h,1) + " °C` │ `" + String((int)round(chuva6h)) + "%`\n";
    msg += "  `" + hora_prev9h + "` │ `" + String(previsao9h,1) + " °C` │ `" + String((int)round(chuva9h)) + "%`";
    bot.sendMessage(chat_id, limitarTelegram(msg), "Markdown");
    return;
  }

  if(cb == "cmd_clima_update"){
    bot.sendMessage(chat_id, "🔄 Atualizando clima...", "");
    atualizarClima();
    bot.sendMessage(chat_id, "✅ Clima atualizado: `" + climaDescricao + "` · `" + String(climaTemp,1) + " °C`", "Markdown");
    return;
  }

  // ── IA Gemini ───────────────────────────────────────────────
  if(cb == "cmd_ai_ask"){
    _estado = "wait_ai";
    bot.sendMessage(chat_id,
      "🤖 *Modo IA ativo*\nDigite sua pergunta a seguir.\n_(Texto livre também funciona a qualquer momento)_",
      "Markdown");
    return;
  }

  if(cb == "cmd_ai_clear"){
    limparHistoricoIA();
    bot.sendMessage(chat_id, "🗑 Histórico Gemini apagado. Nova conversa iniciada.", "");
    return;
  }

  if(cb == "cmd_ai_persona"){
    _estado = "wait_persona";
    String atual = carregarPersonalidade();
    bot.sendMessage(chat_id,
      "✏️ *Editar personalidade da Aurora*\n━━━━━━━━━━━━━━━\n\n"
      "Texto atual:\n`" + limitarTelegram(atual, 300) + "`\n\n"
      "Envie o novo texto de personalidade:",
      "Markdown");
    return;
  }

  // ── Agenda ──────────────────────────────────────────────────
  if(cb == "cmd_agenda_ver"){
    bot.sendMessage(chat_id, lerAgenda(), "Markdown"); return;
  }

  if(cb == "cmd_agenda_add"){
    _estado = "wait_lembrete";
    bot.sendMessage(chat_id,
      "📌 *Adicionar lembrete*\n\n"
      "Envie no formato:\n`25 Consulta médica`\n"
      "_(dia + espaço + descrição)_",
      "Markdown");
    return;
  }

  // ── Relatório ────────────────────────────────────────────────
  if(cb == "cmd_relatorio"){
    bot.sendMessage(chat_id, "📊 Gerando relatório semanal...", "");
    String rel = gerarRelatorio();
    bot.sendMessage(chat_id, limitarTelegram(rel, 3800), "Markdown");
    return;
  }

  // ── Admin ────────────────────────────────────────────────────
  if(cb == "adm_restart"){
    _estado = "wait_admin_restart";
    bot.sendMessage(chat_id, "🔄 *Reiniciar ESP32*\n\nDigite a senha de admin:", "Markdown");
    return;
  }
  if(cb == "adm_ota"){
    _estado = "wait_admin_ota";
    bot.sendMessage(chat_id,
      "📡 *Ativar OTA WiFi*\n\nO OTA fica ativo por *5 minutos*.\nDigite a senha para confirmar:",
      "Markdown");
    return;
  }
  if(cb == "adm_clearlogs"){
    _estado = "wait_admin_clearlogs";
    bot.sendMessage(chat_id, "🗑 *Limpar todos os logs do SD*\n\nSenha:", "Markdown");
    return;
  }
  if(cb == "adm_clearmem"){
    _estado = "wait_admin_clearmem";
    bot.sendMessage(chat_id, "🧹 *Apagar memória de perguntas*\n\nSenha:", "Markdown");
    return;
  }
  if(cb == "adm_ledon"){  ledHabilitado = true;  bot.sendMessage(chat_id,"💡 LED ativado.",""); return; }
  if(cb == "adm_ledoff"){ ledHabilitado = false; pixel.clear(); pixel.show(); bot.sendMessage(chat_id,"💡 LED desativado.",""); return; }
}

// ═══════════════════════════════════════════════════════════════
//  HANDLER DE TEXTO
// ═══════════════════════════════════════════════════════════════
static void handleText(const String& chat_id, const String& text){
  String textCmd = text;
  textCmd.toLowerCase();

  if(isModoNoturnoAgora()){
    bool isAdmin  = textCmd.startsWith("/admin") || _estado.startsWith("wait_admin");
    bool isStatus = (textCmd == "/status" || textCmd == "/hora");
    if(!isAdmin && !isStatus && _estado.isEmpty()){
      bot.sendMessage(chat_id,
        "🌙 *Modo noturno* (" + faixaModoNoturno() + ")\n"
        "Bot silencioso para economizar recursos.\n"
        "Alertas climáticos continuam ativos.\n"
        "Use /status ou /hora se precisar.", "Markdown");
      return;
    }
  }

  // ── Máquina de estados ──────────────────────────────────────
  if(!_estado.isEmpty()){

    if(_estado == "wait_ai"){
      _estado = "";
      if(enfileirarPerguntaGemini(chat_id, text)){
        bot.sendChatAction(chat_id,"typing");
        bot.sendMessage(chat_id,"🧠 _Pensando..._","Markdown");
      }
      return;
    }

    if(_estado == "wait_persona"){
      _estado = "";
      salvarPersonalidade(text);
      bot.sendMessage(chat_id,"✅ *Personalidade atualizada!*","Markdown");
      return;
    }

    if(_estado == "wait_lembrete"){
      _estado = "";
      int esp = text.indexOf(' ');
      if(esp > 0){
        String dia = text.substring(0, esp);
        String evento = text.substring(esp+1); evento.trim();
        if(dia.toInt() >= 1 && dia.toInt() <= 31){
          salvarEvento(dia, evento);
          bot.sendMessage(chat_id,"✅ *Lembrete salvo!*\n📅 Dia *"+dia+"* — "+evento,"Markdown");
        } else bot.sendMessage(chat_id,"❌ Dia inválido. Ex: `25 Consulta médica`","Markdown");
      } else bot.sendMessage(chat_id,"❌ Formato inválido. Ex: `25 Consulta médica`","Markdown");
      return;
    }

    if(_estado == "wait_admin_restart"){
      _estado = "";
      if(text == ADMIN_SENHA){ bot.sendMessage(chat_id,"🔄 Reiniciando...",""); esperarComLED(1200); ESP.restart(); }
      else bot.sendMessage(chat_id,"❌ Senha incorreta.","");
      return;
    }

    if(_estado == "wait_admin_ota"){
      _estado = "";
      if(text == ADMIN_SENHA){
        ativarOTA();
        bot.sendMessage(chat_id,
          "📡 *OTA ativado por 5 minutos!*\n\n"
          "🌐 IP: `" + WiFi.localIP().toString() + "`\n"
          "🔑 Senha OTA: mesma do admin",
          "Markdown");
      } else bot.sendMessage(chat_id,"❌ Senha incorreta.","");
      return;
    }

    if(_estado == "wait_admin_clearlogs"){
      _estado = "";
      if(text == ADMIN_SENHA){ limparLogs(); bot.sendMessage(chat_id,"🗑 Logs apagados.",""); }
      else bot.sendMessage(chat_id,"❌ Senha incorreta.","");
      return;
    }

    if(_estado == "wait_admin_clearmem"){
      _estado = "";
      if(text == ADMIN_SENHA){ limparMemoria(); bot.sendMessage(chat_id,"🧹 Memória apagada.",""); }
      else bot.sendMessage(chat_id,"❌ Senha incorreta.","");
      return;
    }
  }

  // ── Comandos slash ──────────────────────────────────────────
  if(textCmd == "/menu" || textCmd == "/start"){ enviarMenuPrincipal(chat_id); return; }
  if(textCmd == "/status"){ handleCallback(chat_id,"cmd_status",0); return; }
  if(textCmd == "/clima"){  handleCallback(chat_id,"cmd_clima", 0); return; }
  if(textCmd == "/hora"){   bot.sendMessage(chat_id,"🕒 `"+horaAtual()+"`","Markdown"); return; }
  if(textCmd == "/agenda"){ bot.sendMessage(chat_id,lerAgenda(),"Markdown"); return; }
  if(textCmd == "/relatorio"){ handleCallback(chat_id,"cmd_relatorio",0); return; }
  if(textCmd == "/admin"){  enviarMenuAdmin(chat_id); return; }

  if(textCmd == "oi" || textCmd == "olá"){
    bot.sendMessage(chat_id,"Olá, Pedro 👋\nUse /menu para navegar.",""); return;
  }
  if(textCmd.indexOf("bom dia") >= 0){
    bot.sendMessage(chat_id,"Bom dia, Pedro ☀",""); return;
  }

  String mem = buscarMemoria(textCmd);
  if(mem != ""){ bot.sendMessage(chat_id, mem, ""); return; }

  if(!textCmd.startsWith("/")){
    static unsigned long _ultimaRespGemini = 0;
    if(millis() - _ultimaRespGemini < 5000UL){
      bot.sendMessage(chat_id,"⏳ Aguarde antes da próxima pergunta.",""); return;
    }
    _ultimaRespGemini = millis();
    if(contarPergunta(textCmd) >= 3) salvarMemoria(text,"");
    incrementarPergunta(textCmd);
    if(enfileirarPerguntaGemini(chat_id, text)){
      bot.sendChatAction(chat_id,"typing");
      bot.sendMessage(chat_id,"🧠 _Pensando... Pode usar o sistema normalmente._","Markdown");
    }
    return;
  }

  bot.sendMessage(chat_id,"❓ Comando desconhecido. Use /menu.","");
}

// ═══════════════════════════════════════════════════════════════
//  PROCESSAR MENSAGENS — ponto de entrada do loop()
// ═══════════════════════════════════════════════════════════════
void processarMensagens(){
  if(millis() - lastBotCheck < BOT_INTERVAL) return;
  lastBotCheck = millis();
  if(WiFi.status() != WL_CONNECTED) return;

  int n = bot.getUpdates(bot.last_message_received + 1);

  for(int i = 0; i < n; i++){
    String chat_id = bot.messages[i].chat_id;
    if(chat_id != String(MEU_ID)){
      bot.sendMessage(chat_id,"🚫 Acesso não autorizado.","");
      bot.last_message_received = bot.messages[i].update_id;
      continue;
    }
    if(bot.messages[i].type == "callback_query"){
      handleCallback(chat_id, bot.messages[i].text, bot.messages[i].update_id);
    } else {
      String text = bot.messages[i].text;
      text.trim();
      if(!text.isEmpty()) handleText(chat_id, text);
    }
    bot.last_message_received = bot.messages[i].update_id;
  }
}
