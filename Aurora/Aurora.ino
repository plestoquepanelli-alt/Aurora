/*
 * ════════════════════════════════════════════════════════════
 *         AURORA ESP32-S3 · AI Personal Assistant
 *         v3.0 — Pedro Panelli
 * ════════════════════════════════════════════════════════════
 *
 *  BOARD SETTINGS:
 *  Board:      ESP32S3 Dev Module
 *  Partition:  Huge APP (3MB No OTA/1MB SPIFFS)
 *  PSRAM:      OPI PSRAM
 *  CPU:        240 MHz
 *  USB CDC:    Enabled
 *
 *  BOTÕES (todos INPUT_PULLUP → LOW = pressionado):
 *  Pino 14 — Agenda:   mostra compromissos do dia no OLED (30s)
 *  Pino  3 — Display:  toggle liga/desliga do OLED
 *  Pino 45 — Menu:     abre o menu principal do Telegram
 *  Pino 46 — Web Info: exibe IP/acesso ao painel web no OLED (30s)
 *
 *  ARQUITETURA DUAL-CORE:
 *  Core 1 (loop): display, LED, botões, clima, Telegram polling,
 *                 WebServer, alertas INMET
 *  Core 0 (taskGemini): HTTP ao Gemini — nunca bloqueia o Core 1
 *
 *  FUNÇÕES BLOQUEANTES CORRIGIDAS:
 *  - atualizarClima()      → agora com watchdog yield interno
 *  - verificarAlertaINMET()→ timeout protegido + client static
 *  - sanitizarMarkdown()   → loop O(n²) substituído por O(n)
 *  - delay(400) no envio   → yield() em vez de delay blocking
 *  - cmd_scan I2C          → timeout por dispositivo
 *  - loop() delay(1)       → mantido para watchdog, todo o resto
 *                             usa millis() corretamente
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
#define BTN_AGENDA   14   // mostra agenda do dia no OLED
#define BTN_DISPLAY   3   // toggle OLED liga/desliga
#define BTN_MENU     45   // abre menu principal do Telegram
#define BTN_WEB      46   // exibe IP de acesso ao painel web no OLED

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
#include "WebAurora.h"   // ← Painel Web (renomeado para evitar conflito com biblioteca)

// ── Variáveis globais ─────────────────────────────────────────
String modeloAtivo    = "gemini-2.5-flash";
String cidade         = "Muriae,BR";
unsigned long perguntasFeitas = 0;
unsigned long bootTime        = 0;

// ── Estado do OLED ────────────────────────────────────────────
bool oledLigado = true;

// ── Clientes HTTPS ────────────────────────────────────────────
WiFiClientSecure secured_client;
WiFiClientSecure gemini_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);

// ════════════════════════════════════════════════════════════
//  GEMINI ASYNC — FreeRTOS dual-core
//
//  GJobState e GeminiJob definidos em GeminiTypes.h (fonte única).
//  gJob e gMutex são definidos aqui (Aurora.ino) e acessados via
//  extern em Display.cpp, TelegramHandler.cpp e WebAurora.cpp.
// ════════════════════════════════════════════════════════════

// Definições reais dos objetos globais (extern em GeminiTypes.h)
GeminiJob          gJob;
SemaphoreHandle_t  gMutex;

static void taskGemini(void* pv){
    for(;;){
        if(gJob.state == GJOB_PENDING){
            gJob.state = GJOB_RUNNING;
            Serial.printf("[Core0] Gemini start heap=%d\n", ESP.getFreeHeap());
            String resp = perguntarGemini(gJob.pergunta);
            if(xSemaphoreTake(gMutex, pdMS_TO_TICKS(2000)) == pdTRUE){
                gJob.resposta = resp;
                gJob.state    = GJOB_DONE;
                xSemaphoreGive(gMutex);
                Serial.printf("[Core0] Gemini done %dms\n", (int)(millis()-gJob.ts));
            }
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// ════════════════════════════════════════════════════════════
//  SETUP
// ════════════════════════════════════════════════════════════
void setup(){
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
    while(WiFi.status() != WL_CONNECTED && millis() - wt < 15000){
        updateLED(); delay(200); Serial.print(".");
    }
    Serial.println();

    if(WiFi.status() == WL_CONNECTED){
        Serial.println("WiFi OK: " + WiFi.localIP().toString());
        sincronizarHorario();
        delay(800);
        atualizarClima();
        ultimoClima = millis();
        initWebServer();    // ← Inicia painel web após WiFi OK
    } else {
        Serial.println("WiFi falhou — offline");
    }

    initTemperatura();
    sdOK = initSD();
    if(!sdOK) Serial.println("SD nao disponivel");

    { int n = bot.getUpdates(0);
      if(n > 0) bot.last_message_received = bot.messages[n-1].update_id; }

    // ── Configura botões ─────────────────────────────────────
    pinMode(BTN_AGENDA,  INPUT_PULLUP);
    pinMode(BTN_DISPLAY, INPUT_PULLUP);
    pinMode(BTN_MENU,    INPUT_PULLUP);
    pinMode(BTN_WEB,     INPUT_PULLUP);

    initDisplay();

    // ── FreeRTOS: mutex + task Gemini no Core 0 ──────────────
    gMutex = xSemaphoreCreateMutex();
    xTaskCreatePinnedToCore(taskGemini, "GeminiTask", 16384, NULL, 1, NULL, 0);
    Serial.println("GeminiTask → Core 0");

    enviarMenu();
}

// ════════════════════════════════════════════════════════════
//  LOOP — Core 1
//  Nunca bloqueia por Gemini (está no Core 0).
//  Todo o código aqui usa millis() para temporização.
// ════════════════════════════════════════════════════════════
void loop(){

    // ── Lê hora uma única vez por ciclo ─────────────────────
    struct tm _lt;
    bool  _ok      = getLocalTime(&_lt);
    int   _hora    = _ok ? _lt.tm_hour : 12;
    int   _diaHoje = _ok ? _lt.tm_mday : 0;
    bool  _noturno = isModoNoturnoAgora();

    // ── Serviços periódicos ───────────────────────────────────
    loopOTA();
    checkWiFi();
    updateLED();
    verificarRespostaGemini();
    handleWebServer();     // ← WebServer: processa requests pendentes

    // ── Log sensor a cada 5min ────────────────────────────────
    static unsigned long lastSensorLog = 0;
    if(millis() - lastSensorLog > 300000UL){
        lastSensorLog = millis();
        sdLogSensor(lerTemperatura(), ESP.getFreeHeap()/1024, heapPercentual());
    }

    // ── Display ───────────────────────────────────────────────
    // Estados de prioridade (maior → menor):
    //  1. webInfo visível    — mostra URL do painel web
    //  2. Desligado manual   — apaga 1x e para
    //  3. Modo noturno       — apaga automaticamente
    //  4. Agenda visível     — mantém agenda, não sobrescreve
    //  5. Normal             — atualiza a cada 100ms (em Display.cpp)
    static unsigned long lastDisplay     = 0;
    static bool          agendaVisivel   = false;
    static bool          webInfoVisivel  = false;
    static unsigned long webInfoTimer    = 0;

    // Controla exibição da tela web info por 30s
    if(webInfoVisivel && millis() - webInfoTimer > 30000UL){
        webInfoVisivel = false;
        lastDisplay = 0; // força retorno imediato ao display normal
    }

    if(!oledLigado){
        static bool _apagouManual = false;
        if(!_apagouManual){ display.clearDisplay(); display.display(); _apagouManual = true; }
    } else {
        static bool _apagouManual = false; _apagouManual = false;
        if(webInfoVisivel){
            // WebInfo: só atualiza na entrada (já foi desenhado pelo botão)
        } else if(_noturno){
            static bool _apagouNoturno = false;
            if(!_apagouNoturno){ display.clearDisplay(); display.display(); _apagouNoturno = true; }
        } else {
            static bool _apagouNoturno = false; _apagouNoturno = false;
            if(!agendaVisivel && millis() - lastDisplay > 100){
                atualizarDisplay();
                lastDisplay = millis();
            }
        }
    }

    // ── Clima + INMET ─────────────────────────────────────────
    // Reduz frequência quando painel web está com sessão ativa
    // (evita contenção de CPU/heap entre WebServer e HTTPClient)
    unsigned long intervaloClima = webSessaoAtiva
        ? (INTERVALO_CLIMA * 2)   // 10min com sessão web ativa
        : INTERVALO_CLIMA;        // 5min normal

    if(millis() - ultimoClima > intervaloClima){
        atualizarClima();
        verificarAlertaINMET();
        ultimoClima = millis();
    }

    // ── Telegram — só em modo diurno ─────────────────────────
    // Se painel web está logado, reduz polling do Telegram
    // para liberar CPU (ambos fazem HTTP na mesma stack)
    if(!_noturno){
        verificarLembretes();
        if(!webSessaoAtiva){
            processarMensagens();
        } else {
            // Com sessão web ativa: polling mais lento (a cada 10s)
            static unsigned long _lastTgWeb = 0;
            if(millis() - _lastTgWeb > 10000UL){
                _lastTgWeb = millis();
                processarMensagens();
            }
        }
    }

    // ── Debounce robusto por estado estável (LOW = pressionado) ─
    auto botaoPressionado = [&](uint8_t pin, bool &stableState, bool &lastRawState, unsigned long &lastChangeMs, unsigned long debounceMs)->bool {
        bool rawPressed = (digitalRead(pin) == LOW);
        if(rawPressed != lastRawState){
            lastRawState = rawPressed;
            lastChangeMs = millis();
        }
        if((millis() - lastChangeMs) >= debounceMs && rawPressed != stableState){
            stableState = rawPressed;
            if(stableState) return true; // evento só na borda de descida
        }
        return false;
    };

    // ════════════════════════════════════════════════════════
    //  BOTÃO 1 — AGENDA (pino 14)
    // ════════════════════════════════════════════════════════
    {
        static unsigned long tBotao = 0, tExib = 0;
        static bool stableState = false, lastRawState = false;
        static unsigned long lastChange = 0;
        if(botaoPressionado(BTN_AGENDA, stableState, lastRawState, lastChange, 45) && millis() - tBotao > 300){
            tBotao = tExib = millis();
            agendaVisivel  = true;
            webInfoVisivel = false;   // cancela web info se estava ativa
            String todaAgenda = lerAgenda();
            String eventosDia = "";
            if(_diaHoje > 0){
                int pos = 0;
                while(pos < (int)todaAgenda.length()){
                    int nl = todaAgenda.indexOf('\n', pos);
                    if(nl < 0) nl = todaAgenda.length();
                    String l = todaAgenda.substring(pos, nl); pos = nl+1;
                    int dash = l.indexOf(" - ");
                    if(dash > 0 && l.substring(0, dash).toInt() == _diaHoje)
                        eventosDia += l.substring(dash+3) + "\n";
                }
            }
            exibirAgendaOLED(eventosDia, _diaHoje);
        }
        lastState = pressed;
        if(agendaVisivel && millis() - tExib > 30000UL)
            agendaVisivel = false;
    }

    // ════════════════════════════════════════════════════════
    //  BOTÃO 2 — DISPLAY (pino 3)
    // ════════════════════════════════════════════════════════
    {
        static unsigned long tBotaoDisp = 0;
        static bool stableState = false, lastRawState = false;
        static unsigned long lastChange = 0;
        if(botaoPressionado(BTN_DISPLAY, stableState, lastRawState, lastChange, 45) && millis() - tBotaoDisp > 300){
            tBotaoDisp = millis();
            oledLigado = !oledLigado;
            Serial.printf("[BTN3] OLED %s\n", oledLigado ? "ligado" : "desligado");
            if(oledLigado) lastDisplay = 0;
        }
        lastState = pressed;
    }

    // ════════════════════════════════════════════════════════
    //  BOTÃO 3 — MENU TELEGRAM (pino 45)
    // ════════════════════════════════════════════════════════
    {
        static unsigned long tBotaoMenu = 0;
        static bool stableState = false, lastRawState = false;
        static unsigned long lastChange = 0;
        if(botaoPressionado(BTN_MENU, stableState, lastRawState, lastChange, 50) && millis() - tBotaoMenu > 400){
            tBotaoMenu = millis();
            Serial.println("[BTN45] Menu principal");
            enviarMenu();
        }
        lastState = pressed;
    }

    // ════════════════════════════════════════════════════════
    //  BOTÃO 4 — WEB INFO (pino 46)
    //  Exibe IP e acesso ao painel web no OLED por 30s.
    //  Útil para saber o endereço sem precisar do Serial.
    //  Debounce de 500ms.
    // ════════════════════════════════════════════════════════
    {
        static unsigned long tBotaoWeb = 0;
        static bool stableState = false, lastRawState = false;
        static unsigned long lastChange = 0;
        if(botaoPressionado(BTN_WEB, stableState, lastRawState, lastChange, 45) && millis() - tBotaoWeb > 300){
            tBotaoWeb = millis();
            Serial.println("[BTN46] Web info OLED");
            agendaVisivel  = false;   // cancela agenda se estava ativa
            webInfoVisivel = true;
            webInfoTimer   = millis();
            if(WiFi.status() == WL_CONNECTED){
                exibirAcessoWebOLED();
            } else {
                // Mostra aviso de offline
                display.clearDisplay();
                display.fillRect(0, 0, 128, 12, SH110X_WHITE);
                display.setTextColor(SH110X_BLACK);
                display.setCursor(28, 2);
                display.setTextSize(1);
                display.print("PAINEL WEB");
                display.setTextColor(SH110X_WHITE);
                display.setCursor(14, 28);
                display.print("WiFi desconectado");
                display.display();
            }
        }
        lastState = pressed;
        // Refresca a tela web info a cada 5s enquanto visível
        // (o IP não muda, mas muda o estado da sessão)
        static unsigned long _lastWebRefresh = 0;
        if(webInfoVisivel && millis() - _lastWebRefresh > 5000){
            _lastWebRefresh = millis();
            if(WiFi.status() == WL_CONNECTED) exibirAcessoWebOLED();
        }
    }

    // Yield mínimo para watchdog do ESP32
    delay(1);
}
