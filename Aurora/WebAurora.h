#ifndef WEB_AURORA_H
#define WEB_AURORA_H

// ════════════════════════════════════════════════════════════
//  AURORA WEB PANEL — Header público
//
//  Renomeado de WebServer.h para WebAurora.h para evitar
//  conflito com a biblioteca Arduino <WebServer.h>.
//  O compilador resolve <WebServer.h> como biblioteca e
//  "WebServer.h" como arquivo local — com o mesmo nome
//  causava redefinição de classe.
// ════════════════════════════════════════════════════════════

// ── Estado da sessão web (acessível por Aurora.ino/Telegram) ─
extern bool webPanelAtivo;    // servidor iniciado e rodando
extern bool webSessaoAtiva;   // usuário logado ativamente

// ── Funções públicas ────────────────────────────────────────
void initWebServer();         // chama no setup() após WiFi OK
void handleWebServer();       // chama no loop() — não bloqueia
void exibirAcessoWebOLED();   // mostra IP no OLED (botão 46)

#endif
