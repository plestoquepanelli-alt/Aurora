#include "SDCard.h"
#include "Gemini.h"
#include "Config.h"
#include <UniversalTelegramBot.h>

// ================= VARIÁVEIS =================
bool sdOK = false;
static unsigned long ultimoLembrete     = 0;
static int           diaLembreteEnviado = -1;

extern UniversalTelegramBot bot;

// ═══════════════════════════════════════════════════════════════
//  LOG DE ERROS
// ═══════════════════════════════════════════════════════════════
void sdLogErro(const char *modulo, const char *mensagem) {
    if (!sdOK) return;
    if (!SD.exists("/aurora/logs")) SD.mkdir("/aurora/logs");
    File f = SD.open("/aurora/logs/erros.txt", FILE_APPEND);
    if (!f) return;
    struct tm t; char ts[20];
    if (getLocalTime(&t)) strftime(ts, sizeof(ts), "%d/%m %H:%M:%S", &t);
    else strcpy(ts, "??:??:??");
    f.printf("[%s] %s: %s\n", ts, modulo, mensagem);
    f.close();
}

void sdLogSensor(float tempC, int heapKB, int heapPct) {
    if (!sdOK) return;
    struct tm t;
    if (!getLocalTime(&t)) return;
    char path[42];
    strftime(path, sizeof(path), "/aurora/logs/sen_%Y_W%V.csv", &t);
    if (!SD.exists(path)) {
        File f = SD.open(path, FILE_WRITE);
        if (f) { f.println("timestamp,temp_c,heap_kb,heap_pct"); f.close(); }
    }
    File f = SD.open(path, FILE_APPEND);
    if (!f) return;
    char ts[18];
    strftime(ts, sizeof(ts), "%d/%m %H:%M", &t);
    f.printf("%s,%.1f,%d,%d\n", ts, tempC, heapKB, heapPct);
    f.close();
}

// ═══════════════════════════════════════════════════════════════
String gerarRelatorio() {
    if (!sdOK) return "❌ SD não disponível.";
    struct tm t;
    if (!getLocalTime(&t)) return "❌ Horário não sincronizado.";

    char path[42];
    strftime(path, sizeof(path), "/aurora/logs/sen_%Y_W%V.csv", &t);
    if (!SD.exists(path)) return "📭 Sem dados ainda.\nO log inicia após a primeira leitura do loop.";

    File f = SD.open(path, FILE_READ);
    if (!f) return "❌ Erro ao abrir dados.";

    const int MAX = 168;
    float temps[MAX]; int heaps[MAX]; int n = 0;
    f.readStringUntil('\n'); // pula cabeçalho
    while (f.available() && n < MAX) {
        String l = f.readStringUntil('\n'); l.trim();
        if (l.isEmpty()) continue;
        int c1 = l.indexOf(','), c2 = l.indexOf(',', c1+1), c3 = l.indexOf(',', c2+1);
        if (c1 < 0 || c2 < 0 || c3 < 0) continue;
        temps[n] = l.substring(c1+1, c2).toFloat();
        heaps[n] = l.substring(c2+1, c3).toInt();
        n++;
    }
    f.close();
    if (n == 0) return "📭 Arquivo vazio.";

    float tMin=temps[0], tMax=temps[0], tSum=0;
    for (int i = 0; i < n; i++) {
        if (temps[i] < tMin) tMin = temps[i];
        if (temps[i] > tMax) tMax = temps[i];
        tSum += temps[i];
    }
    float tMedia = tSum / n;

    int hMin=heaps[0], hMax=heaps[0]; float hSum=0;
    for (int i = 0; i < n; i++) {
        if (heaps[i] < hMin) hMin = heaps[i];
        if (heaps[i] > hMax) hMax = heaps[i];
        hSum += heaps[i];
    }
    int hMedia = (int)(hSum / n);

    int start = (n > 24) ? n - 24 : 0;
    int cols  = n - start;
    const int ROWS = 5;

    String cT;
    cT.reserve(cols * ROWS + 64);
    float tRange = tMax - tMin; if (tRange < 1.0f) tRange = 1.0f;
    for (int row = ROWS-1; row >= 0; row--) {
        char lbl[8]; snprintf(lbl, sizeof(lbl), "%3.0f|", tMin + (tRange*row/(ROWS-1)));
        cT += String(lbl);
        for (int col = 0; col < cols; col++) {
            float norm = (temps[start+col] - tMin) / tRange * (ROWS-1);
            cT += (norm >= row - 0.4f) ? "#" : " ";
        }
        cT += "\n";
    }
    cT += "   +"; for (int i = 0; i < cols; i++) cT += "-"; cT += "\n";

    String cH;
    cH.reserve(cols * ROWS + 64);
    float hRange = hMax - hMin; if (hRange < 1.0f) hRange = 1.0f;
    for (int row = ROWS-1; row >= 0; row--) {
        char lbl[8]; snprintf(lbl, sizeof(lbl), "%3d|", hMin + (int)(hRange*row/(ROWS-1)));
        cH += String(lbl);
        for (int col = 0; col < cols; col++) {
            float norm = (heaps[start+col] - hMin) / hRange * (ROWS-1);
            cH += (norm >= row - 0.4f) ? "#" : " ";
        }
        cH += "\n";
    }
    cH += "   +"; for (int i = 0; i < cols; i++) cH += "-"; cH += "\n";

    char dataBuf[12];
    strftime(dataBuf, sizeof(dataBuf), "%d/%m/%Y", &t);

    String rel;
    rel.reserve(512);
    rel  = "📊 *Relatório Semanal — Aurora*\n";
    rel += "━━━━━━━━━━━━━━━\n";
    rel += "📅 `" + String(dataBuf) + "` · " + String(n) + " amostras\n\n";
    rel += "🌡 *Temperatura chip (°C)*\n```\n" + cT + "```\n";
    rel += "Min:`"+String(tMin,1)+"` Méd:`"+String(tMedia,1)+"` Máx:`"+String(tMax,1)+"`\n\n";
    rel += "🧠 *Heap livre (KB)*\n```\n" + cH + "```\n";
    rel += "Min:`"+String(hMin)+"` Méd:`"+String(hMedia)+"` Máx:`"+String(hMax)+"`\n\n";
    rel += "_Dados desta semana apagados apos envio._";

    if (rel.length() > 100) SD.remove(path);
    return rel;
}

