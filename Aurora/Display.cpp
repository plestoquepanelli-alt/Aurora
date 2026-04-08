#include "Display.h"
#include "Sistema.h"
#include "freertos/semphr.h"
#include "GeminiTypes.h"
extern bool oledLigado;
#include "Config.h"
#include "Clima.h"
#include "AlertaINMET.h"
#include <WiFi.h>
#include <Wire.h>

Adafruit_SH1106G display = Adafruit_SH1106G(128, 64, &Wire, -1);
static unsigned long lastFrame = 0;

// Ícones PROGMEM — dados estáticos em flash
const uint8_t icon_sol[] PROGMEM = {
    0x06,0x00, 0x06,0x00, 0x00,0x00,
    0x0F,0x80, 0x3F,0xF0, 0x7F,0xF8,
    0xFF,0xFE, 0xFF,0xFE,
    0x7F,0xF8, 0x3F,0xF0, 0x0F,0x80,
    0x00,0x00, 0x06,0x00, 0x06,0x00,
    0x00,0x00, 0x00,0x00
};
const uint8_t icon_nuvem[] PROGMEM = {
    0x00,0x00, 0x00,0x00,
    0x07,0x00, 0x0F,0x80, 0x1F,0xC0,
    0x7F,0xF8, 0xFF,0xFC, 0xFF,0xFE,
    0xFF,0xFE, 0xFF,0xFE, 0x7F,0xFC,
    0x3F,0xF8, 0x00,0x00, 0x00,0x00,
    0x00,0x00, 0x00,0x00
};
const uint8_t icon_chuva[] PROGMEM = {
    0x00,0x00, 0x07,0x00,
    0x1F,0xC0, 0x7F,0xF0, 0xFF,0xF8,
    0xFF,0xF8, 0x7F,0xF0, 0x1F,0xC0,
    0x00,0x00, 0x24,0x90,
    0x00,0x00, 0x12,0x48,
    0x00,0x00, 0x24,0x90,
    0x00,0x00, 0x00,0x00
};
const uint8_t icon_storm[] PROGMEM = {
    0x00,0x00, 0x07,0x00,
    0x1F,0xC0, 0x7F,0xF0, 0xFF,0xF8,
    0xFF,0xF8, 0x7F,0xF0, 0x1F,0xC0,
    0x03,0xC0, 0x01,0x80,
    0x03,0xC0, 0x01,0x80,
    0x03,0x00, 0x01,0x00,
    0x00,0x00, 0x00,0x00
};

void initDisplay() {
    Wire.begin(SDA_PIN, SCL_PIN);
    display.begin(0x3C, true);
    display.clearDisplay();
    display.setTextColor(SH110X_WHITE);
    display.setTextSize(1);
    display.display();
}

// ════════════════════════════════════════════════════════════
//  desenharIcone — cache lowercase (climaDescricao muda a cada 5min)
// ════════════════════════════════════════════════════════════
static void desenharIcone(int x, int y) {
    static String       dCached;
    static unsigned long cacheTs = 0;
    if (millis() - cacheTs > 5000 || dCached.isEmpty()) {
        dCached = climaDescricao; dCached.toLowerCase(); cacheTs = millis();
    }
    const uint8_t *ico =
        (dCached.indexOf("tempest") >= 0 || dCached.indexOf("thunder") >= 0 || dCached.indexOf("storm") >= 0)
            ? icon_storm :
        (dCached.indexOf("chuva")   >= 0 || dCached.indexOf("rain")    >= 0 ||
         dCached.indexOf("garoa")   >= 0 || dCached.indexOf("drizzle")  >= 0)
            ? icon_chuva :
        (dCached.indexOf("nuv")     >= 0 || dCached.indexOf("cloud")   >= 0 ||
         dCached.indexOf("parcial") >= 0 || dCached.indexOf("encob")    >= 0)
            ? icon_nuvem
            : icon_sol;
    display.drawBitmap(x, y, ico, 16, 16, SH110X_WHITE);
}

