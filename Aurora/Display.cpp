#include "Display.h"
#include "Sistema.h"
#include "freertos/semphr.h"
// GJobState e GeminiJob definidos APENAS em GeminiTypes.h
// Remover a declaração local que causava "redefinition of GJobState"
#include "GeminiTypes.h"
// Estado do botão de display (definido em Aurora.ino)
extern bool oledLigado;
#include "Config.h"
#include "Clima.h"
#include "AlertaINMET.h"
#include <WiFi.h>
#include <Wire.h>

Adafruit_SH1106G display = Adafruit_SH1106G(128, 64, &Wire, -1);
static unsigned long lastFrame = 0;


// SOL: circulo raio 4 + raios cardeais 2px
const uint8_t icon_sol[] PROGMEM = {
    0x06,0x00, 0x06,0x00, 0x00,0x00,
    0x0F,0x80, 0x3F,0xF0, 0x7F,0xF8,
    0xFF,0xFE, 0xFF,0xFE,
    0x7F,0xF8, 0x3F,0xF0, 0x0F,0x80,
    0x00,0x00, 0x06,0x00, 0x06,0x00,
    0x00,0x00, 0x00,0x00
};
// NUVEM: massa arredondada com bump superior esquerdo
const uint8_t icon_nuvem[] PROGMEM = {
    0x00,0x00, 0x00,0x00,
    0x07,0x00, 0x0F,0x80, 0x1F,0xC0,
    0x7F,0xF8, 0xFF,0xFC, 0xFF,0xFE,
    0xFF,0xFE, 0xFF,0xFE, 0x7F,0xFC,
    0x3F,0xF8, 0x00,0x00, 0x00,0x00,
    0x00,0x00, 0x00,0x00
};
// CHUVA: nuvem compacta + gotas intercaladas
const uint8_t icon_chuva[] PROGMEM = {
    0x00,0x00, 0x07,0x00,
    0x1F,0xC0, 0x7F,0xF0, 0xFF,0xF8,
    0xFF,0xF8, 0x7F,0xF0, 0x1F,0xC0,
    0x00,0x00, 0x24,0x90,
    0x00,0x00, 0x12,0x48,
    0x00,0x00, 0x24,0x90,
    0x00,0x00, 0x00,0x00
};
// TEMPESTADE: nuvem + raio zigue-zague
const uint8_t icon_storm[] PROGMEM = {
    0x00,0x00, 0x07,0x00,
    0x1F,0xC0, 0x7F,0xF0, 0xFF,0xF8,
    0xFF,0xF8, 0x7F,0xF0, 0x1F,0xC0,
    0x03,0xC0, 0x01,0x80,
    0x03,0xC0, 0x01,0x80,
    0x03,0x00, 0x01,0x00,
    0x00,0x00, 0x00,0x00
};

// ═══════════════════════════════════════════════════════════════
void initDisplay(){
    Wire.begin(SDA_PIN, SCL_PIN);
    display.begin(0x3C, true);
    display.clearDisplay();
    display.setTextColor(SH110X_WHITE);
    display.setTextSize(1);
    display.display();
}

static void desenharIcone(int x, int y){
    const uint8_t* ico = icon_nuvem;
    String d = climaDescricao; d.toLowerCase();
    if(d.indexOf("tempest")>=0||d.indexOf("thunder")>=0||d.indexOf("storm")>=0)
        ico = icon_storm;
    else if(d.indexOf("chuva")>=0||d.indexOf("rain")>=0||
            d.indexOf("garoa")>=0||d.indexOf("drizzle")>=0)
        ico = icon_chuva;
    else if(d.indexOf("nuv")>=0||d.indexOf("cloud")>=0||
            d.indexOf("parcial")>=0||d.indexOf("encob")>=0)
        ico = icon_nuvem;
    else
        ico = icon_sol;
    display.drawBitmap(x, y, ico, 16, 16, SH110X_WHITE);
}

