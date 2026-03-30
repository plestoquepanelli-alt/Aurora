#ifndef CLIMA_H
#define CLIMA_H

#include <Arduino.h>

// ================= FUNÇÕES PÚBLICAS =================
void atualizarClima();
void verificarClimaSevero();

// ================= VARIÁVEIS =================
extern float climaTemp;
extern int climaUmidade;
extern String climaDescricao;
extern bool alertaClimaEnviado;
extern unsigned long ultimoClima;

// ================= PREVISÃO =================
extern float previsao3h;
extern float previsao6h;
extern float previsao9h;
extern float chuva3h;
extern float chuva6h;
extern float chuva9h;

// Dados adicionais
extern float climaSensTermica;
extern int   climaPressao;
extern float climaVento;

// Horarios reais da previsao
extern String hora_prev3h;
extern String hora_prev6h;
extern String hora_prev9h;

// Constante
extern const unsigned long INTERVALO_CLIMA;

#endif
