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
void salvarContexto(String pergunta, String resposta);
String carregarContexto();
String carregarPersonalidade();
String interpretarComando(String text);

// ================= AGENDA =================
String arquivoAgendaMes();
void salvarEvento(String data, String evento);
String lerAgenda();
void verificarLembretes();

// ================= MEMÓRIA =================
String buscarMemoria(String pergunta);
void salvarMemoria(String pergunta, String resposta);
int contarPergunta(String pergunta);
void incrementarPergunta(String pergunta);

// ================= VARIÁVEIS =================
extern bool sdOK;

#endif