// ═══════════════════════════════════════════════════════════════
static void renderHeader(const char* hora, const char* ddmm){
    display.fillRect(0, 0, 128, 12, SH110X_WHITE);
    display.setTextColor(SH110X_BLACK);
    display.setTextSize(1);

    {
        int rssi = WiFi.RSSI(), bars = 0;
        if(WiFi.status() == WL_CONNECTED){
            if     (rssi > -55) bars = 4;
            else if(rssi > -65) bars = 3;
            else if(rssi > -75) bars = 2;
            else                bars = 1;
        }
        int bx[4] = {2, 6, 10, 14};
        for(int b = 0; b < 4; b++){
            int bh = 3 + b * 2;   // alturas: 3,5,7,9
            int by = 10 - bh;     // ancora no fundo do header
            if(b < bars)
                display.fillRect(bx[b], by, 3, bh, SH110X_BLACK);
            else
                display.drawRect(bx[b], by, 3, bh, SH110X_BLACK);
        }
    }

    // Centro: "AURORA" em idle, "AI..." enquanto Core 0 processa
    // Feedback visual imediato: o usuário sabe que a IA está pensando
    display.setCursor(44, 2);
    if(gJob.state == GJOB_RUNNING || gJob.state == GJOB_PENDING){
        display.print("AI..  ");  // pisca substituindo "AURORA"
    } else {
        display.print("AURORA");
    }

    // Direita: alterna hora e data a cada 3s
    {
        static bool          _showHora = true;
        static unsigned long _lastT    = 0;
        if(millis() - _lastT > 3000){ _showHora = !_showHora; _lastT = millis(); }
        display.setCursor(95, 2);
        display.print(_showHora ? hora : ddmm);
    }

    display.setTextColor(SH110X_WHITE);
}

// ═══════════════════════════════════════════════════════════════
static void renderRodape(){
    display.setTextColor(SH110X_WHITE);
    display.drawFastHLine(0, 54, 128, SH110X_WHITE);

    // SEG 1 — CPU temperatura (x=2-37)
    display.setCursor(2, 56);
    {
        float tc = lerTemperatura();
        display.printf("C:%dC", isnan(tc) ? 0 : (int)round(tc));
    }

    // SEG 3 — Heap livre % (x=91-127)
    display.setCursor(91, 56);
    display.printf("R:%d%%", heapPercentual());
}

// ═══════════════════════════════════════════════════════════════
static void renderAgora(){
    display.setTextColor(SH110X_WHITE);
    desenharIcone(2, 14);
    display.setTextSize(2);
    display.setCursor(23, 26);
    int tInt = (int)round(climaTemp);
    display.print(tInt);
    int nd  = (abs(tInt) >= 10) ? 2 : 1;
    if(tInt < 0) nd++;          // sinal negativo
    int xGrau = 24 + nd * 12;  // início do grau/unidade
    display.setTextSize(1);
    display.setCursor(xGrau, 26);
    display.print("o");
    display.setCursor(xGrau, 34);
    display.print("C");
    display.setCursor(2, 43);
    display.printf("HUM: %d%%", climaUmidade);
    // Divisor vertical
    display.drawFastVLine(70, 13, 41, SH110X_WHITE);
    int xr = 73;
    display.setCursor(xr, 15);
    display.printf("ST:%dC",  (int)round(climaSensTermica));
    display.setCursor(xr, 24);
    display.printf("VT:%dkm", (int)round(climaVento));
    display.setCursor(xr, 33);
    display.printf("PR:%d",   climaPressao);
    display.setCursor(xr, 42);
    display.printf("CH:%d%%", (int)round(chuva3h));
}
static void renderForecast(){
    display.setTextColor(SH110X_WHITE);

    // Cabeçalho das colunas
    display.setCursor(2,  13); display.print("HORA");
    display.setCursor(29, 13); display.print("TEMP");
    display.setCursor(58, 13); display.print("CHUVA");
    // Linha separadora abaixo do cabeçalho

    const int   LY[3] = {23, 33, 43};
    String      hs[3] = {hora_prev3h, hora_prev6h, hora_prev9h};
    float       vs[3] = {previsao3h,  previsao6h,  previsao9h};
    float       rs[3] = {chuva3h,     chuva6h,     chuva9h};

    for(int i = 0; i < 3; i++){
        int y = LY[i];

        // Hora real da API ("15h")
        display.setCursor(2, y);
        display.print(hs[i]);

        // Temperatura
        display.setCursor(29, y);
        display.printf("%dC", (int)round(vs[i]));

        // Barra de chuva proporcional (44px) + percentual
        int bw = constrain((int)(44.0f * rs[i] / 100.0f), 0, 44);
        display.drawRect(54, y, 44, 7, SH110X_WHITE);
        if(bw > 0) display.fillRect(54, y, bw, 7, SH110X_WHITE);
        display.setCursor(100, y);
        display.printf("%d%%", (int)round(rs[i]));
    }
}

