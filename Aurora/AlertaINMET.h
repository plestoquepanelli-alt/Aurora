#ifndef ALERTA_INMET_H
#define ALERTA_INMET_H

#include <Arduino.h>

// ================= VARIÁVEIS =================
extern bool alertaOficialAtivo;
extern String alertaOficialMsg;

// ================= FUNÇÕES =================
void verificarAlertaINMET();

#endif