static void renderHeader(const char *hora, const char *ddmm) {
    display.fillRect(0, 0, 128, 12, SH110X_WHITE);
    display.setTextColor(SH110X_BLACK);
    display.setTextSize(1);

    // Barras WiFi
    {
        int rssi = WiFi.RSSI(), bars = 0;
        if (WiFi.status() == WL_CONNECTED) {
            if      (rssi > -55) bars = 4;
            else if (rssi > -65) bars = 3;
            else if (rssi > -75) bars = 2;
            else                 bars = 1;
        }
        const int bx[4] = {2, 6, 10, 14};
        for (int b = 0; b < 4; b++) {
            int bh = 3 + b * 2, by = 10 - bh;
            if (b < bars) display.fillRect(bx[b], by, 3, bh, SH110X_BLACK);
            else          display.drawRect(bx[b], by, 3, bh, SH110X_BLACK);
        }
    }

    display.setCursor(44, 2);
    display.print((gJob.state == GJOB_RUNNING || gJob.state == GJOB_PENDING) ? "AI..  " : "AURORA");

    {
        static bool          _showHora = true;
        static unsigned long _lastT    = 0;
        if (millis() - _lastT > 3000) { _showHora = !_showHora; _lastT = millis(); }
        display.setCursor(95, 2);
        display.print(_showHora ? hora : ddmm);
    }
    display.setTextColor(SH110X_WHITE);
}

static void renderRodape() {
    display.setTextColor(SH110X_WHITE);
    display.drawFastHLine(0, 54, 128, SH110X_WHITE);
    display.setCursor(2, 56);
    { float tc = lerTemperatura(); display.printf("C:%dC", isnan(tc) ? 0 : (int)round(tc)); }
    display.setCursor(91, 56);
    display.printf("R:%d%%", heapPercentual());
}

static void renderAgora() {
    display.setTextColor(SH110X_WHITE);
    desenharIcone(2, 14);
    display.setTextSize(2);
    display.setCursor(23, 26);
    int tInt = (int)round(climaTemp);
    display.print(tInt);
    int nd = (abs(tInt) >= 10) ? 2 : 1; if (tInt < 0) nd++;
    int xGrau = 24 + nd * 12;
    display.setTextSize(1);
    display.setCursor(xGrau, 26); display.print("o");
    display.setCursor(xGrau, 34); display.print("C");
    display.setCursor(2, 43);     display.printf("HUM: %d%%", climaUmidade);
    display.drawFastVLine(70, 13, 41, SH110X_WHITE);
    int xr = 73;
    display.setCursor(xr, 15); display.printf("ST:%dC",  (int)round(climaSensTermica));
    display.setCursor(xr, 24); display.printf("VT:%dkm", (int)round(climaVento));
    display.setCursor(xr, 33); display.printf("PR:%d",   climaPressao);
    display.setCursor(xr, 42); display.printf("CH:%d%%", (int)round(chuva3h));
}

static void renderForecast() {
    display.setTextColor(SH110X_WHITE);
    display.setCursor(2,  13); display.print("HORA");
    display.setCursor(29, 13); display.print("TEMP");
    display.setCursor(58, 13); display.print("CHUVA");

    const int  LY[3] = {23, 33, 43};
    String     hs[3] = {hora_prev3h, hora_prev6h, hora_prev9h};
    float      vs[3] = {previsao3h,  previsao6h,  previsao9h};
    float      rs[3] = {chuva3h,     chuva6h,     chuva9h};

    for (int i = 0; i < 3; i++) {
        display.setCursor(2,   LY[i]); display.print(hs[i]);
        display.setCursor(29,  LY[i]); display.printf("%dC", (int)round(vs[i]));
        int bw = constrain((int)(44.0f * rs[i] / 100.0f), 0, 44);
        display.drawRect(54, LY[i], 44, 7, SH110X_WHITE);
        if (bw > 0) display.fillRect(54, LY[i], bw, 7, SH110X_WHITE);
        display.setCursor(100, LY[i]); display.printf("%d%%", (int)round(rs[i]));
    }
}

void exibirAgendaOLED(const String &eventos, int diaAtual) {
    display.clearDisplay();
    struct tm ti;
    char hora[6] = "--:--", ddmm[6] = "--/--";
    if (getLocalTime(&ti)) {
        strftime(hora, sizeof(hora), "%H:%M", &ti);
        strftime(ddmm, sizeof(ddmm), "%d/%m", &ti);
    }
    renderHeader(hora, ddmm);

    display.setTextColor(SH110X_WHITE);
    display.setCursor(30, 14); display.printf("DIA %d", diaAtual);
    display.drawFastHLine(0, 22, 128, SH110X_WHITE);

    int y = 25, pos = 0, count = 0;
    if (eventos.isEmpty() || eventos == "Agenda vazia.") {
        display.setCursor(22, 35); display.print("SEM EVENTOS HOJE");
    } else {
        while (pos < (int)eventos.length() && count < 3) {
            int nl = eventos.indexOf('\n', pos);
            if (nl < 0) nl = eventos.length();
            String l = eventos.substring(pos, nl); l.trim(); pos = nl + 1;
            if (l.isEmpty()) continue;
            if ((int)l.length() > 20) l = l.substring(0, 19) + ">";
            display.setCursor(4, y); display.printf("- %s", l.c_str());
            y += 11; count++;
        }
        if (count >= 3 && pos < (int)eventos.length()) {
            display.setCursor(88, 47); display.print("+ mais");
        }
    }
    renderRodape();
    display.display();
}

