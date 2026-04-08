/*
 * ════════════════════════════════════════════════════════════
 *         AURORA ESP32-S3 · AI Personal Assistant
 *         v3.3 — Pedro Panelli
 * ════════════════════════════════════════════════════════════
 *
 *  BOARD SETTINGS:
 *  Board:      ESP32S3 Dev Module
 *  Partition:  Huge APP (3MB No OTA/1MB SPIFFS)
 *  PSRAM:      OPI PSRAM
 *  CPU:        240 MHz
 *  USB CDC:    Enabled
 *
 *  BOTÕES (INPUT_PULLUP — LOW = pressionado):
 *  Pino 14 — Agenda:   compromissos do dia no OLED (30s)
 *  Pino  3 — Display:  toggle liga/desliga OLED
 *  Pino 46 — Web Info: IP do painel web no OLED (30s)
 *             duplo clique ≤ 1.2s → ativa/desativa AP config
 *
 *  ARQUITETURA DUAL-CORE:
 *  Core 1 (loop): display, LED, botões, clima, Telegram, Web
 *  Core 0 (taskGemini): HTTP ao Gemini — não bloqueia Core 1
 *
 *  CORREÇÕES v3.3:
 *  - Display.cpp: yield() removido de dentro de PROGMEM array
 *  - Gemini.cpp:  http.end()/yield() inacessíveis após return removidos
 *  - TelegramHandler.cpp: processarMensagens() corrigido —
 *    secured_client.setTimeout() não retorna int (era bool);
 *    bot.getUpdates() tinha retorno descartado → mensagens perdidas
 *  - loop(): guards tTelegram e tINMET redundantes removidos
 *    (BOT_INTERVAL já controla Telegram; INMET sempre dispara com clima)
 * ════════════════════════════════════════════════════════════
 */

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <UniversalTelegramBot.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoOTA.h>
#include <time.h>
#include "driver/temperature_sensor.h"
#include <math.h>
#include <ArduinoJson.h>
#include "esp_system.h"
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

// ── Botões ────────────────────────────────────────────────────
#define BTN_AGENDA  14
#define BTN_DISPLAY  3
#define BTN_WEB     46

// ── Módulos ───────────────────────────────────────────────────
#include "Config.h"
#include "SDCard.h"
#include "LEDControl.h"
#include "Sistema.h"
#include "Clima.h"
#include "Display.h"
#include "GeminiTypes.h"
#include "Gemini.h"
#include "TelegramHandler.h"
#include "AlertaINMET.h"
#include "WebAurora.h"

// ── Variáveis globais ─────────────────────────────────────────
String modeloAtivo    = "gemini-2.5-flash";
String cidade         = "Muriae,BR";
unsigned long perguntasFeitas = 0;
unsigned long bootTime        = 0;
bool oledLigado               = true;

// ── Clientes HTTPS ────────────────────────────────────────────
WiFiClientSecure secured_client;
WiFiClientSecure gemini_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);

// ── Gemini async ──────────────────────────────────────────────
GeminiJob         gJob;
SemaphoreHandle_t gMutex;

// ════════════════════════════════════════════════════════════
//  DEBOUNCE — dispara somente na borda de pressão (LOW)
//  Cada botão mantém suas próprias variáveis como statics
//  no bloco local — não há estado compartilhado entre botões.
// ════════════════════════════════════════════════════════════
static inline bool btnDebounce(
    uint8_t pin,
    bool &stable,
    bool &lastRaw,
    unsigned long &lastChange,
    unsigned long ms)
{
    bool raw = (digitalRead(pin) == LOW);
    if (lastChange == 0) {
        stable = raw; lastRaw = raw; lastChange = millis(); return false;
    }
    if (raw != lastRaw) { lastRaw = raw; lastChange = millis(); }
    if ((millis() - lastChange) >= ms && raw != stable) {
        stable = raw;
        if (stable) return true;  // borda de pressão
    }
    return false;
}

// ════════════════════════════════════════════════════════════
//  HELPER — tela Config-AP no OLED
// ════════════════════════════════════════════════════════════
static void exibirAPConfigOLED() {
    display.clearDisplay();
    display.fillRect(0, 0, 128, 12, SH110X_WHITE);
    display.setTextColor(SH110X_BLACK);
    display.setCursor(14, 2); display.setTextSize(1);
    display.print("WIFI CONFIG AP");
    display.setTextColor(SH110X_WHITE);
    display.setCursor(2, 18); display.print(nomeAPConfig());
    display.setCursor(2, 30); display.print("Senha: aurora123");
    display.setCursor(2, 42); display.print("http://192.168.4.1");
    display.display();
}