// ═══════════════════════════════════════════════════════════════
//  AGENDA NO OLED — botão pino 14
// ═══════════════════════════════════════════════════════════════
void exibirAgendaOLED(const String& eventos, int diaAtual){
    display.clearDisplay();

    // Lê hora atual para passar ao header (corrige bug do renderHeader("",""))
    struct tm ti;
    char hora[6] = "--:--", ddmm[6] = "--/--";
    if(getLocalTime(&ti)){
        strftime(hora, sizeof(hora), "%H:%M", &ti);
        strftime(ddmm, sizeof(ddmm), "%d/%m", &ti);
    }
    renderHeader(hora, ddmm);

    display.setTextColor(SH110X_WHITE);
    display.setCursor(30, 14);
    display.printf("DIA %d", diaAtual);
    display.drawFastHLine(0, 22, 128, SH110X_WHITE);

    int y = 25, pos = 0, count = 0;
    if(eventos.isEmpty() || eventos == "Agenda vazia."){
        display.setCursor(22, 35);
        display.print("SEM EVENTOS HOJE");
    } else {
        while(pos < (int)eventos.length() && count < 3){
            int nl = eventos.indexOf('\n', pos);
            if(nl < 0) nl = eventos.length();
            String l = eventos.substring(pos, nl);
            l.trim(); pos = nl + 1;
            if(l.isEmpty()) continue;
            if((int)l.length() > 20) l = l.substring(0, 19) + ">";
            display.setCursor(4, y);
            display.printf("- %s", l.c_str());
            y += 11; count++;
        }
        if(count >= 3 && pos < (int)eventos.length()){
            display.setCursor(88, 47);
            display.print("+ mais");
        }
    }
    renderRodape();
    display.display();
}

// ═══════════════════════════════════════════════════════════════
//  PONTO DE ENTRADA — chamado pelo loop() a cada 100ms
// ═══════════════════════════════════════════════════════════════
void atualizarDisplay(){
    if(millis() - lastFrame < 100) return;
    lastFrame = millis();

    // Lê hora uma única vez por frame
    struct tm ti;
    char hora[6] = "--:--", ddmm[6] = "--/--";
    if(getLocalTime(&ti)){
        strftime(hora, sizeof(hora), "%H:%M", &ti);
        strftime(ddmm, sizeof(ddmm), "%d/%m", &ti);
    }

    // Alternância: tela 1 (15s) → tela 2 (5s) → repete
    static unsigned long tIni     = 0;
    static bool          forecast = false;
    if(millis() - tIni >= (unsigned long)(forecast ? 5000 : 15000)){
        forecast = !forecast;
        tIni = millis();
    }

    display.clearDisplay();
    renderHeader(hora, ddmm);
    if(forecast) renderForecast();
    else         renderAgora();
    renderRodape();
    display.display();
}
