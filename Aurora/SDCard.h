#ifndef SDCARD_H
#define SDCARD_H

#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <time.h>

// ================= LOG =================
void sdLogErro(const char* modulo, const char* mensagem);
void sdLogSensor(float tempC, int heapKB, int heapPct);

// ================= RELATÓRIO =================
String gerarRelatorio();

// ================= LIMPEZA =================
void limparHistoricoIA();
void limparLogs();
void limparMemoria();

// ================= PERSONALIDADE =================
void salvarPersonalidade(const String& texto);

// ================= ESTRUTURA =================
bool initSD();
void verificarEstrutura();

// ================= CONTEXTO GEMINI =================
// Parâmetros por const ref — evita cópias desnecessárias
void salvarContexto(const String& pergunta, const String& resposta);
String carregarContexto();
String carregarPersonalidade();
String interpretarComando(const String& text);

// ================= AGENDA =================
String arquivoAgendaMes();
void salvarEvento(const String& data, const String& evento);
String lerAgenda();
void verificarLembretes();

// ================= MEMÓRIA =================
String buscarMemoria(const String& pergunta);
void salvarMemoria(const String& pergunta, const String& resposta);
int contarPergunta(const String& pergunta);
void incrementarPergunta(const String& pergunta);

// ================= VARIÁVEIS =================
extern bool sdOK;

#endif