// ════════════════════════════════════════════════════════════
//  TASK GEMINI — Core 0
//  Executa perguntarGemini() de forma assíncrona para não
//  bloquear o Core 1 durante as requisições HTTP longas.
// ════════════════════════════════════════════════════════════
static void taskGemini(void* pv) {
    struct Pending {
        bool          active    = false;
        uint32_t      jobId     = 0;
        String        chatId;
        String        resposta;
        unsigned long ts        = 0;
        unsigned long startedAt = 0;
        uint16_t      retries   = 0;
    };
    static Pending pend;

    // Tenta publicar resultado no gJob compartilhado com Core 1
    auto publicar = [&](Pending &p) -> bool {
        if (!p.active) return true;
        if (xSemaphoreTake(gMutex, pdMS_TO_TICKS(200)) != pdTRUE) {
            p.retries++; return false;
        }
        bool ok = false;
        if (gJob.jobId == p.jobId && gJob.chatId == p.chatId
            && gJob.state == GJOB_RUNNING) {
            gJob.resposta = p.resposta;
            gJob.state    = GJOB_DONE;
            ok = true;
        } else {
            Serial.printf("[Core0] job descartado — esperado=%lu atual=%lu\n",
                          (unsigned long)p.jobId, (unsigned long)gJob.jobId);
            ok = true;  // descarta sem travar pipeline
        }
        xSemaphoreGive(gMutex);
        if (ok) {
            Serial.printf("[Core0] done %dms retries=%u\n",
                          (int)(millis()-p.ts), p.retries);
            p = Pending{};
        }
        return ok;
    };

    for (;;) {
        // Esvazia resultado pendente antes de aceitar novo job
        if (pend.active) {
            publicar(pend);
            if (pend.active && millis() - pend.startedAt > 10000UL) {
                Serial.println("[Core0] timeout — descartando pendente");
                pend = Pending{};
            }
            vTaskDelay(pdMS_TO_TICKS(30));
            continue;
        }

// Definições reais dos objetos globais (extern em GeminiTypes.h)
GeminiJob          gJob;
SemaphoreHandle_t  gMutex;

static inline bool botaoPressionadoDebounced(
    uint8_t pin,
    bool &stableState,
    bool &lastRawState,
    unsigned long &lastChangeMs,
    unsigned long debounceMs){
    bool rawPressed = (digitalRead(pin) == LOW);
    if(lastChangeMs == 0){
        stableState = rawPressed;
        lastRawState = rawPressed;
        lastChangeMs = millis();
        return false;
    }
    if(rawPressed != lastRawState){
        lastRawState = rawPressed;
        lastChangeMs = millis();
    }
    if((millis() - lastChangeMs) >= debounceMs && rawPressed != stableState){
        stableState = rawPressed;
        if(stableState) return true;
    }
    return false;
}

static void taskGemini(void* pv){
    struct PendingResultado {
        bool active = false;
        uint32_t jobId = 0;
        String chatId = "";
        String resposta = "";
        unsigned long ts = 0;
        unsigned long startedAt = 0;
        uint16_t retries = 0;
    };
    static PendingResultado pend;

    auto tentarPublicarResultado = [&](PendingResultado &p) -> bool {
        if(!p.active) return true;
        if(xSemaphoreTake(gMutex, pdMS_TO_TICKS(200)) != pdTRUE){
            p.retries++;
            return false;
        }

        bool publicado = false;
        // Fluxo normal: ainda é o mesmo job em execução.
        if(gJob.jobId == p.jobId && gJob.chatId == p.chatId && gJob.state == GJOB_RUNNING){
            gJob.resposta = p.resposta;
            gJob.state    = GJOB_DONE;
            publicado = true;
        } else {
            // Segurança: se o job já mudou, nunca sobrescreve o job atual.
            // Isso evita entregar resposta antiga no chat errado.
            Serial.printf("[Core0] Gemini resultado descartado (job mudou). esperado=%lu atual=%lu\n",
                          (unsigned long)p.jobId, (unsigned long)gJob.jobId);
            publicado = true; // consome pendente para não travar pipeline
        }

        xSemaphoreGive(gMutex);
        if(publicado){
            Serial.printf("[Core0] Gemini done %dms (retries=%u)\n",
                          (int)(millis() - p.ts), p.retries);
            p = PendingResultado{};
        }
        return publicado;
    };

    for(;;){
        if(pend.active){
            tentarPublicarResultado(pend);
            if(pend.active && (millis() - pend.startedAt > 10000UL)){
                Serial.println("[Core0] Gemini timeout fallback: finalizando job com erro amigavel");
                if(xSemaphoreTake(gMutex, pdMS_TO_TICKS(200)) == pdTRUE){
                    if(gJob.jobId == pend.jobId && gJob.chatId == pend.chatId && gJob.state == GJOB_RUNNING){
                        gJob.resposta = "Falha ao entregar resposta do Gemini (timeout interno). Tente novamente.";
                        gJob.state = GJOB_DONE;
                    } else if(gJob.state == GJOB_RUNNING && gJob.jobId == pend.jobId){
                        gJob.state = GJOB_IDLE;
                    }
                    xSemaphoreGive(gMutex);
                }
                pend = PendingResultado{};
            }
            vTaskDelay(pdMS_TO_TICKS(30));
            continue;
        }

        bool hasJob = false;
        String pergunta = "";
        String chatId = "";
        unsigned long ts = 0;
        uint32_t jobId = 0;

        if(xSemaphoreTake(gMutex, pdMS_TO_TICKS(100)) == pdTRUE){
            if(gJob.state == GJOB_PENDING){
                gJob.state = GJOB_RUNNING;
                pergunta = gJob.pergunta;
                chatId   = gJob.chatId;
                ts       = gJob.ts;
                jobId    = gJob.jobId;
                hasJob   = true;
            }
            xSemaphoreGive(gMutex);
        }

        if(hasJob){
            Serial.printf("[Core0] Gemini start heap=%d\n", ESP.getFreeHeap());
            String resp = perguntarGemini(pergunta);
            if(resp.length() == 0){
                resp = "Sem resposta do Gemini. Tente novamente.";
            }
            pend.active = true;
            pend.jobId = jobId;
            pend.chatId = chatId;
            pend.resposta = resp;
            pend.ts = ts;
            pend.startedAt = millis();
            pend.retries = 0;
            tentarPublicarResultado(pend);
        }
        vTaskDelay(pdMS_TO_TICKS(30));
    }
}

// ════════════════════════════════════════════════════════════
//  SETUP
// ════════════════════════════════════════════════════════════
void setup() {
    Serial.begin(115200);
    bootTime = millis();

    initLED();

    secured_client.setInsecure();
    gemini_client.setInsecure();
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);

    conectarWiFi();
    Serial.print("Conectando WiFi");
    uint32_t wt = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - wt < 15000) {
        updateLED(); delay(200); Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("WiFi OK: " + WiFi.localIP().toString());
        sincronizarHorario();
        delay(800);
        atualizarClima();          // busca clima já no boot
        ultimoClima = millis();
        initWebServer();
    } else {
        Serial.println("WiFi falhou — offline");
    }

    initTemperatura();
    sdOK = initSD();
    if (!sdOK) Serial.println("SD nao disponivel");

    // Drena updates antigos do Telegram
    { int n = bot.getUpdates(0);
      if (n > 0) bot.last_message_received = bot.messages[n-1].update_id; }

    pinMode(BTN_AGENDA,  INPUT_PULLUP);
    pinMode(BTN_DISPLAY, INPUT_PULLUP);
    pinMode(BTN_WEB,     INPUT_PULLUP);

    initDisplay();

    gMutex = xSemaphoreCreateMutex();
    xTaskCreatePinnedToCore(taskGemini, "GeminiTask", 24576, NULL, 1, NULL, 0);
    Serial.println("GeminiTask → Core 0");

    enviarMenu();
}

