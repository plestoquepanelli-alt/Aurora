#include "Gemini.h"
#include "Config.h"
#include "Sistema.h"
#include "LEDControl.h"
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include "esp_system.h"
#include "SDCard.h"

extern WiFiClientSecure gemini_client;

// ─── Cache de personalidade (evita SD a cada pergunta) ────────
static String _persCache;
static bool   _persCached = false;

void invalidarCachePersonalidade() { _persCached = false; _persCache = ""; }

static const String& getPersonalidade() {
    if (!_persCached) {
        _persCache = carregarPersonalidade();
        if (_persCache.isEmpty())
            _persCache = "Você é Aurora, assistente pessoal de Pedro. Seja amigável, técnico e conciso.";
        _persCached = true;
    }
    return _persCache;
}

// ─── Alocador PSRAM ── redireciona JsonDocuments para PSRAM ───
struct PsramAlloc {
    void* allocate(size_t n)   { return heap_caps_malloc(n, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT); }
    void  deallocate(void* p)  { free(p); }
    void* reallocate(void* p, size_t n) { return heap_caps_realloc(p, n, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT); }
};
using PsDoc = BasicJsonDocument<PsramAlloc>;

// ─── perguntarGemini ──────────────────────────────────────────
//
//  CORREÇÕES:
//  1. StaticJsonDocument<128> filtro → <320>  (era overflow silencioso
//     causando "Sem resposta" — filtro precisava de ~288 bytes mínimo)
//  2. https.getString() → parse direto no stream  (eliminava String
//     de até 12KB + DynamicJsonDoc 8KB simultâneos → crash/reset)
//  3. req e resp alocados via PSRAM (sem pressão no heap interno)
//  4. req.clear() antes do POST libera os 4KB antes da resposta
//
String perguntarGemini(const String &pergunta) {
    if (ESP.getFreeHeap() < ((ESP.getFreePsram() > 0) ? 18000UL : HEAP_MINIMO)) {
        currentState = ERROR_STATE;
        return "Heap baixo (" + String(ESP.getFreeHeap()) + "B).";
    }
    if (WiFi.status() != WL_CONNECTED) {
        currentState = ERROR_STATE; return "WiFi desconectado.";
    }
    if (String(GEMINI_API_KEY).length() < 20) {
        currentState = ERROR_STATE; return "GEMINI_API_KEY inválida.";
    }

    currentState = PROCESSING;

    String url;
    url.reserve(180);
    url = "https://generativelanguage.googleapis.com/v1beta/models/";
    url += modeloAtivo;
    url += ":generateContent?key=";
    url += GEMINI_API_KEY;

    int backoff = 300;
    for (int tentativa = 1; tentativa <= 2; tentativa++) {
        HTTPClient https;
        gemini_client.setInsecure();
        https.setTimeout(9000);
        https.setReuse(false);

        if (!https.begin(gemini_client, url)) {
            if (tentativa == 2) { currentState = ERROR_STATE; return "Erro conexão Gemini."; }
            esperarComLED(backoff); backoff *= 2; continue;
        }
        https.addHeader("Content-Type", "application/json");

        // ── NTP ───────────────────────────────────────────────
        char dataHoraAtual[64] = "", dataHoraISO[40] = "";
        { struct tm ti; if (getLocalTime(&ti)) {
            strftime(dataHoraAtual, sizeof(dataHoraAtual),
                "Hoje é %d/%m/%Y. São %H:%M (horário de Brasília).", &ti);
            strftime(dataHoraISO,   sizeof(dataHoraISO),
                "%Y-%m-%d %H:%M:%S -03:00", &ti);
        }}

        // ── Requisição JSON (PSRAM) ───────────────────────────
        { // escopo para liberar req antes do parse da resposta
            PsDoc req(4096);
            { const String &p = getPersonalidade();
              String sys; sys.reserve(strlen(dataHoraAtual) + p.length() + 2);
              sys = dataHoraAtual; sys += ' '; sys += p;
              req["system_instruction"]["parts"][0]["text"] = sys; }
            req["generationConfig"]["maxOutputTokens"] = 1200;
            req["generationConfig"]["temperature"]     = 0.45;
            req["generationConfig"]["topP"]            = 0.9;
            JsonArray contents = req.createNestedArray("contents");

            // histórico
            String ctx = carregarContexto();
            if (!ctx.isEmpty() && ctx.length() < 500) {
                int pos = 0, linhas = 0;
                while (pos < (int)ctx.length()) {
                    int nl = ctx.indexOf('\n', pos);
                    if (nl < 0) nl = ctx.length();
                    String ln = ctx.substring(pos, nl); pos = nl + 1;
                    const char *role = nullptr; int off = 0;
                    if      (ln.startsWith("P:")) { role = "user";  off = 2; }
                    else if (ln.startsWith("R:")) { role = "model"; off = 2; }
                    if (role) { JsonObject m = contents.createNestedObject();
                        m["role"] = role; m["parts"][0]["text"] = ln.substring(off); }
                    if ((++linhas % 4) == 0) yield();
                }
            }
            // mensagem atual
            { JsonObject it = contents.createNestedObject();
              it["role"] = "user";
              String q = pergunta;
              if (dataHoraISO[0]) {
                  q += "\n\n[Contexto temporal: agora = ";
                  q += dataHoraISO; q += ".]";
              }
              it["parts"][0]["text"] = q; }

            String body; body.reserve(2400);
            serializeJson(req, body);
            req.clear(); // libera PSRAM antes do POST

            int httpCode = https.POST(body);
            if (httpCode < 0) {
                https.end();
                if (tentativa == 2) { currentState = ERROR_STATE; return "Erro HTTP interno."; }
                esperarComLED(backoff); backoff *= 2; continue;
            }
            if (httpCode == 429 || (httpCode >= 500 && httpCode < 600)) {
                https.end();
                if (tentativa == 2) { currentState = ERROR_STATE; return "Servidor ocupado (" + String(httpCode) + ")"; }
                esperarComLED(backoff); backoff *= 2; continue;
            }
            if (httpCode != 200) {
                String erro = https.getString(); https.end();
                currentState = ERROR_STATE;
                String msg = "HTTP " + String(httpCode);
                DynamicJsonDocument errDoc(384);
                if (deserializeJson(errDoc, erro) == DeserializationError::Ok
                    && errDoc["error"]["message"].is<const char*>())
                    msg += " " + String((const char*)errDoc["error"]["message"]);
                else if (!erro.isEmpty()) msg += " " + erro.substring(0, 100);
                return msg;
            }

            // ── Parse direto no stream (FIX crash) ───────────
            // FIX: filtro precisava de ≥288 bytes — era 128, silenciosamente
            //      descartado pelo ArduinoJson → "Sem resposta"
            StaticJsonDocument<320> filtro;
            filtro["candidates"][0]["content"]["parts"][0]["text"] = true;
            filtro["candidates"][0]["finishReason"]                = true;
            filtro["error"]["message"]                             = true;

            PsDoc resp(8192);
            DeserializationError err = deserializeJson(resp, https.getStream(),
                                           DeserializationOption::Filter(filtro));
            https.end();

            if (err) { currentState = ERROR_STATE; return "Erro JSON: " + String(err.c_str()); }

            String resposta = "Sem resposta";
            if (resp["candidates"][0]["content"]["parts"][0]["text"].is<const char*>())
                resposta = (const char*)resp["candidates"][0]["content"]["parts"][0]["text"];

            if (resp["candidates"][0]["finishReason"].is<const char*>()
                && String((const char*)resp["candidates"][0]["finishReason"]) == "MAX_TOKENS")
                resposta += "\n\n[Limite de tokens. Envie 'continue' para prosseguir.]";

            currentState = SUCCESS;
            otimizarHeap();
            salvarContexto(pergunta, resposta);
            return resposta;
        } // escopo req
    }
    currentState = ERROR_STATE;
    return "Falha desconhecida.";
}