// ═══════════════════════════════════════════════════════════════
//  LIMPEZA
// ═══════════════════════════════════════════════════════════════
void limparHistoricoIA() {
    if (!sdOK) return;
    SD.remove("/aurora/contexto/conversa.txt");
}

void limparLogs() {
    if (!sdOK) return;
    File root = SD.open("/aurora/logs");
    if (!root || !root.isDirectory()) return;
    char del[16][48]; int n = 0; File e;
    while ((e = root.openNextFile()) && n < 16) {
        snprintf(del[n++], 48, "/aurora/logs/%s", e.name());
        e.close();
    }
    root.close();
    for (int i = 0; i < n; i++) SD.remove(del[i]);
}

void limparMemoria() {
    if (!sdOK) return;
    SD.remove("/aurora/memoria/memoria.csv");
    SD.remove("/aurora/memoria/contador.csv");
    File f1 = SD.open("/aurora/memoria/memoria.csv",  FILE_WRITE); if (f1) f1.close();
    File f2 = SD.open("/aurora/memoria/contador.csv", FILE_WRITE); if (f2) f2.close();
}

// ═══════════════════════════════════════════════════════════════
//  PERSONALIDADE
// ═══════════════════════════════════════════════════════════════
void salvarPersonalidade(const String &texto) {
    if (!sdOK) return;
    File f = SD.open("/aurora/config/personalidade.txt", FILE_WRITE);
    if (!f) return;
    f.print(texto);
    f.close();
    invalidarCachePersonalidade(); // invalida cache
}

// ═══════════════════════════════════════════════════════════════
//  INICIALIZAÇÃO
// ═══════════════════════════════════════════════════════════════
bool initSD() {
    SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
    if (!SD.begin(SD_CS, SPI, 4000000, "/sd", 5, true)) {
        Serial.println("❌ Falha ao iniciar SD");
        return false;
    }
    Serial.println("✅ SD inicializado");
    verificarEstrutura();
    return true;
}