// ════════════════════════════════════════════════════════════
//  LOOP — Core 1
//  Toda temporização usa millis() — nenhum delay() bloqueante.
//  Serviços HTTP (Gemini) rodam no Core 0 de forma assíncrona.
// ════════════════════════════════════════════════════════════
void loop() {

    // ── Hora lida uma vez por ciclo ──────────────────────────
    struct tm _lt;
    bool  _ok      = getLocalTime(&_lt);
    int   _diaHoje = _ok ? _lt.tm_mday : 0;
    bool  _noturno = isModoNoturnoAgora();

    // ── Serviços sempre ativos ───────────────────────────────
    loopOTA();
    checkWiFi();
    updateLED();
    verificarRespostaGemini();     // despacha resposta do Core 0 → Telegram

    // WebServer: throttle 50ms evita saturação de CPU com sessão ativa
    static unsigned long tWeb = 0;
    if (millis() - tWeb > 50) { handleWebServer(); tWeb = millis(); }

    // ── Log sensor a cada 5 min ──────────────────────────────
    static unsigned long tSensor = 0;
    if (millis() - tSensor > 300000UL) {
        tSensor = millis();
        sdLogSensor(lerTemperatura(), ESP.getFreeHeap()/1024, heapPercentual());
    }

    // ── Display ───────────────────────────────────────────────
    // Flags declaradas em escopo único: não há duas variáveis
    // "static bool _apagouManual" em blocos if/else distintos
    static unsigned long lastDisplay    = 0;
    static bool          agendaVisivel  = false;
    static bool          webInfoVisivel = false;
    static unsigned long webInfoTimer   = 0;
    static bool          _apagouManual  = false;
    static bool          _apagouNoturno = false;

    if (webInfoVisivel && millis() - webInfoTimer > 30000UL) {
        webInfoVisivel = false;
        lastDisplay = 0; // força retorno imediato ao display normal
    }

    if (!oledLigado) {
        if (!_apagouManual) { display.clearDisplay(); display.display(); _apagouManual = true; }
        _apagouNoturno = false;
    } else {
        _apagouManual = false;  // reseta: permite apagar na próxima vez
        if (webInfoVisivel) {
            /* já desenhado pelo botão — não sobrescreve */
        } else if (_noturno) {
            if (!_apagouNoturno) { display.clearDisplay(); display.display(); _apagouNoturno = true; }
        } else {
            _apagouNoturno = false;
            if (!agendaVisivel && millis() - lastDisplay > 100) {
                atualizarDisplay();
                lastDisplay = millis();
            }
        }
    }

    // ── Clima + INMET ─────────────────────────────────────────
    // intervalo duplo quando há sessão web ativa (evita contenção HTTP)
    unsigned long intervaloClima = webSessaoAtiva
        ? (INTERVALO_CLIMA * 2)
        :  INTERVALO_CLIMA;

    if (millis() - ultimoClima > intervaloClima) {
        ultimoClima = millis();  // atualiza antes das chamadas HTTP
        atualizarClima();        // busca clima atual + previsão
        verificarAlertaINMET();  // verifica alertas INMET (sempre junto)
    }

    // ── Telegram ──────────────────────────────────────────────
    // Modo noturno: apenas lembretes são verificados; mensagens ignoradas
    if (!_noturno) {
        verificarLembretes();
        // Com sessão web ativa: polling reduzido a cada 10s
        if (webSessaoAtiva) {
            static unsigned long tTgWeb = 0;
            if (millis() - tTgWeb > 10000UL) { tTgWeb = millis(); processarMensagens(); }
        } else {
            processarMensagens();  // BOT_INTERVAL (2500ms) controlado internamente
        }
    }

    // ════════════════════════════════════════════════════════
    //  BOTÃO 1 — AGENDA (pino 14)
    //  Exibe compromissos do dia no OLED por 30s
    // ════════════════════════════════════════════════════════
    {
        static unsigned long tBotao = 0, tExib = 0;
        static bool stableState = false, lastRawState = false;
        static unsigned long lastChange = 0;
        if(botaoPressionadoDebounced(BTN_AGENDA, stableState, lastRawState, lastChange, 45) && millis() - tBotao > 300){
            tBotao = tExib = millis();
            agendaVisivel  = true;
            webInfoVisivel = false;

            String agenda = lerAgenda();
            String hoje   = "";
            if (_diaHoje > 0) {
                int pos = 0;
                while (pos < (int)agenda.length()) {
                    int nl   = agenda.indexOf('\n', pos);
                    if (nl < 0) nl = agenda.length();
                    String l = agenda.substring(pos, nl); pos = nl + 1;
                    int dash = l.indexOf(" - ");
                    if (dash > 0 && l.substring(0, dash).toInt() == _diaHoje)
                        hoje += l.substring(dash + 3) + "\n";
                }
            }
            exibirAgendaOLED(hoje, _diaHoje);
        }
        if (agendaVisivel && millis() - tExib > 30000UL) agendaVisivel = false;
    }

    // ════════════════════════════════════════════════════════
    //  BOTÃO 2 — DISPLAY toggle (pino 3)
    // ════════════════════════════════════════════════════════
    {
        static unsigned long tBotaoDisp = 0;
        static bool stableState = false, lastRawState = false;
        static unsigned long lastChange = 0;
        if(botaoPressionadoDebounced(BTN_DISPLAY, stableState, lastRawState, lastChange, 45) && millis() - tBotaoDisp > 300){
            tBotaoDisp = millis();
            oledLigado = !oledLigado;
            Serial.printf("[BTN3] OLED %s\n", oledLigado ? "ON" : "OFF");
            if (oledLigado) lastDisplay = 0;
        }
    }

    // ════════════════════════════════════════════════════════
    //  BOTÃO 3 — WEB INFO (pino 46)
    //  Simples: exibe IP + senha do painel por 30s
    //  Duplo clique (≤ 1.2s): ativa/desativa AP de configuração
    // ════════════════════════════════════════════════════════
    {
        static unsigned long tBotaoMenu = 0;
        static bool stableState = false, lastRawState = false;
        static unsigned long lastChange = 0;
        if(botaoPressionadoDebounced(BTN_MENU, stableState, lastRawState, lastChange, 50) && millis() - tBotaoMenu > 400){
            tBotaoMenu = millis();
            Serial.println("[BTN45] Menu principal");
            enviarMenu();
        }
    }

    // ════════════════════════════════════════════════════════
    //  BOTÃO 4 — WEB INFO (pino 46)
    //  Exibe IP e acesso ao painel web no OLED por 30s.
    //  Útil para saber o endereço sem precisar do Serial.
    //  Debounce de 500ms.
    // ════════════════════════════════════════════════════════
    {
        static unsigned long tBotaoWeb = 0;
        static unsigned long ultimoCliqueWeb = 0;
        static bool stableState = false, lastRawState = false;
        static unsigned long lastChange = 0;
        if(botaoPressionadoDebounced(BTN_WEB, stableState, lastRawState, lastChange, 45) && millis() - tBotaoWeb > 300){
            tBotaoWeb = millis();
            bool duploClique = (ultimoCliqueWeb > 0 && (millis() - ultimoCliqueWeb) <= 1200UL);
            ultimoCliqueWeb = millis();
            if(duploClique){
                if(!apConfigAtivo()){
                    iniciarModoConfigAP();
                    Serial.println("[BTN46] Duplo clique: AP de configuração ativado");
                } else {
                    pararModoConfigAP();
                    Serial.println("[BTN46] Duplo clique: AP de configuração desativado");
                }
            } else {
                Serial.println("[BTN46] Web info OLED");
            }
            agendaVisivel  = false;   // cancela agenda se estava ativa
            webInfoVisivel = true;
            webInfoTimer   = millis();
            if(apConfigAtivo()){
                display.clearDisplay();
                display.fillRect(0, 0, 128, 12, SH110X_WHITE);
                display.setTextColor(SH110X_BLACK);
                display.setCursor(14, 2);
                display.setTextSize(1);
                display.print("WIFI CONFIG AP");
                display.setTextColor(SH110X_WHITE);
                display.setCursor(2, 18);
                display.print(nomeAPConfig());
                display.setCursor(2, 30);
                display.print("Senha: aurora123");
                display.setCursor(2, 42);
                display.print("http://192.168.4.1");
                display.display();
            } else if(WiFi.status() == WL_CONNECTED){
                exibirAcessoWebOLED();
            } else {
                // Mostra aviso de offline
                display.clearDisplay();
                display.fillRect(0, 0, 128, 12, SH110X_WHITE);
                display.setTextColor(SH110X_BLACK);
                display.setCursor(28, 2); display.setTextSize(1);
                display.print("PAINEL WEB");
                display.setTextColor(SH110X_WHITE);
                display.setCursor(14, 28);
                display.print("WiFi desconectado");
                display.display();
            }
        }
        // Refresca a tela web info a cada 5s enquanto visível
        // (o IP não muda, mas muda o estado da sessão)
        static unsigned long _lastWebRefresh = 0;
        if(webInfoVisivel && millis() - _lastWebRefresh > 5000){
            _lastWebRefresh = millis();
            if(apConfigAtivo()){
                display.clearDisplay();
                display.fillRect(0, 0, 128, 12, SH110X_WHITE);
                display.setTextColor(SH110X_BLACK);
                display.setCursor(14, 2);
                display.setTextSize(1);
                display.print("WIFI CONFIG AP");
                display.setTextColor(SH110X_WHITE);
                display.setCursor(2, 18);
                display.print(nomeAPConfig());
                display.setCursor(2, 30);
                display.print("Senha: aurora123");
                display.setCursor(2, 42);
                display.print("http://192.168.4.1");
                display.display();
            } else if(WiFi.status() == WL_CONNECTED) exibirAcessoWebOLED();
        }
    }

    delay(1);  // yield mínimo para watchdog
}