// ════════════════════════════════════════════════════════════
//  atualizarDisplay
//
//  FIX [7]: Intervalo 100ms → 250ms (4 FPS — dados mudam a cada
//  minuto, 10 FPS era desperdício de I2C e CPU).
//  Renderização diferencial: só redesenha se algum dado mudou.
//  Economia: ~60% menos transferências I2C, mais tempo de CPU
//  disponível para WebServer, botões e Telegram.
// ════════════════════════════════════════════════════════════
void atualizarDisplay() {
    if (millis() - lastFrame < 250) return;   // [7] 250ms = 4 FPS
    lastFrame = millis();

    // ── Lê valores atuais ────────────────────────────────────
    struct tm ti;
    char hora[6] = "--:--", ddmm[6] = "--/--";
    if (getLocalTime(&ti)) {
        strftime(hora, sizeof(hora), "%H:%M", &ti);
        strftime(ddmm, sizeof(ddmm), "%d/%m", &ti);
    }

    // Alterna tela: 15s clima atual → 5s previsão → repete
    static unsigned long tIni     = 0;
    static bool          forecast = false;
    if (millis() - tIni >= (unsigned long)(forecast ? 5000 : 15000)) {
        forecast = !forecast; tIni = millis();
    }

    // ── Renderização diferencial ─────────────────────────────
    // Compara snapshot dos dados com o frame anterior.
    // Só redesenha (e transfere I2C) se algo mudou.
    static float  _pTemp = -999, _pSens = -999, _pVento = -999;
    static int    _pUmi  = -1,   _pPres = -1;
    static String _pDesc = "";
    static float  _pChuva3 = -1, _pPrev3 = -999;
    static int    _pHeap  = -1;
    static bool   _pForecast = false;
    static GJobState _pJobState = GJOB_IDLE;
    static char   _pHora[6]  = "", _pDdmm[6] = "";
    static int    _pBars = -1;
    static float  _pChipTemp = -999;

    // Calcula barras WiFi atuais
    int rssi = WiFi.RSSI(), bars = 0;
    if (WiFi.status() == WL_CONNECTED) {
        if      (rssi > -55) bars = 4;
        else if (rssi > -65) bars = 3;
        else if (rssi > -75) bars = 2;
        else                 bars = 1;
    }
    float chipTemp = lerTemperatura();
    int   hp       = heapPercentual();

    bool dirty =
        forecast      != _pForecast    ||
        gJob.state    != _pJobState    ||
        strcmp(hora,  _pHora)  != 0    ||
        strcmp(ddmm,  _pDdmm)  != 0    ||
        bars          != _pBars        ||
        hp            != _pHeap        ||
        (int)round(chipTemp) != (int)round(_pChipTemp);

    if (!forecast) {
        dirty = dirty ||
            fabsf(climaTemp        - _pTemp)  > 0.09f  ||
            fabsf(climaSensTermica - _pSens)  > 0.09f  ||
            fabsf(climaVento       - _pVento) > 0.09f  ||
            climaUmidade  != _pUmi            ||
            climaPressao  != _pPres           ||
            climaDescricao != _pDesc          ||
            fabsf(chuva3h  - _pChuva3) > 0.09f;
    } else {
        dirty = dirty ||
            fabsf(previsao3h - _pPrev3) > 0.09f;
    }

    if (!dirty) return;   // nada mudou — não gasta I2C

    // Atualiza snapshot
    _pForecast  = forecast;
    _pJobState  = gJob.state;
    strncpy(_pHora,  hora,  6);
    strncpy(_pDdmm,  ddmm,  6);
    _pBars      = bars;
    _pHeap      = hp;
    _pChipTemp  = chipTemp;
    _pTemp      = climaTemp;
    _pSens      = climaSensTermica;
    _pVento     = climaVento;
    _pUmi       = climaUmidade;
    _pPres      = climaPressao;
    _pDesc      = climaDescricao;
    _pChuva3    = chuva3h;
    _pPrev3     = previsao3h;

    display.clearDisplay();
    renderHeader(hora, ddmm);
    if (forecast) renderForecast();
    else          renderAgora();
    renderRodape();
    display.display();
}