void verificarEstrutura() {
    if (!SD.exists("/aurora"))          SD.mkdir("/aurora");
    if (!SD.exists("/aurora/agenda"))   SD.mkdir("/aurora/agenda");
    if (!SD.exists("/aurora/contexto")) SD.mkdir("/aurora/contexto");
    if (!SD.exists("/aurora/memoria"))  SD.mkdir("/aurora/memoria");
    if (!SD.exists("/aurora/logs"))     SD.mkdir("/aurora/logs");
    if (!SD.exists("/aurora/config"))   SD.mkdir("/aurora/config");
    if (!SD.exists("/aurora/memoria/contador.csv")) {
        File f = SD.open("/aurora/memoria/contador.csv", FILE_WRITE); if (f) f.close();
    }
    if (!SD.exists("/aurora/memoria/memoria.csv")) {
        File f = SD.open("/aurora/memoria/memoria.csv", FILE_WRITE); if (f) f.close();
    }
    if (!SD.exists("/aurora/config/personalidade.txt")) {
        File f = SD.open("/aurora/config/personalidade.txt", FILE_WRITE);
        if (f) {
            f.println("Você é Aurora, assistente pessoal de Pedro.");
            f.println("Seu estilo é extremamente amigável, técnico e inteligente.");
            f.println("Você ajuda com tarefas do dia-a-dia, perguntas curtas, agendamentos e lembretes.");
            f.close();
        }
    }
    Serial.println("📂 Estrutura Aurora OK");
}

// ═══════════════════════════════════════════════════════════════
//  CONTEXTO GEMINI
// ═══════════════════════════════════════════════════════════════
void salvarContexto(const String &pergunta, const String &resposta) {
    if (!sdOK) return;
    const int MAX_PARES = 20;
    const char *ARQ = "/aurora/contexto/conversa.txt";
    int linhas = 0;
    {
        File fc = SD.open(ARQ);
        if (fc) { while (fc.available()) { if (fc.read() == '\n') linhas++; } fc.close(); }
    }
    if (linhas >= MAX_PARES * 2) {
        File fc = SD.open(ARQ);
        if (!fc) { sdLogErro("SDCard", "nao reescreveu contexto"); }
        else {
            String novo; novo.reserve(fc.size());
            int puladas = 0;
            while (fc.available()) {
                String ln = fc.readStringUntil('\n');
                if (puladas < 2) { puladas++; continue; }
                novo += ln + "\n";
            }
            fc.close();
            File fw = SD.open(ARQ, FILE_WRITE);
            if (fw) { fw.print(novo); fw.close(); }
        }
    }
    File f = SD.open(ARQ, FILE_APPEND);
    if (!f) return;
    f.println("P:" + pergunta);
    f.println("R:" + resposta);
    f.close();
}

String carregarContexto() {
    if (!sdOK) return "";
    File f = SD.open("/aurora/contexto/conversa.txt");
    if (!f) return "";
    String c;
    c.reserve(f.size());
    while (f.available()) c += f.readStringUntil('\n') + "\n";
    f.close();
    return c;
}

String carregarPersonalidade() {
    File f = SD.open("/aurora/config/personalidade.txt");
    if (!f) return "";
    String t;
    t.reserve(f.size());
    while (f.available()) t += f.readStringUntil('\n') + "\n";
    f.close();
    return t;
}

String interpretarComando(const String &text) {
    if (text.indexOf("clima")  >= 0) return "/clima";
    if (text.indexOf("hora")   >= 0) return "/hora";
    if (text.indexOf("status") >= 0) return "/status";
    if (text.indexOf("agenda") >= 0) return "/agenda";
    return text;
}

// ═══════════════════════════════════════════════════════════════
//  AGENDA
// ═══════════════════════════════════════════════════════════════
String arquivoAgendaMes() {
    struct tm ti;
    if (!getLocalTime(&ti)) return "/aurora/agenda/agenda.txt";
    char nome[40];
    strftime(nome, sizeof(nome), "/aurora/agenda/%Y_%m.csv", &ti);
    return String(nome);
}

void salvarEvento(const String &data, const String &evento) {
    File f = SD.open(arquivoAgendaMes(), FILE_APPEND);
    if (!f) return;
    f.println(data + ";" + evento);
    f.close();
}

String lerAgenda() {
    File f = SD.open(arquivoAgendaMes());
    if (!f) return "Agenda vazia.";
    String resp;
    resp.reserve(256);
    resp = "📅 *Agenda do mês*\n\n";
    while (f.available()) {
        String l = f.readStringUntil('\n');
        int sep = l.indexOf(';');
        if (sep > 0) {
            String dia = l.substring(0, sep), ev = l.substring(sep+1); ev.trim();
            resp += "📌 Dia *" + dia + "* — " + ev + "\n";
        }
    }
    f.close();
    return resp;
}

