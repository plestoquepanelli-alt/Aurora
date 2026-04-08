#include "AlertaINMET.h"
#include "Config.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <UniversalTelegramBot.h>
#include <WiFiClientSecure.h>

extern UniversalTelegramBot bot;

bool   alertaOficialAtivo = false;
String alertaOficialMsg   = "";

static String ultimoAlertaID  = "";
static bool   alertaJaEnviado = false;

// ════════════════════════════════════════════════════════════
//  verificarAlertaINMET
//
//  FIX [3]: clienteSeguro.setInsecure() chamado apenas 1x
//  (static + flag de inicialização).
//  Stream direto no deserializeJson — sem String intermediária.
// ════════════════════════════════════════════════════════════
void verificarAlertaINMET() {
    if (WiFi.status() != WL_CONNECTED) return;

    static WiFiClientSecure clienteSeguro;
    static bool inicializado = false;
    if (!inicializado) {
        clienteSeguro.setInsecure();
        inicializado = true;
    }

    HTTPClient http;
    http.setTimeout(5000);  // INMET raramente passa de 3s;

    if (!http.begin(clienteSeguro, "https://apiprevmet3.inmet.gov.br/avisos")) {
        Serial.println("[INMET] Erro begin");
        return;
    }

    int code = http.GET();
    yield();

    if (code <= 0) {
        Serial.printf("[INMET] Erro conexão: %d\n", code);
        http.end(); return;
    }
    if (code != 200) {
        Serial.printf("[INMET] HTTP %d\n", code);
        http.end(); return;
    }

    StaticJsonDocument<96> filtro;
    filtro[0]["ID"]         = true;
    filtro[0]["UF"]         = true;
    filtro[0]["MUNICIPIO"]  = true;
    filtro[0]["MUN"]        = true;
    filtro[0]["DS"]         = true;
    filtro[0]["SEVERIDADE"] = true;

    DynamicJsonDocument doc(4096);
    DeserializationError err = deserializeJson(doc, http.getStream(),
                                               DeserializationOption::Filter(filtro));
    http.end();

    if (err) {
        Serial.printf("[INMET] Erro JSON: %s\n", err.c_str());
        return;
    }

    alertaOficialAtivo = false;

    for (JsonObject aviso : doc.as<JsonArray>()) {
        String id         = aviso["ID"]         | "";
        String uf         = aviso["UF"]         | "";
        String municipio  = aviso["MUNICIPIO"]  | aviso["MUN"] | "";
        String descricao  = aviso["DS"]         | "";
        String severidade = aviso["SEVERIDADE"] | "";

        municipio.toUpperCase();
        descricao.toUpperCase();

        bool ehMG     = (uf == "MG");
        bool ehMuriae = (municipio.indexOf("MURIAE")      >= 0 ||
                         descricao.indexOf("MURIAE")       >= 0 ||
                         descricao.indexOf("ZONA DA MATA") >= 0);

        if (!(ehMG && ehMuriae)) continue;

        if (severidade == "PERIGO" || severidade == "GRANDE PERIGO") {
            alertaOficialAtivo = true;

            if (id == ultimoAlertaID) return;   // mesmo alerta — não reenviar
            ultimoAlertaID = id;

            alertaOficialMsg  = "ALERTA INMET\n";
            alertaOficialMsg += descricao + "\n";
            alertaOficialMsg += "Nivel: " + severidade;

            Serial.println("[INMET] ALERTA: " + severidade);

            if (!alertaJaEnviado) {
                String msg;
                msg  = "🚨 *ALERTA OFICIAL - INMET*\n";
                msg += "━━━━━━━━━━━━━━━\n\n";
                msg += alertaOficialMsg + "\n\n";
                msg += "📍 Muriaé - MG";
                bot.sendMessage(MEU_ID, msg, "Markdown");
                alertaJaEnviado = true;
            }
            return;
        }
    }

    alertaJaEnviado = false;
}
