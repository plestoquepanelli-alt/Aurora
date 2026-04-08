#include "Clima.h"
#include "Config.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <UniversalTelegramBot.h>
#include "AlertaINMET.h"

// ================= VARIÁVEIS =================
float  climaTemp        = 0;
int    climaUmidade     = 0;
String climaDescricao   = "Sem dados";
bool   alertaClimaEnviado = false;
unsigned long ultimoClima = 0;
const unsigned long INTERVALO_CLIMA = 300000UL; // 5 min

float previsao3h = 0, previsao6h = 0, previsao9h = 0;
float chuva3h    = 0, chuva6h    = 0, chuva9h    = 0;

String hora_prev3h = "+3h";
String hora_prev6h = "+6h";
String hora_prev9h = "+9h";

float climaSensTermica = 0;
int   climaPressao     = 0;
float climaVento       = 0;

extern UniversalTelegramBot bot;

// ════════════════════════════════════════════════════════════
//  FIX LENTIDÃO [2]: Ciclos alternados — actual a cada 5min,
//  forecast a cada 10min. Corta o bloqueio no Core 1 à metade
//  em cada ciclo de atualização.
// ════════════════════════════════════════════════════════════
static void _atualizarAtual() {
    HTTPClient http;
    http.setTimeout(HTTP_TIMEOUT);                       // [1] 6000ms
    String url = "http://api.openweathermap.org/data/2.5/weather?q=" + cidade
               + "&units=metric&appid=" + WEATHER_API_KEY + "&lang=pt_br";

    if (!http.begin(url)) {
        Serial.println("[Clima] begin atual falhou");
        http.end(); return;
    }

    int code = http.GET();
    yield();

    if (code == 200) {
        StaticJsonDocument<96> filtro;
        filtro["main"]["temp"]       = true;
        filtro["main"]["humidity"]   = true;
        filtro["main"]["feels_like"] = true;
        filtro["main"]["pressure"]   = true;
        filtro["weather"][0]["description"] = true;
        filtro["wind"]["speed"]      = true;

        DynamicJsonDocument doc(1024);
        if (!deserializeJson(doc, http.getStream(), DeserializationOption::Filter(filtro))) {
            climaTemp        = doc["main"]["temp"]       | 0.0f;
            climaUmidade     = doc["main"]["humidity"]   | 0;
            climaSensTermica = doc["main"]["feels_like"] | 0.0f;
            climaPressao     = doc["main"]["pressure"]   | 0;
            climaVento       = (doc["wind"]["speed"]     | 0.0f) * 3.6f;
            climaDescricao   = doc["weather"][0]["description"].as<String>();
            Serial.println("[Clima] atual OK: " + climaDescricao);
        } else {
            Serial.println("[Clima] Erro JSON atual");
        }
    } else {
        Serial.printf("[Clima] HTTP atual: %d\n", code);
    }
    http.end();
}

static void _atualizarForecast() {
    HTTPClient http;
    http.setTimeout(HTTP_TIMEOUT);                       // [1] 6000ms
    String url = "http://api.openweathermap.org/data/2.5/forecast?q=" + cidade
               + "&units=metric&appid=" + WEATHER_API_KEY + "&lang=pt_br";

    if (!http.begin(url)) {
        Serial.println("[Clima] begin forecast falhou");
        http.end(); return;
    }

    int code = http.GET();
    yield();

    if (code == 200) {
        StaticJsonDocument<128> filtro;
        filtro["list"][0]["main"]["temp"] = true;
        filtro["list"][0]["pop"]          = true;
        filtro["list"][0]["dt_txt"]       = true;

        DynamicJsonDocument doc(2048);
        DeserializationError err = deserializeJson(doc, http.getStream(),
            DeserializationOption::Filter(filtro));

        if (!err) {
            JsonArray lista = doc["list"];
            if (lista.size() >= 3) {
                previsao3h = lista[0]["main"]["temp"] | 0.0f;
                previsao6h = lista[1]["main"]["temp"] | 0.0f;
                previsao9h = lista[2]["main"]["temp"] | 0.0f;
                chuva3h    = (lista[0]["pop"] | 0.0f) * 100.0f;
                chuva6h    = (lista[1]["pop"] | 0.0f) * 100.0f;
                chuva9h    = (lista[2]["pop"] | 0.0f) * 100.0f;

                auto extrairHora = [](JsonVariant v, const String &fb) -> String {
                    const char *dt = v["dt_txt"] | "";
                    if (strlen(dt) >= 13) {
                        char buf[4]; buf[0]=dt[11]; buf[1]=dt[12]; buf[2]='h'; buf[3]=0;
                        return String(buf);
                    }
                    return fb;
                };
                hora_prev3h = extrairHora(lista[0], "+3h");
                hora_prev6h = extrairHora(lista[1], "+6h");
                hora_prev9h = extrairHora(lista[2], "+9h");
                Serial.println("[Clima] forecast OK");
            }
        } else {
            Serial.println("[Clima] Erro JSON forecast");
        }
    } else {
        Serial.printf("[Clima] HTTP forecast: %d\n", code);
    }
    http.end();
}

// ════════════════════════════════════════════════════════════
//  atualizarClima — ciclos alternados
//
//  Ciclos pares  (0,2,4…): atual + forecast  (2 requests)
//  Ciclos ímpares (1,3,5…): apenas atual     (1 request)
//
//  Efeito: a cada 5min faz no máximo 1 request bloqueante;
//  a cada 10min faz 2 (igual ao comportamento anterior mas
//  distribuído no tempo, nunca 2 de uma vez).
// ════════════════════════════════════════════════════════════
void atualizarClima() {
    if (WiFi.status() != WL_CONNECTED) return;

    static uint8_t ciclo = 0;

    _atualizarAtual();
    yield();

    if (ciclo % 2 == 0) {          // forecast só nos ciclos pares
        _atualizarForecast();
    }
    ciclo++;

    verificarClimaSevero();
}

void verificarClimaSevero() {
    bool   climaSevero = false;
    String alerta      = "";

    if (climaTemp > 38) {
        climaSevero = true;
        alerta = "🔥 Calor extremo";
    }

    String d = climaDescricao;
    d.toLowerCase();

    if (d.indexOf("tempestade") >= 0 || d.indexOf("storm")  >= 0 || d.indexOf("thunder") >= 0) {
        climaSevero = true;
        alerta = "🌩 Tempestade detectada";
    }
    if (d.indexOf("chuva forte") >= 0 || d.indexOf("heavy rain") >= 0) {
        climaSevero = true;
        alerta = "🌧 Chuva forte";
    }

    if (climaSevero && !alertaClimaEnviado) {
        String msg;
        msg  = "⚠ *ALERTA CLIMÁTICO*\n";
        msg += "━━━━━━━━━━━━━━━\n\n";
        msg += alerta + "\n\n";
        msg += "🌡 " + String(climaTemp, 1) + " °C\n";
        msg += "💧 " + String(climaUmidade) + "%\n";
        msg += "☁ " + climaDescricao;
        bot.sendMessage(MEU_ID, msg, "Markdown");
        alertaClimaEnviado = true;
    }

    if (!climaSevero) alertaClimaEnviado = false;
}