void verificarLembretes() {
    if (millis() - ultimoLembrete < 60000UL) return;
    ultimoLembrete = millis();
    struct tm ti; if (!getLocalTime(&ti)) return;
    int dia = ti.tm_mday, hora = ti.tm_hour;
    if (hora != 8 || dia == diaLembreteEnviado) return;
    File f = SD.open(arquivoAgendaMes());
    String eventos; int count = 0;
    if (f) {
        eventos.reserve(128);
        while (f.available()) {
            String l = f.readStringUntil('\n'); l.trim(); if (l.isEmpty()) continue;
            int sep = l.indexOf(';'); if (sep <= 0) continue;
            if (l.substring(0, sep).toInt() == dia) {
                count++;
                String ev = l.substring(sep+1); ev.trim();
                eventos += String(count) + ". " + ev + "\n";
            }
        }
        f.close();
    }
    char buf[12]; strftime(buf, sizeof(buf), "%d/%m/%Y", &ti);
    String msg;
    msg.reserve(200);
    msg  = "🌅 *Bom dia, Pedro!*\n━━━━━━━━━━━━━━━\n📅 *" + String(buf) + "*\n\n";
    msg += count > 0 ? "📌 *" + String(count) + " evento(s) hoje:*\n" + eventos
                     : "✅ Sem compromissos hoje.\n";
    msg += "\n⏱ Aurora online.";
    bot.sendMessage(MEU_ID, msg, "Markdown");
    diaLembreteEnviado = dia;
}

// ═══════════════════════════════════════════════════════════════
//  MEMÓRIA
// ═══════════════════════════════════════════════════════════════
String buscarMemoria(const String &pergunta) {
    if (!sdOK) return "";
    File f = SD.open("/aurora/memoria/memoria.csv"); if (!f) return "";
    String pLower = pergunta; pLower.toLowerCase();
    while (f.available()) {
        String l = f.readStringUntil('\n'); int sep = l.indexOf(';'); if (sep < 0) continue;
        String p = l.substring(0, sep); p.toLowerCase();
        if (pLower.indexOf(p) >= 0) { f.close(); return l.substring(sep+1); }
    }
    f.close(); return "";
}

void salvarMemoria(const String &pergunta, const String &resposta) {
    if (!sdOK) return;
    File f = SD.open("/aurora/memoria/memoria.csv", FILE_APPEND); if (!f) return;
    f.print(pergunta); f.print(";"); f.println(resposta);
    f.close();
}

int contarPergunta(const String &pergunta) {
    if (!sdOK) return 0;
    File f = SD.open("/aurora/memoria/contador.csv"); if (!f) return 0;
    String pLower = pergunta; pLower.toLowerCase();
    while (f.available()) {
        String l = f.readStringUntil('\n'); int sep = l.indexOf(';'); if (sep < 0) continue;
        String p = l.substring(0, sep); p.toLowerCase();
        if (pLower.indexOf(p) >= 0) { f.close(); return l.substring(sep+1).toInt(); }
    }
    f.close(); return 0;
}

void incrementarPergunta(const String &pergunta) {
    if (!sdOK) return;
    const int MAX_ENTRADAS = 500;
    String pLower = pergunta; pLower.toLowerCase();
    File f = SD.open("/aurora/memoria/contador.csv"); if (!f) return;
    String novo; novo.reserve(f.size() + 32);
    bool enc = false;
    while (f.available()) {
        String l = f.readStringUntil('\n'); int sep = l.indexOf(';');
        if (sep < 0) { novo += l + "\n"; continue; }
        String p = l.substring(0, sep); int c = l.substring(sep+1).toInt();
        String pL = p; pL.toLowerCase();
        if (pLower.indexOf(pL) >= 0) { c++; enc = true; novo += p + ";" + String(c) + "\n"; }
        else novo += l + "\n";
    }
    f.close();
    if (!enc) {
        int tot = 0; for (int i = 0; i < (int)novo.length(); i++) if (novo[i] == '\n') tot++;
        if (tot < MAX_ENTRADAS) novo += pergunta + ";1\n";
    }
    File fw = SD.open("/aurora/memoria/contador.csv", FILE_WRITE); if (!fw) return;
    fw.print(novo); fw.close();
}
