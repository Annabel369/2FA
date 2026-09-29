// ===== CREEPER AUTH v7.2.2 - DUAL STACK + NETWORK + SEED COLUMNS (VERSÃO
// FINAL)
// =====
#include "mbedtls/md.h"
#include "qrcode.h"
#include <ArduinoJson.h> // Você precisará instalar a biblioteca ArduinoJson
#include <ESP32FtpServer.h>
#include <ESPmDNS.h>
#include <FS.h>
#include <HTTPClient.h>
#include <NTPClient.h>
#include <SD.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <TJpg_Decoder.h>
#include <WebServer.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiUdp.h>
#include <mbedtls/base64.h>
#include <vector>
// --- TOUCH DO CYD ---
#include <ESP32Servo.h>
#include <XPT2046_Touchscreen.h>
Servo servoCreeper;
const int PINO_RELE_LUZ =
    22; // Alterado de 26 para 22 (livre no conector traseiro)
const int PINO_SERVO = 27; // Mantido no 27 (livre no conector traseiro)
unsigned long tempoAberto = 0;
bool hardwareAtivo = false;
#define XPT2046_IRQ 36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33
SPIClass touchscreenSPI = SPIClass(HSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);
#define SD_CS 5
TFT_eSPI tft = TFT_eSPI();
WiFiUDP ntpUDP;
WiFiUDP udpWhitelist;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000);
WebServer server(80);
FtpServer ftpSrv;
File uploadFile;
// --- DECLARAÇÕES GLOBAIS PARA O MONITOR DO PC ---
WiFiUDP udpPC;
unsigned int portPC = 5005;
char bufferPC[255];
int pcFPS = 0;
int pcGPU = 0;
int pcTemp = 0;
// ------------------------------------------------
// Configurações e Whitelist
String cfgSSID = "Maria Cristina 4G2";
String cfgPASS = "1247bfam12";
String cfgMODO = "REDE";
String cfgIP = "192.168.100.";
String cfgPIX = "810924f7-69b3-4116-8d8f-692e4a25c251"; // Pode ser CPF, E-mail
                                                        // ou Chave Aleatória
String cfgWiser =
    "wise.com/pay/me/amauribuenodossantoss"; // Wiser banco de coversao de
                                             // Moedas seu
                                             // wise.com/pay/me/amauribuenodossantoss
String dynamicWhitelist = "";
String weatherApiKey =
    "fff7ce772990b49a7efd6b1a6827c687"; // https://home.openweathermap.org/api_keys
String city = "Bom Jesus dos Perdões, BR";
String classificarVento(float kmh) {
  if (kmh < 5)
    return "Brisa Calma";
  if (kmh < 20)
    return "Brisa Leve";
  if (kmh < 40)
    return "Vento Moderado";
  if (kmh < 60)
    return "Vento Forte";
  if (kmh < 90)
    return "VENDAVAL";
  if (kmh < 117)
    return "TEMPESTADE";
  return "FURACAO/TORNADO";
}
String getFooter() {
  // Busca a hora atualizada do servidor NTP agora
  time_t epochTime = timeClient.getEpochTime();
  struct tm *ptm = gmtime((time_t *)&epochTime);
  int ano = ptm->tm_year + 1900;
  // Se o NTP ainda não sincronizou, ele vai marcar 1970.
  // Podemos forçar a exibição de 2026 enquanto não sincroniza:
  if (ano < 2025)
    ano = 2026;
  return "<footer>'Copyright' 2025-" + String(ano) +
         " Criado por Amauri Bueno dos Santos com apoio da Gemini. "
         "https://github.com/Annabel369/2FATouch</footer>";
}
char packetBuffer[255];
struct TotpAccount {
  String name;
  String secretBase32;
  String password;
};
struct SeedRecord {
  String label;
  String phrase;
};
struct WeatherData {
  float temp;
  float windSpeed;
  String main;
  String windDesc; // Para guardar o nome (Brisa, Vendaval, etc)
  int code;
};
const char *headerkeys[] = {"Range", "Authorization"};
const size_t headerkeyssize = sizeof(headerkeys) / sizeof(char *);
const int PIN_RED = 4;
const int PIN_GREEN = 16;
const int PIN_BLUE = 17;
std::vector<TotpAccount> accounts;
std::vector<SeedRecord> seeds;
// Variáveis Globais
int displayMode = 1; // Começa no rosto do Creeper
int currentIndex = -1;
int currentSeedIndex = -1;
int lastSec = -1;
bool forceRedraw = true;
bool sessaoAtiva = false; // Controle de sessão customizado
// --- DECLARAÇÕES E FUNÇÕES DE CARREGAMENTO (LOADING SCREEN) ---
void updateWeather(); // Declaração antecipada
void scanJSON(File dir, String &json) {
  File entry = dir.openNextFile();
  bool first = true;
  while (entry) {
    if (!first)
      json += ",";
    json += "{\"name\":\"" + String(entry.path()) + "\",";
    json += "\"isDir\":" + String(entry.isDirectory() ? "true" : "false") + ",";
    json += "\"size\":" + String(entry.size()) + "}";
    first = false;
    if (entry.isDirectory()) {
      // Nota: Para manter o JSON simples, varre todos os níveis
    }
    entry.close();
    entry = dir.openNextFile();
  }
}
void handleUpload() {
  HTTPUpload &upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    String filename = upload.filename;
    // Garante que o nome comece com '/'
    if (!filename.startsWith("/")) {
      filename = "/" + filename;
    }
    // Abre/Cria o arquivo no SD para escrita
    uploadFile = SD.open(filename, FILE_WRITE);
    if (!uploadFile) {
      Serial.println("Erro ao abrir arquivo no SD para escrita!");
    } else {
      Serial.printf("Iniciando upload: %s\n", filename.c_str());
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (uploadFile) {
      uploadFile.write(upload.buf, upload.currentSize);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (uploadFile) {
      uploadFile.close();
      Serial.printf("Upload concluído! Tamanho: %u bytes\n", upload.totalSize);
    }
  }
}
void handleLoginRoute() {
  if (SD.exists("/login.html")) {
    File file = SD.open("/login.html", FILE_READ);
    server.streamFile(file, "text/html");
    file.close();
  } else {
    server.send(404, "text/plain", "login.html nao encontrado");
  }
}
void handleDoLogin() {
  if (server.hasArg("user") && server.hasArg("pass")) {
    String u = server.arg("user");
    String p = server.arg("pass");
    String hashDigitado = calcularSHA256(p);
    String hashSalvo = obterHashDoSD();
    if (hashSalvo.length() == 0) {
      server.send(500, "text/plain", "Erro de leitura no SD");
      return;
    }
    if (u == "creeper" && hashDigitado.equalsIgnoreCase(hashSalvo)) {
      sessaoAtiva = true;
      server.send(200, "text/plain", "OK");
      return;
    }
  }
  server.send(401, "text/plain", "Incorreto");
}
// Le o Hash SHA-256 armazenado no arquivo /ListPass.txt no cartão SD
String obterHashDoSD() {
  if (!SD.exists("/ListPass.txt")) {
    Serial.println("Erro: /ListPass.txt nao encontrado no SD");
    return "";
  }
  File file = SD.open("/ListPass.txt", FILE_READ);
  if (!file) {
    Serial.println("Erro ao abrir /ListPass.txt");
    return "";
  }
  String hashSalvo = file.readStringUntil('\n');
  file.close();
  hashSalvo.trim();
  hashSalvo.replace("\r", ""); // Limpa caracteres ocultos de quebra de linha
  return hashSalvo;
}
void handleLogoutCustom() {
  sessaoAtiva = false;
  server.sendHeader("Location", "/login.html");
  server.send(302, "text/plain", "Logout");
}
void handleEditFile() {
  if (!server.hasArg("file")) {
    server.send(400, "text/plain", "Parametro 'file' ausente");
    return;
  }
  String path = server.arg("file");
  // Garante que o caminho comece com '/' se necessário
  if (!path.startsWith("/")) {
    path = "/" + path;
  }
  // Se o arquivo existir no SD, serve o editor ou o conteúdo
  if (SD.exists(path)) {
    File file = SD.open(path, FILE_READ);
    // Envia o conteúdo ou a página de edição
    server.streamFile(file, "text/plain");
    file.close();
  } else {
    server.send(404, "text/plain", "Arquivo nao encontrado: " + path);
  }
}
void handleDeleteFile() {
  if (!server.hasArg("file")) {
    server.send(400, "text/plain", "Parametro 'file' ausente");
    return;
  }
  // Recebe o caminho
  String path = server.arg("file");
  // Garante que o caminho comece com '/'
  if (!path.startsWith("/")) {
    path = "/" + path;
  }
  // Tenta remover o arquivo do SD
  if (SD.exists(path)) {
    if (SD.remove(path)) {
      // Redireciona de volta para a lista de arquivos
      server.sendHeader("Location", "/list.html");
      server.send(303);
    } else {
      server.send(500, "text/plain", "Falha ao deletar o arquivo: " + path);
    }
  } else {
    server.send(404, "text/plain", "Arquivo nao encontrado: " + path);
  }
}
void handleListHTML() {
  if (!verificarAcesso())
    return;
  if (SD.exists("/list.html")) {
    File file = SD.open("/list.html", FILE_READ);
    server.streamFile(file, "text/html");
    file.close();
  } else {
    server.send(404, "text/plain", "list.html nao encontrado no SD");
  }
}
void handleListHTML2() {
  if (!verificarAcesso())
    return;
  if (SD.exists("/v.html")) {
    File file = SD.open("/v.html", FILE_READ);
    server.streamFile(file, "text/html");
    file.close();
  } else {
    server.send(404, "text/plain", "v.html nao encontrado no SD");
  }
}
// --- Rota de Logout ---
void handleLogout() {
  // Trocar o realm força o navegador a descartar as credenciais salvas do realm
  // anterior
  server.sendHeader("WWW-Authenticate",
                    "Basic realm=\"Sessao Encerrada - Re-login Necessario\"");
  String html =
      "<!DOCTYPE html><html lang='pt-BR'><head><meta charset='UTF-8'>";
  html +=
      "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>Sessao Encerrada</title>";
  html += "<style>";
  html += "body { font-family: sans-serif; background-color: #121212; color: "
          "#e0e0e0; display: flex; justify-content: center; align-items: "
          "center; height: 100vh; margin: 0; }";
  html += ".card { background: #1a1a1a; padding: 30px; border-radius: 8px; "
          "border: 1px solid #333; text-align: center; box-shadow: 0 4px 10px "
          "rgba(0,0,0,0.5); max-width: 420px; }";
  html += "h2 { color: #ff4444; margin-bottom: 15px; }";
  html += "p { color: #aaa; margin-bottom: 20px; font-size: 14px; }";
  html += ".status { background: #261313; color: #ff6666; padding: 10px; "
          "border-radius: 4px; font-family: monospace; font-size: 12px; "
          "margin-bottom: 20px; border: 1px solid #552222; }";
  html += "a { display: inline-block; background: #00ff66; color: #000; "
          "text-decoration: none; padding: 10px 20px; font-weight: bold; "
          "border-radius: 4px; }";
  html += "a:hover { background: #00cc52; }";
  html += "</style></head><body>";
  html += "<div class='card'>";
  html += "<h2>🔒 Sessão Encerrada</h2>";
  html += "<div class='status'>SESSAO_AUTENTICADA = "
          "FALSE<br>CREDENCIAIS_REVOGADAS</div>";
  html += "<p>Suas credenciais foram apagadas com sucesso do navegador.</p>";
  html += "<a href='/list.html'>🔑 Fazer Login Novamente</a>";
  html += "</div></body></html>";
  server.send(401, "text/html", html);
}
void handleListJSON() {
  if (!verificarAcesso())
    return; // Impede a execução se a senha falhar
  String dirPath = "/";
  if (server.hasArg("dir")) {
    dirPath = server.arg("dir");
  }
  File root = SD.open(dirPath);
  String json = "[";
  if (root && root.isDirectory()) {
    File entry = root.openNextFile();
    bool first = true;
    while (entry) {
      if (!first)
        json += ",";
      json += "{\"name\":\"" + String(entry.path()) + "\",";
      json +=
          "\"isDir\":" + String(entry.isDirectory() ? "true" : "false") + ",";
      json += "\"size\":" + String(entry.size()) + "}";
      first = false;
      entry.close();
      entry = root.openNextFile();
    }
    root.close();
  }
  json += "]";
  server.send(200, "application/json", json);
}
void drawLoadingCreeper(int cx, int cy, int cSize) {
  int pX = cSize / 8; // Proporção da largura (8 colunas)
  int pY = cSize / 9; // Proporção da altura (9 linhas)
  tft.fillRect(cx, cy, cSize, cSize, TFT_GREEN); // 1. Fundo Verde
  tft.fillRect(cx + (1 * pX), cy + (1 * pY), 2 * pX, 2 * pY,
               TFT_BLACK); // 2. Olho Esquerdo (linhas 1 e 2)
  tft.fillRect(cx + (5 * pX), cy + (1 * pY), 2 * pX, 2 * pY,
               TFT_BLACK); // 3. Olho Direito (linhas 1 e 2)
  tft.fillRect(cx + (3 * pX), cy + (4 * pY), 2 * pX, 1 * pY,
               TFT_BLACK); // 4. Topo do Nariz (linha 4)
  tft.fillRect(cx + (1 * pX), cy + (5 * pY), 6 * pX, 2 * pY,
               TFT_BLACK); // 5. Meio Lardo da Boca (linhas 5 e 6)
  tft.fillRect(cx + (1 * pX), cy + (7 * pY), 2 * pX, 1 * pY,
               TFT_BLACK); // 6. Perna Esquerda (linha 7)
  tft.fillRect(cx + (5 * pX), cy + (7 * pY), 2 * pX, 1 * pY,
               TFT_BLACK); // 7. Perna Direita (linha 7)
}
void drawLoadingScreen(int percent) {
  tft.fillScreen(TFT_BLACK);
  // 1. Creeper Logo (Tamanho 64x64 centralizado horizontalmente)
  drawLoadingCreeper((tft.width() - 64) / 2, 20, 64);
  // 2. Texto "Loading... 80%"
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawCentreString("Loading... " + String(percent) + "%", tft.width() / 2,
                       100, 4);
  // 3. Barra de carregamento identica a escala/design
  // Borda arredondada branca
  tft.drawRoundRect(14, 145, 212, 40, 8, TFT_WHITE);
  tft.drawRoundRect(15, 146, 210, 38, 7, TFT_WHITE);
  // 10 blocos verdes de carregamento
  int activeSegments = percent / 10;
  for (int i = 0; i < 10; i++) {
    if (i < activeSegments) {
      tft.fillRect(22 + i * 20, 151, 16, 28, TFT_GREEN);
    }
  }
  // 4. Texto "Por favor aguarde..."
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawCentreString("Por favor aguarde...", tft.width() / 2, 205, 4);
}
void carregarTelaMeteorologia() {
  for (int pct = 0; pct <= 80; pct += 10) {
    drawLoadingScreen(pct);
    delay(100);
  }
  updateWeather();
  for (int pct = 90; pct <= 100; pct += 10) {
    drawLoadingScreen(pct);
    delay(80);
  }
}
// --- CONFIGURAÇÕES DE CALIBRAÇÃO CIRÚRGICA DO TOUCH XPT2046 ---
const int TOUCH_MIN_RAW_X = 200;
const int TOUCH_MAX_RAW_X = 3700;
const int TOUCH_MIN_RAW_Y = 200;
const int TOUCH_MAX_RAW_Y = 3700;
const bool TOUCH_INVERT_X =
    false; // Mude para true se o toque horizontal estiver espelhado
const bool TOUCH_INVERT_Y =
    false; // Mude para true se o toque vertical estiver espelhado
const bool TOUCH_SWAP_XY =
    false; // Mude para true se os eixos X e Y estiverem trocados
void converterPontoTouch(TS_Point p, int &tx, int &ty) {
  int rx = p.x;
  int ry = p.y;
  if (TOUCH_SWAP_XY) {
    rx = p.y;
    ry = p.x;
  }

  if (TOUCH_INVERT_X) {
    tx = map(rx, TOUCH_MIN_RAW_X, TOUCH_MAX_RAW_X, 239, 0);
  } else {
    tx = map(rx, TOUCH_MIN_RAW_X, TOUCH_MAX_RAW_X, 0, 239);
  }
  if (TOUCH_INVERT_Y) {
    ty = map(ry, TOUCH_MIN_RAW_Y, TOUCH_MAX_RAW_Y, 319, 0);
  } else {
    ty = map(ry, TOUCH_MIN_RAW_Y, TOUCH_MAX_RAW_Y, 0, 319);
  }
  tx = constrain(tx, 0, 239);
  ty = constrain(ty, 0, 319);
}
// --- VARIÁVEIS E FUNÇÕES DO PORTAL DE CONFIGURAÇÃO WI-FI TOUCH (MODO 7) ---
int wifiSetupState = 0; // 0 = Selecionar SSID (Scan), 1 = Teclado de Senha
std::vector<String> scannedSSIDs;
std::vector<int32_t> scannedRSSI;
String selectedSSID = "";
String inputWifiPass = "";
bool shiftActive = false;
int wifiScanPage = 0;
void iniciarScanWiFiTFT() {
  tft.fillScreen(TFT_BLACK);
  tft.drawRect(0, 0, 240, 320, TFT_GREEN);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawCentreString("PROCURANDO REDES...", 120, 130, 2);
  tft.drawCentreString("Aguarde o Scan Wi-Fi", 120, 160, 2);
  scannedSSIDs.clear();
  scannedRSSI.clear();
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  int n = WiFi.scanNetworks();
  if (n > 0) {
    for (int i = 0; i < n; ++i) {
      String ssid = WiFi.SSID(i);
      if (ssid.length() > 0) {
        bool jaExiste = false;
        for (const auto &s : scannedSSIDs) {
          if (s == ssid) {
            jaExiste = true;
            break;
          }
        }
        if (!jaExiste) {
          scannedSSIDs.push_back(ssid);
          scannedRSSI.push_back(WiFi.RSSI(i));
        }
      }
    }
  }
  WiFi.scanDelete();
  wifiSetupState = 0;
  wifiScanPage = 0;
  forceRedraw = true;
}
void drawWiFiScanScreen() {
  tft.fillScreen(TFT_BLACK);
  tft.drawRect(0, 0, 240, 320, TFT_GREEN);
  tft.drawRect(1, 1, 238, 318, TFT_GREEN);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawCentreString("SELECAO DE WI-FI", 120, 8, 4);
  if (scannedSSIDs.empty()) {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.drawCentreString("Nenhuma rede encontrada", 120, 120, 2);
  } else {
    int totalPages = (scannedSSIDs.size() + 3) / 4;
    if (wifiScanPage >= totalPages)
      wifiScanPage = 0;
    int startIdx = wifiScanPage * 4;
    int endIdx = min((int)scannedSSIDs.size(), startIdx + 4);
    for (int i = startIdx; i < endIdx; i++) {
      int boxY = 42 + (i - startIdx) * 52;
      tft.drawRoundRect(10, boxY, 220, 46, 6, TFT_GREEN);
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      String dispSSID = scannedSSIDs[i];
      if (dispSSID.length() > 16)
        dispSSID = dispSSID.substring(0, 14) + "..";
      tft.drawString(dispSSID, 20, boxY + 8, 2);
      int rssi = scannedRSSI[i];
      uint16_t signalCor =
          (rssi > -65) ? TFT_GREEN : ((rssi > -80) ? TFT_YELLOW : TFT_RED);
      tft.setTextColor(signalCor, TFT_BLACK);
      tft.drawString(String(rssi) + "dBm", 20, boxY + 26, 1);
      tft.setTextColor(TFT_CYAN, TFT_BLACK);
      tft.drawString("[ TOQUE ]", 145, boxY + 15, 2);
    }
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.drawCentreString("Pagina " + String(wifiScanPage + 1) + "/" +
                             String(totalPages),
                         120, 252, 1);
  }
  // Botões Rodapé: [ESCANEAR] e [PROX PAG]
  tft.drawRoundRect(10, 268, 105, 42, 5, TFT_CYAN);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawCentreString("ESCANEAR", 62, 282, 2);
  tft.drawRoundRect(125, 268, 105, 42, 5, TFT_MAGENTA);
  tft.setTextColor(TFT_MAGENTA, TFT_BLACK);
  tft.drawCentreString("PROX PAG", 177, 282, 2);
}
void drawWiFiKeyboardScreen() {
  tft.fillScreen(TFT_BLACK);
  tft.drawRect(0, 0, 240, 320, TFT_GREEN);
  // Cabeçalho com o SSID selecionado
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  String headerText = "SSID: " + selectedSSID;
  if (headerText.length() > 20)
    headerText = headerText.substring(0, 18) + "..";
  tft.drawString(headerText, 10, 8, 2);
  // Caixa de Entrada de Senha
  tft.drawRoundRect(8, 26, 224, 34, 4, TFT_GREEN);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  String passDisplay = inputWifiPass + "_";
  if (passDisplay.length() > 20)
    passDisplay = passDisplay.substring(passDisplay.length() - 20);
  tft.drawString(passDisplay, 14, 34, 2);
  const char *r0 = "1234567890";
  const char *r1_low = "qwertyuiop";
  const char *r1_up = "QWERTYUIOP";
  const char *r2_low = "asdfghjkl.";
  const char *r2_up = "ASDFGHJKL@";
  const char *r3_low = "zxcvbnm_-";
  const char *r3_up = "ZXCVBNM#!";
  const char *r1 = shiftActive ? r1_up : r1_low;
  const char *r2 = shiftActive ? r2_up : r2_low;
  const char *r3 = shiftActive ? r3_up : r3_low;
  // Linha 0 (Números)
  for (int i = 0; i < 10; i++) {
    int kx = 6 + i * 23;
    tft.drawRoundRect(kx, 66, 21, 34, 3, TFT_WHITE);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawChar(r0[i], kx + 6, 75, 2);
  }
  // Linha 1
  for (int i = 0; i < 10; i++) {
    int kx = 6 + i * 23;
    tft.drawRoundRect(kx, 104, 21, 34, 3, TFT_WHITE);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.drawChar(r1[i], kx + 6, 113, 2);
  }
  // Linha 2
  for (int i = 0; i < 10; i++) {
    int kx = 6 + i * 23;
    tft.drawRoundRect(kx, 142, 21, 34, 3, TFT_WHITE);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.drawChar(r2[i], kx + 6, 151, 2);
  }
  // Linha 3 (9 letras + Backspace)
  for (int i = 0; i < 9; i++) {
    int kx = 6 + i * 20;
    tft.drawRoundRect(kx, 180, 18, 34, 3, TFT_WHITE);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.drawChar(r3[i], kx + 4, 189, 2);
  }
  // Botão Apagar (<-)
  tft.drawRoundRect(188, 180, 46, 34, 3, TFT_RED);
  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.drawString("<-", 200, 189, 2);
  // Linha 4: SHIFT | DEL | SAIR | ESPACO
  tft.drawRoundRect(6, 218, 52, 34, 3, shiftActive ? TFT_YELLOW : TFT_CYAN);
  tft.setTextColor(shiftActive ? TFT_YELLOW : TFT_CYAN, TFT_BLACK);
  tft.drawString(shiftActive ? "ABC" : "abc", 14, 227, 2);
  tft.drawRoundRect(62, 218, 52, 34, 3, TFT_ORANGE);
  tft.setTextColor(TFT_ORANGE, TFT_BLACK);
  tft.drawString("DEL", 72, 227, 2);
  tft.drawRoundRect(118, 218, 56, 34, 3, TFT_MAGENTA);
  tft.setTextColor(TFT_MAGENTA, TFT_BLACK);
  tft.drawString("SAIR", 126, 227, 2);
  tft.drawRoundRect(178, 218, 56, 34, 3, TFT_WHITE);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("ESP", 188, 227, 2);
  // Linha 5: Botão Grande Salvar e Conectar
  tft.fillRoundRect(8, 258, 224, 46, 6, TFT_GREEN);
  tft.setTextColor(TFT_BLACK, TFT_GREEN);
  tft.drawCentreString("CONECTAR & SALVAR", 120, 272, 2);
}
void atualizarCaixaSenhaTFT() {
  tft.fillRect(9, 27, 222, 32, TFT_BLACK);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  String passDisplay = inputWifiPass + "_";
  if (passDisplay.length() > 20)
    passDisplay = passDisplay.substring(passDisplay.length() - 20);
  tft.drawString(passDisplay, 14, 34, 2);
}
void handleWiFiTouch(int tx, int ty) {
  if (wifiSetupState == 0) {
    if (scannedSSIDs.size() > 0) {
      int startIdx = wifiScanPage * 4;
      int endIdx = min((int)scannedSSIDs.size(), startIdx + 4);
      for (int i = startIdx; i < endIdx; i++) {
        int boxY = 42 + (i - startIdx) * 52;
        if (ty >= boxY && ty <= (boxY + 46) && tx >= 10 && tx <= 230) {
          tft.fillRoundRect(10, boxY, 220, 46, 6, TFT_YELLOW);
          delay(40);
          selectedSSID = scannedSSIDs[i];
          inputWifiPass = "";
          wifiSetupState = 1;
          forceRedraw = true;
          return;
        }
      }
    }
    // Botão ESCANEAR
    if (tx >= 10 && tx <= 115 && ty >= 268 && ty <= 310) {
      tft.fillRoundRect(10, 268, 105, 42, 5, TFT_YELLOW);
      delay(40);
      iniciarScanWiFiTFT();
      return;
    }
    // Botão PROX PAG
    if (tx >= 125 && tx <= 230 && ty >= 268 && ty <= 310) {
      tft.fillRoundRect(125, 268, 105, 42, 5, TFT_YELLOW);
      delay(40);
      if (scannedSSIDs.size() > 0) {
        int totalPages = (scannedSSIDs.size() + 3) / 4;
        wifiScanPage = (wifiScanPage + 1) % totalPages;
      } else {
        displayMode = 0;
      }
      drawWiFiScanScreen();
      return;
    }
  } else if (wifiSetupState == 1) {
    const char *r0 = "1234567890";
    const char *r1_low = "qwertyuiop";
    const char *r1_up = "QWERTYUIOP";
    const char *r2_low = "asdfghjkl.";
    const char *r2_up = "ASDFGHJKL@";
    const char *r3_low = "zxcvbnm_-";
    const char *r3_up = "ZXCVBNM#!";
    const char *r1 = shiftActive ? r1_up : r1_low;
    const char *r2 = shiftActive ? r2_up : r2_low;
    const char *r3 = shiftActive ? r3_up : r3_low;
    // Linha 0 (Números)
    if (ty >= 66 && ty <= 100) {
      int idx = (tx - 6) / 23;
      if (idx >= 0 && idx < 10) {
        int kx = 6 + idx * 23;
        tft.fillRoundRect(kx, 66, 21, 34, 3, TFT_YELLOW);
        delay(25);
        tft.fillRoundRect(kx, 66, 21, 34, 3, TFT_BLACK);
        tft.drawRoundRect(kx, 66, 21, 34, 3, TFT_WHITE);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.drawChar(r0[idx], kx + 6, 75, 2);
        inputWifiPass += r0[idx];
        atualizarCaixaSenhaTFT();
      }
      return;
    }
    // Linha 1
    if (ty >= 104 && ty <= 138) {
      int idx = (tx - 6) / 23;
      if (idx >= 0 && idx < 10) {
        int kx = 6 + idx * 23;
        tft.fillRoundRect(kx, 104, 21, 34, 3, TFT_YELLOW);
        delay(25);
        tft.fillRoundRect(kx, 104, 21, 34, 3, TFT_BLACK);
        tft.drawRoundRect(kx, 104, 21, 34, 3, TFT_WHITE);
        tft.setTextColor(TFT_GREEN, TFT_BLACK);
        tft.drawChar(r1[idx], kx + 6, 113, 2);
        inputWifiPass += r1[idx];
        atualizarCaixaSenhaTFT();
      }
      return;
    }
    // Linha 2
    if (ty >= 142 && ty <= 176) {
      int idx = (tx - 6) / 23;
      if (idx >= 0 && idx < 10) {
        int kx = 6 + idx * 23;
        tft.fillRoundRect(kx, 142, 21, 34, 3, TFT_YELLOW);
        delay(25);
        tft.fillRoundRect(kx, 142, 21, 34, 3, TFT_BLACK);
        tft.drawRoundRect(kx, 142, 21, 34, 3, TFT_WHITE);
        tft.setTextColor(TFT_GREEN, TFT_BLACK);
        tft.drawChar(r2[idx], kx + 6, 151, 2);
        inputWifiPass += r2[idx];
        atualizarCaixaSenhaTFT();
      }
      return;
    }
    // Linha 3 (Letras + Backspace)
    if (ty >= 180 && ty <= 214) {
      if (tx >= 188 && tx <= 234) {
        tft.fillRoundRect(188, 180, 46, 34, 3, TFT_YELLOW);
        delay(25);
        tft.fillRoundRect(188, 180, 46, 34, 3, TFT_BLACK);
        tft.drawRoundRect(188, 180, 46, 34, 3, TFT_RED);
        tft.setTextColor(TFT_RED, TFT_BLACK);
        tft.drawString("<-", 200, 189, 2);
        if (inputWifiPass.length() > 0) {
          inputWifiPass.remove(inputWifiPass.length() - 1);
        }
        atualizarCaixaSenhaTFT();
      } else {
        int idx = (tx - 6) / 20;
        if (idx >= 0 && idx < 9) {
          int kx = 6 + idx * 20;
          tft.fillRoundRect(kx, 180, 18, 34, 3, TFT_YELLOW);
          delay(25);
          tft.fillRoundRect(kx, 180, 18, 34, 3, TFT_BLACK);
          tft.drawRoundRect(kx, 180, 18, 34, 3, TFT_WHITE);
          tft.setTextColor(TFT_GREEN, TFT_BLACK);
          tft.drawChar(r3[idx], kx + 4, 189, 2);
          inputWifiPass += r3[idx];
          atualizarCaixaSenhaTFT();
        }
      }
      return;
    }
    // Linha 4: SHIFT | DEL | SAIR | ESPACO
    if (ty >= 218 && ty <= 252) {
      if (tx >= 6 && tx <= 58) {
        shiftActive = !shiftActive;
        drawWiFiKeyboardScreen();
      } else if (tx >= 62 && tx <= 114) {
        inputWifiPass = "";
        atualizarCaixaSenhaTFT();
      } else if (tx >= 118 && tx <= 174) {
        if (WiFi.status() == WL_CONNECTED) {
          displayMode = 0;
          forceRedraw = true;
        } else {
          wifiSetupState = 0;
          drawWiFiScanScreen();
        }
      } else if (tx >= 178 && tx <= 234) {
        inputWifiPass += ' ';
        atualizarCaixaSenhaTFT();
      }
      return;
    }
    // Linha 5: CONECTAR & SALVAR
    if (ty >= 258 && ty <= 304 && tx >= 8 && tx <= 232) {
      tft.fillRoundRect(8, 258, 224, 46, 6, TFT_YELLOW);
      delay(80);
      cfgSSID = selectedSSID;
      cfgPASS = inputWifiPass;
      salvarConfig();
      tft.fillScreen(TFT_BLACK);
      tft.drawRect(0, 0, 240, 320, TFT_GREEN);
      tft.setTextColor(TFT_GREEN, TFT_BLACK);
      tft.drawCentreString("REDE SALVA!", 120, 100, 4);
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.drawCentreString("SSID: " + cfgSSID, 120, 150, 2);
      tft.drawCentreString("REINICIANDO...", 120, 200, 4);
      delay(2500);
      ESP.restart();
    }
  }
}
// --- FUNÇÕES DE TROCA DE TELA COM TOUCH ---
void proximaTela() {
  int validos[] = {0, 1, 2, 4, 5, 6, 7};
  int n = 7;
  int idx = 0;
  for (int i = 0; i < n; i++)
    if (validos[i] == displayMode) {
      idx = i;
      break;
    }
  displayMode = validos[(idx + 1) % n];
  if (displayMode == 4)
    carregarTelaMeteorologia();
  else if (displayMode == 7)
    iniciarScanWiFiTFT();
  forceRedraw = true;
}
void telaAnterior() {
  int validos[] = {0, 1, 2, 4, 5, 6, 7};
  int n = 7;
  int idx = 0;
  for (int i = 0; i < n; i++)
    if (validos[i] == displayMode) {
      idx = i;
      break;
    }
  displayMode = validos[(idx - 1 + n) % n];
  if (displayMode == 4)
    carregarTelaMeteorologia();
  else if (displayMode == 7)
    iniciarScanWiFiTFT();
  forceRedraw = true;
}
String versaoAtual = "7.2";
String urlVersaoGitHub =
    "https://raw.githubusercontent.com/Annabel369/2FA/main/version.txt";
String versaoNova = ""; // Vai guardar a versão que o GitHub responder
bool updateDisponivel = false;
// Função auxiliar para calcular SHA-256 de uma String
String calcularSHA256(String input) {
  byte shaResult[32];
  mbedtls_md_context_t ctx;
  mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;
  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 0);
  mbedtls_md_starts(&ctx);
  mbedtls_md_update(&ctx, (const unsigned char *)input.c_str(), input.length());
  mbedtls_md_finish(&ctx, shaResult);
  mbedtls_md_free(&ctx);
  String hashStr = "";
  for (int i = 0; i < 32; i++) {
    char buf[3];
    sprintf(buf, "%02x", shaResult[i]);
    hashStr += buf;
  }
  return hashStr;
}
// 2. Decodifica o cabeçalho Base64 enviado pelo navegador (HTTP Basic Auth)
String decodificarBase64(String input) {
  unsigned char output[128];
  size_t output_len = 0;
  int ret = mbedtls_base64_decode(output, sizeof(output) - 1, &output_len,
                                  (const unsigned char *)input.c_str(),
                                  input.length());
  if (ret == 0) {
    output[output_len] = '\0';
    return String((char *)output);
  }
  return "";
}
// 3. Valida se a senha digitada bate com o Hash do SD
bool verificarAcesso() {
  // Se o usuário já logou via login.html (sessão customizada), permite acesso
  // direto
  if (sessaoAtiva) {
    return true;
  }
  if (!SD.exists("/ListPass.txt")) {
    server.send(500, "text/plain", "Erro: ListPass.txt nao encontrado no SD");
    return false;
  }
  // 1. Lê o Hash salvo no SD e remove todos os caracteres invisiveis (\r, \n,
  // espaços)
  File file = SD.open("/ListPass.txt", FILE_READ);
  if (!file)
    return false;
  String hashSalvo = file.readStringUntil('\n');
  file.close();
  hashSalvo.trim();
  hashSalvo.replace("\r", ""); // Remove CR do Windows
  // 2. Se não veio o cabeçalho "Authorization", pede o login ao navegador
  if (!server.hasHeader("Authorization")) {
    server.sendHeader("WWW-Authenticate",
                      "Basic realm=\"Acesso Restrito ao SD\"");
    server.send(401, "text/plain", "Acesso nao autorizado");
    return false;
  }
  // 3. Captura e decodifica as credenciais enviadas
  String authHeader = server.header("Authorization");
  if (authHeader.startsWith("Basic ")) {
    String base64Credentials = authHeader.substring(6);
    String decodedStr =
        decodificarBase64(base64Credentials); // Formato "usuario:senha"
    int colonIndex = decodedStr.indexOf(':');
    if (colonIndex != -1) {
      String usuarioDigitado = decodedStr.substring(0, colonIndex);
      String senhaDigitada = decodedStr.substring(colonIndex + 1);
      // Gera o SHA-256 da senha que você acabou de digitar na caixa do
      // navegador
      String hashSenhaDigitada = calcularSHA256(senhaDigitada);
      hashSenhaDigitada.toLowerCase();
      hashSalvo.toLowerCase();
      // DEBUG VIA SERIAL (Para você ver se o hash bateu)
      Serial.println("--- TENTATIVA DE LOGIN ---");
      Serial.println("Usuario: " + usuarioDigitado);
      Serial.println("Hash Digitado: " + hashSenhaDigitada);
      Serial.println("Hash no SD:       " + hashSalvo);
      // Compara se o Usuário é 'creeper' e se os Hashes são idênticos
      if (usuarioDigitado == "creeper" &&
          hashSenhaDigitada.equalsIgnoreCase(hashSalvo)) {
        Serial.println(">> ACESSO PERMITIDO <<");
        return true;
      } else {
        Serial.println(">> ACESSO NEGADO: Hash incorreto <<");
      }
    }
  }
  // Se errou a senha/usuário, força a caixa de login a reaparecer
  server.sendHeader("WWW-Authenticate",
                    "Basic realm=\"Acesso Restrito ao SD\"");
  server.send(401, "text/plain", "Usuario ou senha incorretos");
  return false;
}
// Função para validar se o usuário e senha informados no pop-up estão corretos
bool autenticarUsuario() {
  // Lemos o hash esperado do arquivo ListPass.txt no SD
  if (!SD.exists("/ListPass.txt")) {
    Serial.println("Erro: Arquivo ListPass.txt nao encontrado no SD!");
    return false;
  }
  File file = SD.open("/ListPass.txt", FILE_READ);
  if (!file)
    return false;
  String hashSalvo = file.readStringUntil('\n');
  hashSalvo.trim(); // Remove quebras de linha/espaços
  file.close();
  // Verifica se o cliente enviou credenciais HTTP Basic Auth
  if (!server.authenticate("creeper",
                           "dummy")) { // Teste rápido de envio de credencial
    // Captura o que o usuário digitou
    String userDigitado = server.arg("user"); // O WebServer valida internamente
  }
  // Pegamos a senha enviada pelo navegador via HTTP Auth
  // O server.requestHeader("Authorization") ou validação interna do WebServer
  // Vamos usar uma abordagem onde comparamos a senha recebida:
  // Como o server.authenticate() do ESP32 checa diretamente o texto puro,
  // fazemos a checagem calculando o hash da senha enviada:
  // Para extrair a senha enviada no header de autenticação Basic:
  if (server.hasHeader("Authorization")) {
    String authHeader = server.header("Authorization");
    if (authHeader.startsWith("Basic ")) {
      // O header é Base64(usuario:senha)
      // Mas para simplificar usando a função nativa do WebServer:
    }
  }
  return false;
}
WiFiClientSecure client;
WeatherData weather;
bool tlsAtivado = false;
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h,
                uint16_t *bitmap) {
  if (y >= tft.height())
    return false;
  // Força o desenho do bloco de pixels
  tft.startWrite();
  tft.setAddrWindow(x, y, w, h);
  tft.pushColors(bitmap, w * h, true);
  tft.endWrite();
  return true;
}
void handleFileRead() {
  String path = server.uri();
  path = uriDecode(path);
  if (path.endsWith("/"))
    path += "index.html";
  String contentType = "text/plain";
  if (path.endsWith(".html"))
    contentType = "text/html";
  else if (path.endsWith(".css"))
    contentType = "text/css";
  else if (path.endsWith(".js"))
    contentType = "application/javascript";
  else if (path.endsWith(".json"))
    contentType = "application/json";
  else if (path.endsWith(".jpg"))
    contentType = "image/jpeg";
  else if (path.endsWith(".png"))
    contentType = "image/png";
  else if (path.endsWith(".ico"))
    contentType = "image/x-icon";
  else if (path.endsWith(".mp4"))
    contentType = "video/mp4";
  else if (path.endsWith(".mp3"))
    contentType = "audio/mpeg";
  if (!SD.exists(path)) {
    // Tenta verificar sem a barra inicial caso o SD exija
    if (path.startsWith("/") && SD.exists(path.substring(1))) {
      path = path.substring(1);
    } else {
      server.send(404, "text/plain", "Arquivo nao encontrado no SD");
      return;
    }
  }
  File file = SD.open(path, "r");
  // Para vídeos MP4, informamos que o servidor aceita requisições por intervalo
  // de bytes
  server.sendHeader("Accept-Ranges", "bytes");
  if (server.hasHeader("Range")) {
    String range = server.header("Range");
    // Extrai a posição inicial requisitada pelo navegador
    int rangeStart = 0;
    int equalIdx = range.indexOf('=');
    int dashIdx = range.indexOf('-');
    if (equalIdx != -1 && dashIdx != -1) {
      String startStr = range.substring(equalIdx + 1, dashIdx);
      if (startStr.length() > 0)
        rangeStart = startStr.toInt();
    }
    size_t totalSize = file.size();
    if (rangeStart < totalSize) {
      file.seek(rangeStart);
      size_t contentLength = totalSize - rangeStart;
      server.sendHeader("Content-Range", "bytes " + String(rangeStart) + "-" +
                                             String(totalSize - 1) + "/" +
                                             String(totalSize));
      server.setContentLength(contentLength);
      server.send(206, contentType, ""); // HTTP 206 Partial Content
      WiFiClient client = server.client();
      uint8_t buffer[2048]; // Buffer otimizado de 2KB
      while (client.connected() && file.available()) {
        size_t bytesRead = file.read(buffer, sizeof(buffer));
        client.write(buffer, bytesRead);
      }
      file.close();
      return;
    }
  }
  // Se não houver requisição de Range, faz o stream padrão
  server.streamFile(file, contentType);
  file.close();
}
String uriDecode(String str) {
  String decoded = "";
  char c;
  for (int i = 0; i < str.length(); i++) {
    c = str.charAt(i);
    if (c == '+') {
      decoded += ' ';
    } else if (c == '%' && i + 2 < str.length()) {
      char code1 = str.charAt(i + 1);
      char code2 = str.charAt(i + 2);
      c = (char)strtol((String(code1) + String(code2)).c_str(), NULL, 16);
      decoded += c;
      i += 2;
    } else {
      decoded += c;
    }
  }
  return decoded;
}
void checkUpdate() {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClientSecure client;
    client.setInsecure(); // Necessário para pular a checagem de certificado SSL
    HTTPClient http;
    http.setFollowRedirects(
        HTTPC_STRICT_FOLLOW_REDIRECTS); // Importante para o GitHub
    Serial.println("[Update] Conectando ao GitHub Raw...");
    if (http.begin(client, urlVersaoGitHub)) {
      int httpCode = http.GET();
      if (httpCode == 200) {
        versaoNova = http.getString();
        versaoNova.trim(); // Remove espaços e pulos de linha
        Serial.print("[Update] Versao no GitHub: ");
        Serial.println(versaoNova);
        Serial.print("[Update] Versao no ESP32: ");
        Serial.println(versaoAtual);
        // Se a versão do GitHub for diferente da versão atual do código
        if (versaoNova != "" && versaoNova != versaoAtual) {
          updateDisponivel = true;
          Serial.println("!!! AVISO: Versao nova encontrada !!!");
        }
      } else {
        Serial.printf("[Update] Erro HTTP: %d\n", httpCode);
      }
      http.end();
    }
  }
}
void drawPCPerformance() {
  tft.fillScreen(TFT_BLACK);
  // Estilo Matrix/Creeper
  tft.drawRect(0, 0, 240, 240, TFT_GREEN);
  tft.drawRect(2, 2, 236, 236, TFT_GREEN);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawCentreString("NVIDIA INFO", 120, 15, 2);
  // FPS em destaque
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  // --- LÓGICA DO FPS (LIMITE 999) ---
  int fpsExibir = pcFPS;
  if (fpsExibir > 999)
    fpsExibir = 999; // Trava o limite
  if (fpsExibir < 0)
    fpsExibir = 0; // Evita números negativos
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  // Criamos uma string com espaços para "limpar" o rastro do número anterior
  // O "  " no final é o segredo para não picar a tela
  String txtFPS = String(fpsExibir) + " ";
  // Desenha o número grande (Fonte 7) centralizado um pouco para a esquerda
  tft.drawCentreString(txtFPS, 110, 50, 7);
  // Desenha o "FPS" menor (Fonte 4) fixo ao lado
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("FPS", 175, 75, 4);
  // Barras de Carga
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString("GPU LOAD: " + String(pcGPU) + "%", 30, 140, 4);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.fillRect(30, 165, (pcGPU * 1.8), 10, TFT_YELLOW); // Barra dinâmica
  tft.setTextColor(TFT_ORANGE, TFT_BLACK);
  tft.drawString("GPU TEMP: " + String(pcTemp) + "C", 30, 190, 4);
  if (pcTemp <= 0) {
    desligarTela();
  }
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.fillRect(30, 215, (pcTemp * 2), 10, (pcTemp > 75) ? TFT_RED : TFT_YELLOW);
}
String get_weather_description(int code) {
  switch (code) {
  case 0:
    return "Ceu Limpo";
  case 1:
  case 2:
  case 3:
    return "Nuvens Esparsas";
  case 45:
  case 48:
    return "Nevoeiro";
  case 51:
  case 53:
  case 55:
  case 61:
  case 63:
  case 65:
    return "Chuva leve";
  case 80:
  case 81:
  case 82:
    return "Chuva forte";
  case 95:
  case 96:
  case 99:
    return "Tempestade";
  default:
    return "Nublado";
  }
}
void updateWeather() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    // URL Open-Meteo direta (São Paulo -> Lat: -23.5505, Lon: -46.6333)
    String url =
        "http://api.open-meteo.com/v1/"
        "forecast?latitude=-23.5505&longitude=-46.6333&current_weather=true";
    Serial.println("Buscando dados meteorologicos...");
    http.begin(url);
    int httpCode = http.GET();
    if (httpCode == 200) {
      String payload = http.getString();
      // Documento dinamico para processar o JSON da One Call
      DynamicJsonDocument doc(2048);
      DeserializationError error = deserializeJson(doc, payload);
      if (!error) {
        // 1. Temperatura
        weather.temp = doc["current_weather"]["temperature"];
        // 2. Vento (A API ja envia em km/h)
        weather.windSpeed = doc["current_weather"]["windspeed"];
        // 3. Classificacao do vento (Brisa, Vendaval, Tornado...)
        weather.windDesc = classificarVento(weather.windSpeed);
        // 4. Descricao do Ceu
        int wCode = doc["current_weather"]["weathercode"];
        weather.code = wCode;
        weather.main = get_weather_description(wCode);
        // Log para o Monitor Serial
        Serial.println("--- DADOS RECEBIDOS ---");
        Serial.print("Temp: ");
        Serial.print(weather.temp);
        Serial.println(" C");
        Serial.print("Vento: ");
        Serial.print(weather.windSpeed);
        Serial.print(" km/h - ");
        Serial.println(weather.windDesc);
        Serial.println("-----------------------");
        forceRedraw = true; // Força a atualização do visor TFT
      } else {
        Serial.print("Erro ao processar JSON: ");
        Serial.println(error.c_str());
      }
    } else {
      Serial.printf("Erro na comunicacao (HTTP): %d\n", httpCode);
    }
    http.end();
  }
}
void drawIconSun(int x, int y) {
  tft.fillCircle(x + 30, y + 25, 12, TFT_YELLOW);
  tft.drawLine(x + 30, y + 5, x + 30, y + 11, TFT_ORANGE);  // N
  tft.drawLine(x + 30, y + 39, x + 30, y + 45, TFT_ORANGE); // S
  tft.drawLine(x + 10, y + 25, x + 16, y + 25, TFT_ORANGE); // W
  tft.drawLine(x + 44, y + 25, x + 50, y + 25, TFT_ORANGE); // E
  tft.drawLine(x + 16, y + 11, x + 20, y + 15, TFT_ORANGE); // NW
  tft.drawLine(x + 44, y + 11, x + 40, y + 15, TFT_ORANGE); // NE
  tft.drawLine(x + 16, y + 39, x + 20, y + 35, TFT_ORANGE); // SW
  tft.drawLine(x + 44, y + 39, x + 40, y + 35, TFT_ORANGE); // SE
}
void drawIconCloud(int x, int y) {
  uint16_t skyBlue = tft.color565(135, 206, 250);
  uint16_t lightBlue = tft.color565(176, 224, 230);
  tft.fillCircle(x + 20, y + 28, 10, skyBlue);
  tft.fillCircle(x + 40, y + 28, 10, skyBlue);
  tft.fillCircle(x + 30, y + 20, 14, lightBlue);
  tft.fillRect(x + 20, y + 22, 20, 16, skyBlue);
}
void drawIconRain(int x, int y) {
  uint16_t darkCloud = tft.color565(70, 130, 180);
  uint16_t lightCloud = tft.color565(100, 149, 237);
  tft.fillCircle(x + 20, y + 24, 9, darkCloud);
  tft.fillCircle(x + 40, y + 24, 9, darkCloud);
  tft.fillCircle(x + 30, y + 16, 12, lightCloud);
  tft.fillRect(x + 20, y + 18, 20, 15, darkCloud);
  uint16_t dropColor = tft.color565(0, 191, 255);
  tft.drawLine(x + 20, y + 34, x + 17, y + 42, dropColor);
  tft.drawLine(x + 30, y + 36, x + 27, y + 44, dropColor);
  tft.drawLine(x + 40, y + 34, x + 37, y + 42, dropColor);
}
void drawIconSnow(int x, int y) {
  uint16_t darkCloud = tft.color565(70, 130, 180);
  uint16_t lightCloud = tft.color565(100, 149, 237);
  tft.fillCircle(x + 20, y + 24, 9, darkCloud);
  tft.fillCircle(x + 40, y + 24, 9, darkCloud);
  tft.fillCircle(x + 30, y + 16, 12, lightCloud);
  tft.fillRect(x + 20, y + 18, 20, 15, darkCloud);
  tft.drawLine(x + 18, y + 38, x + 22, y + 38, TFT_WHITE);
  tft.drawLine(x + 20, y + 36, x + 20, y + 40, TFT_WHITE);
  tft.drawLine(x + 28, y + 40, x + 32, y + 40, TFT_WHITE);
  tft.drawLine(x + 30, y + 38, x + 30, y + 42, TFT_WHITE);
  tft.drawLine(x + 38, y + 38, x + 42, y + 38, TFT_WHITE);
  tft.drawLine(x + 40, y + 36, x + 40, y + 40, TFT_WHITE);
}
void drawIconThunderstorm(int x, int y) {
  uint16_t darkCloud = tft.color565(47, 79, 79);
  uint16_t lightCloud = tft.color565(112, 128, 144);
  tft.fillCircle(x + 20, y + 22, 9, darkCloud);
  tft.fillCircle(x + 40, y + 22, 9, darkCloud);
  tft.fillCircle(x + 30, y + 14, 12, lightCloud);
  tft.fillRect(x + 20, y + 16, 20, 15, darkCloud);
  uint16_t dropColor = tft.color565(0, 191, 255);
  tft.drawLine(x + 18, y + 32, x + 15, y + 40, dropColor);
  tft.drawLine(x + 42, y + 32, x + 39, y + 40, dropColor);
  tft.drawLine(x + 32, y + 28, x + 26, y + 36, TFT_YELLOW);
  tft.drawLine(x + 26, y + 36, x + 33, y + 36, TFT_YELLOW);
  tft.drawLine(x + 33, y + 36, x + 27, y + 45, TFT_YELLOW);
  tft.drawLine(x + 33, y + 28, x + 27, y + 36, TFT_YELLOW);
  tft.drawLine(x + 27, y + 36, x + 34, y + 36, TFT_YELLOW);
  tft.drawLine(x + 34, y + 36, x + 28, y + 45, TFT_YELLOW);
}
void drawIconWind(int x, int y) {
  uint16_t windColor = tft.color565(176, 224, 230);
  tft.drawLine(x + 10, y + 15, x + 40, y + 15, windColor);
  tft.drawCircle(x + 43, y + 18, 3, windColor);
  tft.drawLine(x + 5, y + 25, x + 45, y + 25, windColor);
  tft.drawLine(x + 15, y + 35, x + 35, y + 35, windColor);
  tft.drawCircle(x + 38, y + 38, 3, windColor);
}
void drawIconFog(int x, int y) {
  uint16_t fogCloud = tft.color565(176, 196, 222);
  tft.fillCircle(x + 20, y + 20, 9, fogCloud);
  tft.fillCircle(x + 40, y + 20, 9, fogCloud);
  tft.fillCircle(x + 30, y + 12, 12, fogCloud);
  tft.fillRect(x + 20, y + 14, 20, 15, fogCloud);
  uint16_t fogLine = tft.color565(211, 211, 211);
  tft.drawLine(x + 12, y + 34, x + 48, y + 34, fogLine);
  tft.drawLine(x + 8, y + 39, x + 52, y + 39, fogLine);
  tft.drawLine(x + 16, y + 44, x + 44, y + 44, fogLine);
}
void drawIconHail(int x, int y) {
  uint16_t darkCloud = tft.color565(70, 130, 180);
  uint16_t lightCloud = tft.color565(100, 149, 237);
  tft.fillCircle(x + 20, y + 24, 9, darkCloud);
  tft.fillCircle(x + 40, y + 24, 9, darkCloud);
  tft.fillCircle(x + 30, y + 16, 12, lightCloud);
  tft.fillRect(x + 20, y + 18, 20, 15, darkCloud);
  tft.drawLine(x + 20, y + 34, x + 18, y + 40, TFT_WHITE);
  tft.fillCircle(x + 18, y + 43, 2, TFT_WHITE);
  tft.drawLine(x + 30, y + 34, x + 28, y + 40, TFT_WHITE);
  tft.fillCircle(x + 28, y + 43, 2, TFT_WHITE);
  tft.drawLine(x + 40, y + 34, x + 38, y + 40, TFT_WHITE);
  tft.fillCircle(x + 38, y + 43, 2, TFT_WHITE);
}
void drawIconTornado(int x, int y) {
  uint16_t tornadoColor = tft.color565(112, 128, 144);
  tft.drawRoundRect(x + 10, y + 10, 40, 6, 3, tornadoColor);
  tft.drawRoundRect(x + 15, y + 18, 30, 5, 2, tornadoColor);
  tft.drawRoundRect(x + 20, y + 25, 20, 5, 2, tornadoColor);
  tft.drawRoundRect(x + 24, y + 32, 12, 4, 2, tornadoColor);
  tft.drawLine(x + 28, y + 38, x + 30, y + 44, tornadoColor);
  tft.drawCircle(x + 26, y + 45, 1, tornadoColor);
  tft.drawCircle(x + 34, y + 45, 1, tornadoColor);
}
String obterEstacao(int mes, int dia) {
  int d = mes * 100 + dia;
  if (d >= 1221 || d < 320) {
    return "Verao";
  } else if (d >= 320 && d < 620) {
    return "Outono";
  } else if (d >= 620 && d < 922) {
    return "Inverno";
  } else {
    return "Primavera";
  }
}
String obterFaseLua(unsigned long epoch, bool &isBloodMoon) {
  unsigned long ref = 947182440;
  double cycle = 2551442.877;
  double diff = 0;
  if (epoch > ref) {
    diff = (double)(epoch - ref);
  } else {
    diff = (double)(ref - epoch);
  }
  double phase = fmod(diff, cycle);
  if (epoch < ref && phase > 0) {
    phase = cycle - phase;
  }
  double fraction = phase / cycle;
  isBloodMoon =
      (fraction >= 0.44 && fraction < 0.56) && (((epoch / 86400) % 20) == 5);
  if (isBloodMoon) {
    return "Lua de Sangue";
  }
  if (fraction < 0.06 || fraction >= 0.94) {
    return "Lua Nova";
  } else if (fraction >= 0.06 && fraction < 0.44) {
    return "Lua Crescente";
  } else if (fraction >= 0.44 && fraction < 0.56) {
    return "Lua Cheia";
  } else {
    return "Lua Minguante";
  }
}
void drawIconMoon(int x, int y, unsigned long epoch) {
  bool isBloodMoon = false;
  obterFaseLua(epoch, isBloodMoon);
  unsigned long ref = 947182440;
  double cycle = 2551442.877;
  double diff = 0;
  if (epoch > ref) {
    diff = (double)(epoch - ref);
  } else {
    diff = (double)(ref - epoch);
  }
  double phase = fmod(diff, cycle);
  if (epoch < ref && phase > 0) {
    phase = cycle - phase;
  }
  double fraction = phase / cycle;
  // Estrelas ao redor
  tft.drawPixel(x + 10, y + 10, TFT_WHITE);
  tft.drawPixel(x + 45, y + 8, TFT_WHITE);
  tft.drawPixel(x + 12, y + 35, TFT_WHITE);
  tft.drawPixel(x + 48, y + 38, TFT_WHITE);
  uint16_t moonColor =
      isBloodMoon ? tft.color565(220, 40, 40) : tft.color565(240, 240, 245);
  uint16_t shadowColor = TFT_BLACK;
  if (fraction < 0.06 || fraction >= 0.94) {
    tft.drawCircle(x + 30, y + 25, 12, tft.color565(80, 80, 80));
    tft.drawPixel(x + 30, y + 25, TFT_WHITE);
  } else if (fraction >= 0.06 && fraction < 0.44) {
    tft.fillCircle(x + 30, y + 25, 12, moonColor);
    tft.fillCircle(x + 36, y + 25, 12, shadowColor);
  } else if (fraction >= 0.44 && fraction < 0.56) {
    tft.fillCircle(x + 30, y + 25, 12, moonColor);
  } else {
    tft.fillCircle(x + 30, y + 25, 12, moonColor);
    tft.fillCircle(x + 24, y + 25, 12, shadowColor);
  }
}
void drawWeatherIcon(int code, float windSpeed, int x, int y,
                     unsigned long epoch) {
  if (windSpeed >= 90) {
    drawIconTornado(x, y);
    return;
  }
  if (windSpeed >= 40 && (code == 0 || code == 1 || code == 2 || code == 3)) {
    drawIconWind(x, y);
    return;
  }
  time_t rawtime = (time_t)epoch - (3 * 3600);
  struct tm *ti = localtime(&rawtime);
  bool isNight = (ti->tm_hour >= 18 || ti->tm_hour < 6);
  // Se for noite e o céu não estiver totalmente limpo, desenha a lua
  // menor/atrás das nuvens
  if (isNight && code != 0) {
    drawIconMoon(x + 8, y - 10, epoch);
  }
  switch (code) {
  case 0:
    if (isNight) {
      drawIconMoon(x, y, epoch);
    } else {
      drawIconSun(x, y);
    }
    break;
  case 1:
  case 2:
  case 3:
    drawIconCloud(x, y);
    break;
  case 45:
  case 48:
    drawIconFog(x, y);
    break;
  case 51:
  case 53:
  case 55:
  case 61:
  case 63:
  case 65:
  case 80:
  case 81:
  case 82:
    drawIconRain(x, y);
    break;
  case 71:
  case 73:
  case 75:
  case 77:
  case 85:
  case 86:
    drawIconSnow(x, y);
    break;
  case 95:
    drawIconThunderstorm(x, y);
    break;
  case 96:
  case 99:
    drawIconHail(x, y);
    break;
  default:
    drawIconCloud(x, y);
    break;
  }
}
void drawWeatherHeader(unsigned long epoch) {
  time_t rawtime = (time_t)epoch - (3 * 3600); // Horário de Brasília (UTC-3)
  struct tm *ti = localtime(&rawtime);
  const char *diasSemana[] = {"Dom", "Seg", "Ter", "Qua", "Qui", "Sex", "Sab"};
  String diaHoje = diasSemana[ti->tm_wday];
  char headerBuf[50];
  sprintf(headerBuf, "%02d:%02d:%02d | %s %02d/%02d", ti->tm_hour, ti->tm_min,
          ti->tm_sec, diaHoje.c_str(), ti->tm_mday, ti->tm_mon + 1);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawCentreString(headerBuf, 120, 10, 2);
}
void drawWeatherScreen(unsigned long epoch) {
  tft.fillScreen(TFT_BLACK);
  // Desenha o Ícone do Clima baseado nas condições
  drawWeatherIcon(weather.code, weather.windSpeed, 90, 45, epoch);
  time_t rawtime = (time_t)epoch - (3 * 3600);
  struct tm *ti = localtime(&rawtime);
  bool isNight = (ti->tm_hour >= 18 || ti->tm_hour < 6);
  // Escrita pequena descrevendo a fase da lua se for noite
  if (isNight) {
    bool isBloodMoon = false;
    String faseLua = obterFaseLua(epoch, isBloodMoon);
    tft.setTextColor(isBloodMoon ? TFT_RED : tft.color565(80, 160, 255),
                     TFT_BLACK);
    tft.drawCentreString(faseLua, 120, 92, 2);
  }
  // Temperatura em destaque abaixo do ícone
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawCentreString(String(weather.temp, 1) + " C", 120, 105, 6);
  // Condição do Céu
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawCentreString(weather.main, 120, 165, 4);
  // Velocidade do Vento
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawCentreString("Velocidade do Vento", 120, 205, 2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawCentreString(String(weather.windSpeed, 1) + " km/h", 120, 225, 4);
  // Classificacao do Vento (Muda de cor se for perigoso)
  uint16_t corVento = TFT_GREEN;
  if (weather.windSpeed > 40)
    corVento = TFT_ORANGE;
  if (weather.windSpeed > 70)
    corVento = TFT_RED;
  tft.setTextColor(corVento, TFT_BLACK);
  tft.drawCentreString(weather.windDesc, 120, 260, 4);
  // Exibe a estação do ano embaixo do vento
  String estacao = obterEstacao(ti->tm_mon + 1, ti->tm_mday);
  tft.setTextColor(tft.color565(180, 80, 255), TFT_BLACK);
  tft.drawCentreString("Estacao: " + estacao, 120, 290, 2);
}
void drawWiFiScreen() {
  tft.fillScreen(TFT_BLACK); // Fundo preto para destaque
  QRCode qrcode;
  // Versão 4 suporta o logo central com segurança
  uint8_t qData[qrcode_getBufferSize(4)];
  String wifiPayload = "WIFI:S:" + cfgSSID + ";T:WPA;P:" + cfgPASS + ";;";
  // Inicializa Versão 4, Nível de correção 2 (Q - Quartile)
  // O nível Q é melhor para quando colocamos logos no centro.
  qrcode_initText(&qrcode, qData, 4, 2, wifiPayload.c_str());
  // Ajuste de escala (6 é o ideal para 320x240)
  int esc = 6;
  int qSize = qrcode.size * esc;
  int xOff = (tft.width() - qSize) / 2;
  int yOff = 10;
  // 1. Desenha o fundo branco para leitura do sensor da câmera
  tft.fillRect(xOff - 8, yOff - 8, qSize + 16, qSize + 16, TFT_WHITE);
  // 2. Desenha os módulos pretos do QR Code
  for (uint8_t y = 0; y < qrcode.size; y++) {
    for (uint8_t x = 0; x < qrcode.size; x++) {
      if (qrcode_getModule(&qrcode, x, y)) {
        tft.fillRect(xOff + (x * esc), yOff + (y * esc), esc, esc, TFT_BLACK);
      }
    }
  }
  // 3. DESENHO DO LOGO CREEPER (PROPORCIONAL 8x8)
  int cSize = 48; // Múltiplo de 8 (48 / 8 = 6 pixels por unidade 'p')
  int cx = xOff + (qSize / 2) - (cSize / 2);
  int cy = yOff + (qSize / 2) - (cSize / 2);
  // Limpa a área central para o logo
  tft.fillRect(cx - 2, cy - 2, cSize + 4, cSize + 4, TFT_WHITE);
  int p = cSize / 8; // Unidade básica de 6 pixels
  // Olhos (2x2 unidades)
  tft.fillRect(cx + (1 * p), cy + (1 * p), 2 * p, 2 * p, TFT_BLACK); // Esq
  tft.fillRect(cx + (5 * p), cy + (1 * p), 2 * p, 2 * p, TFT_BLACK); // Dir
  // Nariz (2x3 unidades)
  tft.fillRect(cx + (3 * p), cy + (3 * p), 2 * p, 3 * p, TFT_BLACK);
  // Boca/Bigode (Lados - 2x3 unidades cada)
  tft.fillRect(cx + (2 * p), cy + (4 * p), p, 3 * p, TFT_BLACK); // Canto Esq
  tft.fillRect(cx + (5 * p), cy + (4 * p), p, 3 * p, TFT_BLACK); // Canto Dir
  // O QUEIXO BRANCO (Espaço central sob o nariz)
  // Isso cria o "vão" que você pediu, deixando o queixo livre
  tft.fillRect(cx + (3 * p), cy + (6 * p), 2 * p, 2 * p, TFT_WHITE);
  // 4. INFORMAÇÕES DE TEXTO
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  // Usando tft.width()/2 para garantir centralização independente da rotação
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawCentreString("REDE: " + cfgSSID, tft.width() / 2, 225, 2);
  tft.setTextColor(TFT_GREEN,
                   TFT_BLACK); // Mudei para verde para destacar a chave
  tft.drawCentreString("SENHA: " + cfgPASS, tft.width() / 2, 280, 2);
  if (updateDisponivel) {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.drawCentreString("UPDATE DISPONIVEL: v" + versaoNova, 120, 290, 2);
  }
}
void drawPixScreen() {
  tft.fillScreen(TFT_BLACK);
  QRCode qrcode;
  uint8_t qData[qrcode_getBufferSize(4)];
  // Inicia o QR com a variável dinâmica
  qrcode_initText(&qrcode, qData, 4, 2, cfgPIX.c_str());
  int esc = 6;
  int qSize = qrcode.size * esc;
  int xOff = (tft.width() - qSize) / 2;
  int yOff = 15;
  // Fundo branco do QR
  tft.fillRect(xOff - 10, yOff - 10, qSize + 20, qSize + 20, TFT_WHITE);
  // Desenha o QR
  for (uint8_t y = 0; y < qrcode.size; y++) {
    for (uint8_t x = 0; x < qrcode.size; x++) {
      if (qrcode_getModule(&qrcode, x, y)) {
        tft.fillRect(xOff + (x * esc), yOff + (y * esc), esc, esc, TFT_BLACK);
      }
    }
  }
  // --- LOGO DO PORCO (Centralizado no QR) ---
  int cSize = 48;
  int cx = xOff + (qSize / 2) - (cSize / 2);
  int cy = yOff + (qSize / 2) - (cSize / 2);
  int p = cSize / 8;
  tft.fillRect(cx - 2, cy - 2, cSize + 4, cSize + 4, TFT_WHITE);
  uint16_t ROSA = tft.color565(255, 180, 190);
  uint16_t FOCINHO = tft.color565(255, 120, 160);
  tft.fillRect(cx, cy, cSize, cSize, ROSA);
  tft.fillRect(cx, cy + 2 * p, p, p, TFT_BLACK);
  tft.fillRect(cx + p, cy + 2 * p, p, p, TFT_WHITE);
  tft.fillRect(cx + 6 * p, cy + 2 * p, p, p, TFT_WHITE);
  tft.fillRect(cx + 7 * p, cy + 2 * p, p, p, TFT_BLACK);
  tft.fillRect(cx + 2 * p, cy + 4 * p, 4 * p, 2 * p, FOCINHO);
  // --- AJUSTE DE TEXTO (Mais para baixo) ---
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  // Subi um pouco o título para não amontoar
  tft.drawCentreString("PAGAR VIA PIX", tft.width() / 2, 250, 4);
  // A chave PIX agora fica em 215 (antes era 225/rodape)
  // Isso deixa um respiro de 5-10 pixels da borda física
  int fonteChave = (cfgPIX.length() > 20) ? 1 : 2;
  tft.setTextColor(TFT_GREEN,
                   TFT_BLACK); // Mudei para verde para destacar a chave
  tft.drawCentreString(cfgPIX, tft.width() / 2, 280, fonteChave);
}
void drawWiserScreen() {
  tft.fillScreen(TFT_BLACK);
  QRCode qrcode;
  // Aumentamos o buffer e a versão (de 4 para 5)
  // para garantir que a URL do Wise caiba sem erros.
  uint8_t qData[qrcode_getBufferSize(5)];
  // Inicia o QR com a variável do Wise
  qrcode_initText(&qrcode, qData, 5, 2, cfgWiser.c_str());
  int esc = 6;
  int qSize = qrcode.size * esc;
  int xOff = (tft.width() - qSize) / 2;
  int yOff = 15;
  // Fundo branco do QR
  tft.fillRect(xOff - 10, yOff - 10, qSize + 20, qSize + 20, TFT_WHITE);
  // Cor Verde Escuro do Wise baseada na imagem de referência
  uint16_t WISE_GREEN = tft.color565(20, 80, 28);
  // Desenha o QR
  for (uint8_t y = 0; y < qrcode.size; y++) {
    for (uint8_t x = 0; x < qrcode.size; x++) {
      if (qrcode_getModule(&qrcode, x, y)) {
        // Preenche com o verde escuro em vez de preto
        tft.fillRect(xOff + (x * esc), yOff + (y * esc), esc, esc, WISE_GREEN);
      }
    }
  }
  // --- LOGO DA WISE (Centralizado no QR) ---
  int cSize = 48;
  int cx = xOff + (qSize / 2) - (cSize / 2);
  int cy = yOff + (qSize / 2) - (cSize / 2);
  // Quadrado de limpeza (fundo branco para dar o respiro do logo)
  tft.fillRect(cx - 2, cy - 2, cSize + 4, cSize + 4, TFT_WHITE);
  // Círculo base do logo (Verde Escuro)
  tft.fillCircle(cx + (cSize / 2), cy + (cSize / 2), cSize / 2, WISE_GREEN);
  // Desenhando o Símbolo Branco da Wise (Bandeira/Raio) usando triângulos
  // Parte Superior
  tft.fillTriangle(cx + 14, cy + 22, cx + 32, cy + 14, cx + 26, cy + 26,
                   TFT_WHITE);
  // Parte Inferior
  tft.fillTriangle(cx + 20, cy + 36, cx + 28, cy + 24, cx + 22, cy + 24,
                   TFT_WHITE);
  // --- AJUSTE DE TEXTO (Rodapé) ---
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  // Título atualizado
  tft.drawCentreString("PAGAR VIA WISE", tft.width() / 2, 250, 4);
  // A fonte se ajusta conforme o tamanho do link
  int fonteChave = (cfgWiser.length() > 20) ? 1 : 2;
  // Cor do link usa o mesmo verde da marca para consistência
  tft.setTextColor(WISE_GREEN, TFT_BLACK);
  tft.drawCentreString(cfgWiser, tft.width() / 2, 280, fonteChave);
}
// --- Gestão de Arquivos ---
void salvarConfig() {
  File f = SD.open("/config.txt", FILE_WRITE);
  if (f) {
    f.println("SSID=" + cfgSSID);
    f.println("PASS=" + cfgPASS);
    f.println("MODO=" + cfgMODO);
    f.println("IP_ALVO=" + cfgIP);
    f.println("PIX=" + cfgPIX); // ADICIONE ESTA LINHA
    f.println("PIX=" + cfgPIX);
    f.println("WISER=" + cfgWiser);
    f.close();
  }
}
void carregarTudo() {
  if (!SD.begin(SD_CS))
    return;
  if (SD.exists("/config.txt")) {
    File f = SD.open("/config.txt", FILE_READ);
    while (f.available()) {
      String line = f.readStringUntil('\n');
      line.trim();
      if (line.startsWith("SSID="))
        cfgSSID = line.substring(5);
      else if (line.startsWith("PASS="))
        cfgPASS = line.substring(5);
      else if (line.startsWith("MODO="))
        cfgMODO = line.substring(5);
      else if (line.startsWith("IP_ALVO="))
        cfgIP = line.substring(8);
      else if (line.startsWith("PIX="))
        cfgPIX = line.substring(4);
      else if (line.startsWith("WISER="))
        cfgWiser = line.substring(6);
    }
    f.close();
  }
  accounts.clear();
  File f2 = SD.open("/totp_secrets.txt", FILE_READ);
  if (f2) {
    while (f2.available()) {
      String line = f2.readStringUntil('\n');
      line.trim();
      int e1 = line.indexOf('=');
      int e2 = line.indexOf('=', e1 + 1);
      if (e1 > 0 && e2 > 0)
        accounts.push_back({line.substring(0, e1), line.substring(e1 + 1, e2),
                            line.substring(e2 + 1)});
    }
    f2.close();
  }
  seeds.clear();
  File fs = SD.open("/seeds.txt", FILE_READ);
  if (fs) {
    while (fs.available()) {
      String line = fs.readStringUntil('\n');
      line.trim();
      int p = line.indexOf('|');
      if (p > 0)
        seeds.push_back({line.substring(0, p), line.substring(p + 1)});
    }
    fs.close();
  }
}
void drawSpiderJockey(int x, int y, int tam) {
  int p = tam / 10; // Unidade de pixel
  // Corpo da Aranha (Marrom escuro)
  uint16_t MARROM = tft.color565(60, 40, 30);
  tft.fillRect(x, y + 5 * p, tam, 4 * p, MARROM);           // Abdômen
  tft.fillRect(x + 2 * p, y + 4 * p, 4 * p, 3 * p, MARROM); // Cabeça da aranha
  // Olhos Vermelhos da Aranha
  tft.fillRect(x + 3 * p, y + 5 * p, 1, 1, TFT_RED);
  tft.fillRect(x + 5 * p, y + 5 * p, 1, 1, TFT_RED);
  // Pernas da Aranha
  for (int i = 0; i < 4; i++) {
    tft.drawLine(x + 2 * p, y + 6 * p, x - 2 * p, y + 4 * p + (i * 2),
                 MARROM); // Esquerda
    tft.drawLine(x + 6 * p, y + 6 * p, x + tam, y + 4 * p + (i * 2),
                 MARROM); // Direita
  }
  // Esqueleto (Cinza claro)
  uint16_t CINZA = tft.color565(200, 200, 200);
  tft.fillRect(x + 3 * p, y, 3 * p, 3 * p, CINZA);     // Cabeça
  tft.fillRect(x + 4 * p, y + 3 * p, p, 3 * p, CINZA); // Coluna/Corpo
  // Olhos do Esqueleto
  tft.fillRect(x + 3 * p + 1, y + 1, 1, 1, TFT_BLACK);
  tft.fillRect(x + 5 * p - 1, y + 1, 1, 1, TFT_BLACK);
  // Arco (Amarelo queimado)
  tft.drawCircle(x + 6 * p, y + 3 * p, 4, tft.color565(150, 120, 50));
}
// --- TOTP Lógica ---
int base32CharToVal(char c) {
  if (c >= 'A' && c <= 'Z')
    return c - 'A';
  if (c >= '2' && c <= '7')
    return 26 + (c - '2');
  return -1;
}
int base32Decode(const String &input, uint8_t *output, int maxOut) {
  String s = input;
  s.toUpperCase();
  s.replace(" ", "");
  int buffer = 0, bitsLeft = 0, outCount = 0;
  for (size_t i = 0; i < s.length(); i++) {
    int val = base32CharToVal(s[i]);
    if (val < 0)
      continue;
    buffer <<= 5;
    buffer |= val & 0x1F;
    bitsLeft += 5;
    if (bitsLeft >= 8) {
      bitsLeft -= 8;
      if (outCount < maxOut)
        output[outCount++] = (buffer >> bitsLeft) & 0xFF;
    }
  }
  return outCount;
}
String calcTOTP(const String &secret, unsigned long epoch) {
  unsigned long counter = epoch / 30;
  uint8_t msg[8];
  for (int i = 7; i >= 0; i--) {
    msg[i] = counter & 0xFF;
    counter >>= 8;
  }
  uint8_t key[64];
  int keyLen = base32Decode(secret, key, sizeof(key));
  if (keyLen <= 0)
    return "ERRO";
  uint8_t hash[20];
  mbedtls_md_hmac(mbedtls_md_info_from_type(MBEDTLS_MD_SHA1), key, keyLen, msg,
                  8, hash);
  int offset = hash[19] & 0x0F;
  uint32_t bin_code =
      ((hash[offset] & 0x7F) << 24) | ((hash[offset + 1] & 0xFF) << 16) |
      ((hash[offset + 2] & 0xFF) << 8) | (hash[offset + 3] & 0xFF);
  char buf[7];
  snprintf(buf, sizeof(buf), "%06d", bin_code % 1000000);
  return String(buf);
}
// --- Interface Visor Atualizada ---
void drawCreeper() {
  // Centraliza o rosto: x=60, y=40, tamanho=120
  // Isso mantém a simetria com o resto das informações na tela
  drawCustomCreeper(60, 40, 120);
}
void drawCustomCreeper(int x, int y, int tam) {
  int pixelSize = tam / 12; // Se tam for 120, o pixel será 10x10
  // Desenha o fundo verde (Base)
  tft.fillRect(x, y, tam, tam, TFT_GREEN);
  // Pixels Pretos (O desenho exato da sua Web)
  // Olhos
  tft.fillRect(x + 30, y + 10, pixelSize, pixelSize, TFT_BLACK);
  tft.fillRect(x + 40, y + 10, pixelSize, pixelSize, TFT_BLACK);
  tft.fillRect(x + 70, y + 10, pixelSize, pixelSize, TFT_BLACK);
  tft.fillRect(x + 80, y + 10, pixelSize, pixelSize, TFT_BLACK);
  tft.fillRect(x + 20, y + 20, pixelSize, pixelSize, TFT_BLACK);
  tft.fillRect(x + 30, y + 20, pixelSize, pixelSize, TFT_BLACK);
  tft.fillRect(x + 40, y + 20, pixelSize, pixelSize, TFT_BLACK);
  tft.fillRect(x + 70, y + 20, pixelSize, pixelSize, TFT_BLACK);
  tft.fillRect(x + 80, y + 20, pixelSize, pixelSize, TFT_BLACK);
  tft.fillRect(x + 90, y + 20, pixelSize, pixelSize, TFT_BLACK);
  tft.fillRect(x + 20, y + 30, pixelSize, pixelSize, TFT_BLACK);
  tft.fillRect(x + 30, y + 30, pixelSize, pixelSize, TFT_BLACK);
  tft.fillRect(x + 40, y + 30, pixelSize, pixelSize, TFT_BLACK);
  tft.fillRect(x + 70, y + 30, pixelSize, pixelSize, TFT_BLACK);
  tft.fillRect(x + 80, y + 30, pixelSize, pixelSize, TFT_BLACK);
  tft.fillRect(x + 90, y + 30, pixelSize, pixelSize, TFT_BLACK);
  // Nariz/Ponte
  tft.fillRect(x + 50, y + 50, pixelSize, pixelSize, TFT_BLACK);
  tft.fillRect(x + 60, y + 50, pixelSize, pixelSize, TFT_BLACK);
  // Boca (Parte superior e meio)
  for (int i = 60; i <= 80; i += 10) {
    tft.fillRect(x + 30, y + i, pixelSize, pixelSize, TFT_BLACK);
    tft.fillRect(x + 40, y + i, pixelSize, pixelSize, TFT_BLACK);
    tft.fillRect(x + 50, y + i, pixelSize, pixelSize, TFT_BLACK);
    tft.fillRect(x + 60, y + i, pixelSize, pixelSize, TFT_BLACK);
    tft.fillRect(x + 70, y + i, pixelSize, pixelSize, TFT_BLACK);
    tft.fillRect(x + 80, y + i, pixelSize, pixelSize, TFT_BLACK);
  }
  // "Pés" da boca (Laterais inferiores)
  tft.fillRect(x + 30, y + 90, pixelSize, pixelSize, TFT_BLACK);
  tft.fillRect(x + 40, y + 90, pixelSize, pixelSize, TFT_BLACK);
  tft.fillRect(x + 70, y + 90, pixelSize, pixelSize, TFT_BLACK);
  tft.fillRect(x + 80, y + 90, pixelSize, pixelSize, TFT_BLACK);
  tft.fillRect(x + 30, y + 100, pixelSize, pixelSize, TFT_BLACK);
  tft.fillRect(x + 40, y + 100, pixelSize, pixelSize, TFT_BLACK);
  tft.fillRect(x + 70, y + 100, pixelSize, pixelSize, TFT_BLACK);
  tft.fillRect(x + 80, y + 100, pixelSize, pixelSize, TFT_BLACK);
}
void drawInfo(unsigned long epoch) {
  // Ajuste para Horário de Brasília (UTC-3)
  // Como você está usando gmtime, subtraímos 3 horas (3 * 3600 segundos)
  time_t rawtime = (time_t)epoch - (3 * 3600);
  struct tm *ti = localtime(&rawtime); // Usamos localtime após o ajuste
  char f_time[30];
  char f_date[30];
  // 1. Nomes dos dias da semana
  const char *diasSemana[] = {"Domingo", "Segunda", "Terca", "Quarta",
                              "Quinta",  "Sexta",   "Sabado"};
  String diaHoje = diasSemana[ti->tm_wday];
  // 2. Lógica AM/PM
  int hora = ti->tm_hour;
  String sufixo = (hora >= 12) ? "PM" : "AM";
  // Converte formato 24h para 12h
  if (hora == 0)
    hora = 12; // Meia-noite vira 12 AM
  else if (hora > 12)
    hora -= 12; // 13h vira 1 PM
  // 3. Formata as Strings
  // Data e Dia da Semana: "Sabado - 14/01"
  String diaFormatado = diaHoje;
  if (ti->tm_wday >= 1 && ti->tm_wday <= 5) {
    diaFormatado += "-feira";
  }
  // 2. Agora usamos a variável diaFormatado no sprintf
  sprintf(f_date, "%s - %02d/%02d/%d", diaFormatado.c_str(), ti->tm_mday,
          ti->tm_mon + 1, ti->tm_year + 1900);
  // Hora com segundos: "04:45:30 PM"
  sprintf(f_time, "%02d:%02d:%02d %s", hora, ti->tm_min, ti->tm_sec,
          sufixo.c_str());
  // --- DESENHO NO TFT ---
  tft.setTextColor(TFT_MAGENTA, TFT_BLACK);
  // Desenha a Data e Dia da Semana em cima (fonte menor)
  tft.drawCentreString(f_date, 120, 2, 2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  // Desenha a Hora AM/PM com Segundos (fonte maior 4)
  tft.drawCentreString(f_time, 120, 200, 4);
  // Mostra o IP em Verde Matrix
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawCentreString(WiFi.localIP().toString(), 120, 246, 2);
  // Mostra a condição do tempo
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawCentreString(weather.main, 120, 280, 4);
}
void desligarTela() {
  digitalWrite(TFT_BL, LOW); // Apaga os LEDs de fundo
  digitalWrite(PIN_RED, LOW);
  digitalWrite(PIN_GREEN, HIGH);
  digitalWrite(PIN_BLUE, HIGH);
}
void ligarTela() {
  digitalWrite(TFT_BL, HIGH); // Acende os LEDs de fundo
  digitalWrite(PIN_GREEN, LOW);
}
// --- Setup ---
void setup() {
  Serial.begin(115200);
  // Configura o pino de Backlight como saída
  pinMode(TFT_BL, OUTPUT);
  // Configura os pinos como saída
  pinMode(PIN_RED, OUTPUT);
  pinMode(PIN_GREEN, OUTPUT);
  pinMode(PIN_BLUE, OUTPUT);
  // Inicia com todos os LEDs DESLIGADOS (HIGH = Desligado)
  digitalWrite(PIN_RED, HIGH);
  digitalWrite(PIN_GREEN, HIGH);
  digitalWrite(PIN_BLUE, HIGH);
  SPI.setFrequency(20000000);
  SPI.begin(18, 19, 23, SD_CS);
  tft.init();
  tft.setRotation(0);
  tft.invertDisplay(false); // Inverte as cores da tela conforme solicitado
  tft.fillScreen(TFT_BLACK);
  // INICIA O TOUCH (DEPOIS DO TFT)
  touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  touchscreen.begin(touchscreenSPI);
  touchscreen.setRotation(0);
  // Configuração do Decodificador de Imagem
  server.onNotFound(handleFileRead);
  TJpgDec.setJpgScale(1);
  TJpgDec.setCallback(tft_output);
  tft.setSwapBytes(false);
  // --- DENTRO DO SEU void setup() ---
  server.collectHeaders(headerkeys, headerkeyssize);
  pinMode(PINO_RELE_LUZ, OUTPUT);
  digitalWrite(PINO_RELE_LUZ, LOW); // Relé desligado
  // Define o padrão do ESP32 para Servos (Novo código)
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  servoCreeper.setPeriodHertz(50); // Frequência padrão de 50Hz
  // Configura o pino e define posição inicial
  servoCreeper.attach(PINO_SERVO, 500,
                      2400); // 500 e 2400 são os pulsos min/max padrão
  servoCreeper.write(0);     // Posição zero (cabeça fechada)
  checkUpdate();
  carregarTudo();
  WiFi.begin(cfgSSID.c_str(), cfgPASS.c_str());
  WiFi.enableIPv6();
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 10000)
    delay(500);
  // --- PRINT NO CONSOLE (Monitor Serial) ---
  Serial.println("\n--- REDE CONECTADA ---");
  Serial.print("IPv4: ");
  Serial.println(WiFi.localIP());
  // No Core 3.x, usamos linkLocalIPv6() para o endereço fe80::
  Serial.print("IPv6: ");
  Serial.println(WiFi.linkLocalIPv6());
  Serial.println("-------------------------");
  Serial.println("FTP PORTA 21 User: creeper, Pass: k9R7xM2pQ4vL8wT5");
  Serial.println("ftp://creeper:k9R7xM2pQ4vL8wT5@192.168.100.49/");
  Serial.println("-------------------------");
  // No setup, após conectar no Wi-Fi:
  if (MDNS.begin("steve")) {
    Serial.println("MDNS responder iniciado: http://steve.local");
  }
  timeClient.begin();
  udpWhitelist.begin(1234);
  udpPC.begin(portPC);
  auto ehMickey = []() {
    String clientIP = server.client().remoteIP().toString();
    // 1. Limpa espaços que podem vir do config.txt
    String cleanCfg = cfgIP;
    cleanCfg.replace(" ", "");
    // 2. LOG NO SERIAL: Isso vai te mostrar quem é o "intruso"
    Serial.print("Tentativa de login de: [");
    Serial.print(clientIP);
    Serial.println("]");
    // 3. Verificação na Whitelist Dinâmica (Python)
    if (dynamicWhitelist.indexOf(clientIP) >= 0) {
      Serial.println("Acesso Liberado: Whitelist Dinamica");
      return true;
    }
    // 4. Verificação na Lista Fixa do SD (Aceita vírgulas)
    if (cleanCfg.indexOf(clientIP) >= 0) {
      Serial.println("Acesso Liberado: Lista Fixa SD");
      return true;
    }
    // 5. Verificação de Prefixo (Modo REDE)
    if (cfgMODO == "REDE" && clientIP.startsWith(cfgIP)) {
      Serial.println("Acesso Liberado: Prefixo de Rede");
      return true;
    }
    // 6. IPv6 na Whitelist
    if (clientIP.startsWith("fe80")) {
      Serial.println("Acesso Liberado: Link-Local IPv6 (Mickey)");
      return true;
    }
    Serial.println("!!! ACESSO NEGADO !!!");
    return false;
  };
  String css = R"rawliteral(
<style>
  body{background:#000;color:#0f0;font-family:monospace;text-align:center;margin:0;}
  footer{margin-top:40px;font-size:0.85em;color:#666;}
  
  /* Box Responsiva */
  .box {
    border: 2px solid #0f0;
    padding: 20px;
    display: inline-block;
    margin-top: 20px;
    width: 90%;
    max-width: 360px;
    box-sizing: border-box;
  }
  /* Rosto do Creeper */
#creeper-container {
  width: 120px;
  height: 120px;
  position: relative;
  margin: 20px auto;
  display: block;
}
#creeper-body {
  width: 100%;
  height: 100%;
  background-color: #0f0; /* Verde Matrix */
  position: absolute;
  top: 0;
  left: 0;
  z-index: 1; /* Fica no fundo */
}
.pixel {
  background-color: #000;
  position: absolute;
  width: 10px;
  height: 10px;
  z-index: 2; /* Fica na frente do verde */
}
  /* Botões e Inputs */
  a{background:#000;color:#0f0;text-decoration:none;border:1px solid #0f0;padding:5px;margin:3px;display:inline-block;transition:0.3s;}
  a:hover{background:#0f0;color:#000;box-shadow:0 0 10px #0f0;}
  input,select{background:#111;color:#0f0;border:1px solid #0f0;padding:8px;width:100%;margin:5px 0;box-sizing:border-box;}
  
  .edit{color:#ff0;border-color:#ff0;}
  .del{color:#f00;border-color:#f00;}
  .inverted { background: #0f0 !important; color: #000 !important; }
</style>
)rawliteral";
  // --- Rotas ---
  server.on("/", [css, ehMickey]() {
    // Cabeçalho e CSS
    String h =
        "<!DOCTYPE html><html lang='pt'><head><link rel='shortcut icon' "
        "type='image/x-icon' href='http://" +
        WiFi.localIP().toString() +
        "/favicon.ico'><meta name='viewport' content='width=device-width, "
        "initial-scale=1.0'><meta charset='UTF-8'>" +
        css + "</head><body>";
    h += "<div class='box'>";
    // --- SEU NOVO BLOCO DO CREEPER ---
    h += "<div id='creeper-container'>";
    h += "<div id='creeper-body'></div>";
    h += "<div class='pixel' style='left: 30px; top: 10px;'></div><div "
         "class='pixel' style='left: 40px; top: 10px;'></div><div "
         "class='pixel' style='left: 70px; top: 10px;'></div><div "
         "class='pixel' style='left: 80px; top: 10px;'></div>";
    h +=
        "<div class='pixel' style='left: 20px; top: 20px;'></div><div "
        "class='pixel' style='left: 30px; top: 20px;'></div><div class='pixel' "
        "style='left: 40px; top: 20px;'></div><div class='pixel' style='left: "
        "70px; top: 20px;'></div><div class='pixel' style='left: 80px; top: "
        "20px;'></div><div class='pixel' style='left: 90px; top: 20px;'></div>";
    h +=
        "<div class='pixel' style='left: 20px; top: 30px;'></div><div "
        "class='pixel' style='left: 30px; top: 30px;'></div><div class='pixel' "
        "style='left: 40px; top: 30px;'></div><div class='pixel' style='left: "
        "70px; top: 30px;'></div><div class='pixel' style='left: 80px; top: "
        "30px;'></div><div class='pixel' style='left: 90px; top: 30px;'></div>";
    h += "<div class='pixel' style='left: 50px; top: 50px;'></div><div "
         "class='pixel' style='left: 60px; top: 50px;'></div>";
    h +=
        "<div class='pixel' style='left: 30px; top: 60px;'></div><div "
        "class='pixel' style='left: 40px; top: 60px;'></div><div class='pixel' "
        "style='left: 50px; top: 60px;'></div><div class='pixel' style='left: "
        "60px; top: 60px;'></div><div class='pixel' style='left: 70px; top: "
        "60px;'></div><div class='pixel' style='left: 80px; top: 60px;'></div>";
    h +=
        "<div class='pixel' style='left: 30px; top: 70px;'></div><div "
        "class='pixel' style='left: 40px; top: 70px;'></div><div class='pixel' "
        "style='left: 50px; top: 70px;'></div><div class='pixel' style='left: "
        "60px; top: 70px;'></div><div class='pixel' style='left: 70px; top: "
        "70px;'></div><div class='pixel' style='left: 80px; top: 70px;'></div>";
    h +=
        "<div class='pixel' style='left: 30px; top: 80px;'></div><div "
        "class='pixel' style='left: 40px; top: 80px;'></div><div class='pixel' "
        "style='left: 50px; top: 80px;'></div><div class='pixel' style='left: "
        "60px; top: 80px;'></div><div class='pixel' style='left: 70px; top: "
        "80px;'></div><div class='pixel' style='left: 80px; top: 80px;'></div>";
    h += "<div class='pixel' style='left: 30px; top: 90px;'></div><div "
         "class='pixel' style='left: 40px; top: 90px;'></div><div "
         "class='pixel' style='left: 70px; top: 90px;'></div><div "
         "class='pixel' style='left: 80px; top: 90px;'></div>";
    h += "<div class='pixel' style='left: 30px; top: 100px;'></div><div "
         "class='pixel' style='left: 40px; top: 100px;'></div><div "
         "class='pixel' style='left: 70px; top: 100px;'></div><div "
         "class='pixel' style='left: 80px; top: 100px;'></div>";
    h += "</div>";
    // ------------------------------------
    if (tlsAtivado) {
      h +=
          "<p style='color:#0f0; font-size:0.8em;'>🔒 CONEXÃO SEGURA (TLS)</p>";
    } else {
      h +=
          "<p style='color:#666; font-size:0.8em;'>🔓 MODO INTRANET (HTTP)</p>";
    }
    h += "<h2>CREEPER AUTH v7.2.2</h2>";
    // --- NOVO BLOCO: CONTROLO DO VISOR FÍSICO ---
    // h += "<div style='border:1px solid #444; padding:10px;
    // margin-bottom:15px;'>"; h += "<p>VISOR DO DISPOSITIVO:</p>"; h += "<a
    // href='https://home.openweathermap.org/api_keys' class='edit'>Api
    // Meteorologica</a> "; h += "</div>";
    // --------------------------------------------
    //    h += "<form action='/select'>";
    //--------------------novo
    h += "<div>";
    h += "<select id='idVisor' name='id'>";
    h += "  <option value='-1'>VISOR CREEPER</option>";
    h += "  <option value='-2'>VISOR: QR CODE WIFI</option>";
    h += "  <option value='-3'>VISOR: QR CODE PIX</option>";
    h += "  <option value='-4'>VISOR: METEOROLOGIA (API)</option>";
    h += "  <option value='-5'>VISOR: PERFORMANCE PC</option>";
    h += "  <option value='-6'>VISOR: QR CODE WISER</option>";
    h += "  <option value='-7'>VISOR: TECLADO WI-FI TOUCH</option>";
    for (int i = 0; i < accounts.size(); i++) {
      h +=
          "<option value='" + String(i) + "'>" + accounts[i].name + "</option>";
    }
    h += "</select>";
    h += "<button onclick='mudarTela()' "
         "style='background:#000;color:#0f0;border:1px solid "
         "#0f0;padding:10px;width:100%;cursor:pointer;font-family:monospace;'>"
         "EXECUTAR COMANDO</button>";
    h += "<p id='statusMsg' style='height:20px; color:#ff0; font-size:0.9em; "
         "margin-top:10px;'></p>"; // Espaço para a mensagem
    h += "</div>";
    // JavaScript Atualizado
    h += "<script>";
    h += "function mudarTela(){";
    h += "  var sel = document.getElementById('idVisor');";
    h += "  var id = sel.value;";
    h += "  var texto = sel.options[sel.selectedIndex].text;";
    h += "  var msg = document.getElementById('statusMsg');";
    h += "  ";
    // Lógica para a mensagem personalizada
    h += "  if(id == '-1') { msg.innerHTML = '> Iniciando Rosto Creeper...'; }";
    h += "  else if(id == '-2') { msg.innerHTML = '> Gerando QR Code WiFi...'; "
         "}";
    h += "  else if(id == '-3') { msg.innerHTML = '> Chamando Pagamento "
         "PIX...'; }";
    h += "  else if(id == '-4') { msg.innerHTML = '> Consultando "
         "Meteorologia...'; }";
    h += "  else if(id == '-5') { msg.innerHTML = '> Monitorando Hardware "
         "Debian...'; }";
    h += "  else if(id == '-6') { msg.innerHTML = '> Chamando Pagamento "
         "Wiser...'; }"; // <-- ADICIONE ESTA
    h += "  else { msg.innerHTML = '> Solicitando Token: ' + texto; }";
    h += "  ";
    h += "  fetch('/select?id=' + id).then(response => {";
    h += "    if(response.ok) {";
    h += "       setTimeout(() => { msg.innerHTML = '> Comando enviado com "
         "sucesso!'; }, 500);";
    h += "       setTimeout(() => { msg.innerHTML = ''; }, 3000);"; // Limpa a
                                                                    // mensagem
                                                                    // após 3
                                                                    // segundos
    h += "    }";
    h += "  });";
    h += "}";
    h += "</script>";
    //--------------------novo
    // h += "</select><input type='submit' value='EXIBIR NO VISOR'></form><br>";
    if (ehMickey()) {
      h += "<div style='display: grid; grid-template-columns: 1fr 1fr; gap: "
           "10px;'>";
      h +=
          "  <a href='/v.html' style='display:block; background-color:#bb86fc; "
          "color:#000; padding:10px; text-decoration:none; border-radius:4px; "
          "font-weight:bold; text-align:center;'>🎬 VÍDEOS</a>";
      h += "  <a href='/list.html' style='display:block; "
           "background-color:#03dac6; color:#000; padding:10px; "
           "text-decoration:none; border-radius:4px; font-weight:bold; "
           "text-align:center;'>📋 LISTA</a>";
      h += "  <a href='/vault' style='display:block; background-color:#2c2c2c; "
           "color:#fff; padding:10px; text-decoration:none; border-radius:4px; "
           "text-align:center;'>📁 VAULT</a>";
      h +=
          "  <a href='/manage' style='display:block; background-color:#2c2c2c; "
          "color:#fff; padding:10px; text-decoration:none; border-radius:4px; "
          "text-align:center;'>🔑 TOKENS</a>";
      h += "  <a href='/login.html' style='display:block; "
           "background-color:#2c2c2c; color:#fff; padding:10px; "
           "text-decoration:none; border-radius:4px; text-align:center; "
           "grid-column: span 2;'>🔐 LOGIN</a>";
      h += "  <a href='/network' style='display:block; "
           "background-color:#2c2c2c; color:#fff; padding:10px; "
           "text-decoration:none; border-radius:4px; text-align:center; "
           "grid-column: span 2;'>⚙️ CONFIG WI-FI & IP</a>";
      h += "</div>";
    } else {
      h += "<div style='text-align:center;'>";
      h += "  <p style='color:#cf6679; font-weight:bold;'>ACESSO NEGADO: IP "
           "PROTEGIDO</p>";
      h += "  <a href='/login.html' style='display:inline-block; "
           "background-color:#bb86fc; color:#000; padding:10px 20px; "
           "text-decoration:none; border-radius:4px; font-weight:bold;'>🔐 IR "
           "PARA LOGIN</a>";
      h += "</div>";
    }
    h += "</div>";    // Fecha a div box
    h += getFooter(); // CHAMA A FUNÇÃO AQUI
    if (updateDisponivel) {
      h += "<div style='background:#330; border:1px solid #ff0; color:#ff0; "
           "padding:10px; margin:10px 0; text-align:center;'>";
      h += "📢 <b>Nova versão disponível!</b> (v" + versaoNova + ")<br>";
      h +=
          "<a href='https://github.com/Annabel369/2FATouch' style='color:#fff; "
          "text-decoration:underline;'>Clique para atualizar</a>";
      h += "</div>";
    }
    h += "</body></html>";
    server.send(200, "text/html", h);
  });
  server.on("/manage", [css, ehMickey]() {
    if (!ehMickey())
      return server.send(403, "Negado");
    if (!verificarAcesso())
      return;
    String h =
        "<!DOCTYPE html><html lang='pt'><head><link rel='shortcut icon' "
        "type='image/x-icon' href='http://" +
        WiFi.localIP().toString() +
        "/favicon.ico'>><head><meta charset='UTF-8'><meta name='viewport' "
        "content='width=device-width, initial-scale=1.0'>" +
        css + "</head><body><div class='box'><h2>GERENCIAR TOKENS</h2>";
    for (int i = 0; i < accounts.size(); i++) {
      h += "<div style='margin-bottom:10px;'>" + accounts[i].name + " <br>";
      h += "<a href='/edit?id=" + String(i) + "' class='edit'>[E] EDITAR</a> ";
      h += "<a href='/del?id=" + String(i) +
           "' class='del'>[X] EXCLUIR</a></div>";
    }
    h += "<hr><a href='/add'>+ NOVO TOKEN</a><br>"
         "<a href='/'>VOLTAR</a></div>"
         "<footer>Copyright 2025-2026 Criado por Amauri Bueno dos Santos com "
         "apoio da Gemini. "
         "<a href='https://github.com/Annabel369/2FATouch' target='_blank' "
         "style='color:#bb86fc;'>GitHub</a></footer>"
         "</body></html>";
    server.send(200, "text/html", h);
  });
  server.on("/exibir", []() {
    String modo = server.arg("modo");
    currentIndex = -1;
    currentSeedIndex = -1;
    if (modo == "WIFI") {
      displayMode = 1; // QR WiFi
    } else if (modo == "PIX") {
      displayMode = 2; // QR PIX
    } else if (modo == "CLIMA") {
      displayMode = 4;            // Nova tela de Meteorologia
      carregarTelaMeteorologia(); // Busca os dados com animação de progresso
    } else if (modo == "WISE") {
      displayMode = 6; // QR WISE
    } else {
      displayMode = 0; // Volta para o Rosto do Creeper
    }
    forceRedraw = true;
    server.send(200, "text/html", "Modo " + modo + " Ativado no Visor");
  });
  // --- NOVA ROTA: FORMULÁRIO PARA ADICIONAR ---
  server.on("/add", [css, ehMickey]() {
    if (!ehMickey())
      return server.send(403, "Negado");
    if (!verificarAcesso())
      return;
    String h =
        "<!DOCTYPE html><html lang='pt'><head><link rel='shortcut icon' "
        "type='image/x-icon' href='http://" +
        WiFi.localIP().toString() +
        "/favicon.ico'><head><meta charset='UTF-8'><meta name='viewport' "
        "content='width=device-width, initial-scale=1.0'>" +
        css +
        "</head><body><div class='box'><h2>NOVO TOKEN</h2><form method='POST' "
        "action='/reg'>";
    h += "NOME (Ex: Discord):<input name='u'>SECRET (Base32):<input "
         "name='s'>SENHA (Opcional):<input name='p'><input type='submit' "
         "value='CRIAR TOKEN'></form><br><a "
         "href='/manage'>VOLTAR</a></div></body></html>";
    server.send(200, "text/html", h);
  });
  // --- NOVA ROTA: REGISTRAR NO SD ---
  server.on("/reg", HTTP_POST, [ehMickey]() {
    if (ehMickey()) {
      String s = server.arg("s");
      s.toUpperCase();
      s.replace(" ", "");
      accounts.push_back({server.arg("u"), s, server.arg("p")});
      File f = SD.open("/totp_secrets.txt", FILE_APPEND);
      if (f) {
        f.println(server.arg("u") + "=" + s + "=" + server.arg("p"));
        f.close();
      }
      server.send(200, "text/html",
                  "<script>location.href='/manage';</script>");
    }
  });
  server.on("/edit", [css, ehMickey]() {
    if (!ehMickey())
      return server.send(403, "Negado");
    int id = server.arg("id").toInt();
    TotpAccount acc = accounts[id];
    String h =
        "<!DOCTYPE html><html lang='pt'><head><link rel='shortcut icon' "
        "type='image/x-icon' href='http://" +
        WiFi.localIP().toString() +
        "/favicon.ico'><head><meta charset='UTF-8'><meta name='viewport' "
        "content='width=device-width, initial-scale=1.0'>" +
        css +
        "</head><body><div class='box'><h2>EDITAR TOKEN</h2><form "
        "method='POST' action='/update?id=" +
        String(id) + "'>'";
    h += "NOME:<input name='u' value='" + acc.name +
         "'>SECRET:<input name='s' value='" + acc.secretBase32 +
         "'>PASS:<input name='p' value='" + acc.password +
         "'><input type='submit' value='SALVAR "
         "ALTERAÇÕES'></form></div></body></html>";
    server.send(200, "text/html", h);
  });
  server.on("/update", HTTP_POST, [ehMickey]() {
    if (ehMickey()) {
      int id = server.arg("id").toInt();
      String s = server.arg("s");
      s.toUpperCase();
      s.replace(" ", "");
      accounts[id] = {server.arg("u"), s, server.arg("p")};
      SD.remove("/totp_secrets.txt");
      File f = SD.open("/totp_secrets.txt", FILE_WRITE);
      for (auto const &a : accounts)
        f.println(a.name + "=" + a.secretBase32 + "=" + a.password);
      f.close();
      forceRedraw = true;
      server.send(200, "text/html",
                  "<script>location.href='/manage';</script>");
    }
  });
  // REGISTRE ESTA LINHA OBRIGATORIAMENTE para ler o cabeçalho de login do
  // navegador:
  const char *headerkeys2[] = {"Authorization", "Range"};
  size_t headerkeyssize2 = sizeof(headerkeys2) / sizeof(char *);
  server.collectHeaders(headerkeys2, headerkeyssize2);
  // Registra a rota do JSON
  server.on("/json", handleListJSON);
  // Registra as rotas de Editar e Deletar
  server.on("/editXARQ", handleEditFile);
  server.on("/deleteXARQ", handleDeleteFile);
  // Registra rota de logout
  server.on("/list.html", handleListHTML);
  server.on("/v.html", handleListHTML2);
  server.on("/login.html", handleLoginRoute);
  server.on("/doLogin", HTTP_POST, handleDoLogin);
  server.on("/logout", handleLogoutCustom);
  // Rota de Upload corrigida (envia o HTTP 200 apenas após concluir)
  server.on(
      "/upload", HTTP_POST,
      []() {
        if (!verificarAcesso())
          return;
        server.send(200, "text/plain", "Upload OK");
      },
      handleUpload);
  server.on("/saveXARQ", HTTP_POST, []() {
    if (!verificarAcesso())
      return;
    if (server.hasArg("file") && server.hasArg("data")) {
      String path = server.arg("file");
      String conteudo = server.arg("data");
      if (!path.startsWith("/"))
        path = "/" + path;
      File file = SD.open(path, FILE_WRITE);
      if (file) {
        file.print(conteudo);
        file.close();
        server.send(200, "text/plain", "OK");
      } else {
        server.send(500, "text/plain", "Erro ao abrir arquivo para escrita");
      }
    } else {
      server.send(400, "text/plain", "Parametros ausentes");
    }
  });
  server.on("/network", [css, ehMickey]() {
    desligarTela();
    if (!verificarAcesso())
      return;
    if (!ehMickey())
      return server.send(403, "Negado");
    String h =
        "<!DOCTYPE html><html lang='pt'><head><link rel='shortcut icon' "
        "type='image/x-icon' href='http://" +
        WiFi.localIP().toString() +
        "/favicon.ico'><head><meta charset='UTF-8'><meta name='viewport' "
        "content='width=device-width, initial-scale=1.0'>" +
        css +
        "</head><body><div class='box'><h2>CONFIG REDE</h2><form method='POST' "
        "action='/net_save'>";
    h += "SSID:<input name='ss' value='" + cfgSSID +
         "'>PASS:<input name='pw' value='" + cfgPASS + "'>";
    h += "MODO:<select name='mo'><option value='REDE' " +
         (String(cfgMODO == "REDE" ? "selected" : "")) +
         ">REDE (Prefixo)</option>";
    h += "<option value='UNICO' " +
         (String(cfgMODO == "UNICO" ? "selected" : "")) +
         ">IP UNICO</option></select>";
    h += "IP/PREFIXO:<input name='ip' value='" + cfgIP + "'>";
    h += "CHAVE PIX (CPF/Email):<input name='px' value='" + cfgPIX + "'>";
    h += "LINK WISE (Ex: wise.com/pay/me/...):<input name='ws' value='" +
         cfgWiser + "'>";
    h += "<input type='submit' value='SALVAR E REINICIAR'></form><br><a "
         "href='/'>VOLTAR</a></div><footer>'Copyright' 2025-2026 Criado por "
         "Amauri Bueno dos Santos com apoio da Gemini. "
         "https://github.com/Annabel369/2FATouch</footer></body></html>";
    server.send(200, "text/html", h);
  });
  server.on("/net_save", HTTP_POST, [ehMickey]() {
    if (!verificarAcesso())
      return;
    if (ehMickey()) {
      cfgSSID = server.arg("ss");
      cfgPASS = server.arg("pw");
      cfgMODO = server.arg("mo");
      cfgIP = server.arg("ip");
      cfgPIX = server.arg("px");
      cfgWiser = server.arg("ws");
      salvarConfig();
      delay(1000);
      ESP.restart();
    }
  });
  server.on("/vault", [css, ehMickey]() {
    ligarTela();
    if (!verificarAcesso())
      return;
    if (!ehMickey())
      return server.send(403, "Negado");
    String h = "<!DOCTYPE html><html lang='pt'><head><link rel='shortcut icon' "
               "type='image/x-icon' href='http://" +
               WiFi.localIP().toString() +
               "/favicon.ico'><meta charset='UTF-8'><meta name='viewport' "
               "content='width=device-width, initial-scale=1.0'>" +
               css + "</head><body>";
    h += "<div class='box'><h2>CRYPTO VAULT</h2>";
    // --- LISTAGEM DAS SEEDS SALVAS ---
    h += "<div style='text-align:left; margin-bottom:20px; border-bottom:1px "
         "solid #444; padding-bottom:10px;'>";
    if (seeds.size() == 0) {
      h += "<p style='color:#666;'>Nenhuma semente salva.</p>";
    } else {
      for (int i = 0; i < seeds.size(); i++) {
        h += "<div style='margin-bottom:10px; display:flex; "
             "justify-content:space-between; align-items:center;'>";
        h += "<span><b>" + seeds[i].label + "</b></span>";
        h += "<div>";
        h += "<a href='/view_seed?id=" + String(i) +
             "' style='color:#0f0;'>[VER]</a> ";
        h += "<a href='/del_seed?id=" + String(i) + "' class='del'>[X]</a>";
        h += "</div></div>";
      }
    }
    h += "</div>";
    // ---------------------------------
    h += "<form method='POST' action='/reg_seed'>NOME:<input name='n'>SEED (12 "
         "Palavras):<input name='s'><input type='submit' value='ADD "
         "SEED'></form>";
    h += "<br><a href='/'>VOLTAR</a></div>";
    h += "<footer>'Copyright' 2025-2026 Criado por Amauri Bueno dos Santos com "
         "apoio da Gemini.</footer></body></html>";
    server.send(200, "text/html", h);
  });
  server.on("/view_seed", [ehMickey]() {
    if (!verificarAcesso())
      return;
    if (ehMickey()) {
      currentSeedIndex = server.arg("id").toInt();
      forceRedraw = true;
      server.send(200, "text/html", "<script>location.href='/vault';</script>");
    }
  });
  server.on("/del_seed", [ehMickey]() {
    if (!verificarAcesso())
      return;
    if (ehMickey()) {
      int id = server.arg("id").toInt();
      if (id >= 0 && id < seeds.size())
        seeds.erase(seeds.begin() + id);
      SD.remove("/seeds.txt");
      File f = SD.open("/seeds.txt", FILE_WRITE);
      for (auto const &sd : seeds)
        f.println(sd.label + "|" + sd.phrase);
      f.close();
      server.send(200, "text/html", "<script>location.href='/vault';</script>");
    }
  });
  server.on("/reg_seed", HTTP_POST, [ehMickey]() {
    if (!verificarAcesso())
      return;
    if (ehMickey()) {
      seeds.push_back({server.arg("n"), server.arg("s")});
      SD.remove("/seeds.txt");
      File f = SD.open("/seeds.txt", FILE_WRITE);
      for (auto const &sd : seeds)
        f.println(sd.label + "|" + sd.phrase);
      f.close();
      server.send(200, "text/html", "<script>location.href='/vault';</script>");
    }
  });
  server.on("/del", [ehMickey]() {
    if (!verificarAcesso())
      return;
    if (ehMickey()) {
      int id = server.arg("id").toInt();
      if (id >= 0 && id < accounts.size())
        accounts.erase(accounts.begin() + id);
      SD.remove("/totp_secrets.txt");
      File f = SD.open("/totp_secrets.txt", FILE_WRITE);
      for (auto const &a : accounts)
        f.println(a.name + "=" + a.secretBase32 + "=" + a.password);
      f.close();
      forceRedraw = true;
      server.send(200, "text/html",
                  "<script>location.href='/manage';</script>");
    }
  });
  // --- NOVA ROTA PARA O LINUX / YUBIKEY ---
  server.on("/aprovado", []() {
    if (server.hasArg("senha")) {
      String senhaRecebida = server.arg("senha");
      if (senhaRecebida == "T!9vL#4qZp2@hX7d" || senhaRecebida == "R7m2k9Xq") {
        displayMode = 10;
        forceRedraw = true;
        // --- A MÁGICA ACONTECE AQUI ---
        digitalWrite(PINO_RELE_LUZ, HIGH); // Aciona o relé da lâmpada
        servoCreeper.write(90); // Gira o braço para 90 graus (abre a cabeça)
        // Marca o tempo para um possível fechamento automático
        tempoAberto = millis();
        hardwareAtivo = true;
        server.send(200, "text/plain", "Acesso Liberado! Creeper ativado.\n");
        Serial.println("YubiKey ativada! Luz e Servo ligados.");
        return;
      }
    }
    server.send(403, "text/plain", "Acesso Negado: Senha invalida!\n");
  });
  server.on("/pcstats", [ehMickey]() {
    if (ehMickey()) {
      displayMode = 5; // Modo que criamos para o Monitor de Performance
      currentIndex = -1;
      currentSeedIndex = -1;
      forceRedraw = true;
      server.send(200, "text/plain", "Monitor de Performance Ativado!");
    } else {
      server.send(403, "text/plain", "Negado");
    }
  });
  server.on("/select", [ehMickey]() {
    if (ehMickey()) {
      int id = server.arg("id").toInt();
      currentIndex = -1;
      currentSeedIndex = -1;
      // Lógica de seleção (igual à sua)
      if (id == -1)
        displayMode = 0;
      else if (id == -2)
        displayMode = 1;
      else if (id == -3)
        displayMode = 2;
      else if (id == -4) {
        displayMode = 3;
        updateWeather();
      } else if (id == -5)
        displayMode = 5; // <--- SEU NOVO MONITOR DE PC
      else if (id == -6)
        displayMode = 6; // wiser qrcod
      else if (id == -7) {
        displayMode = 7;
        iniciarScanWiFiTFT();
      } else {
        displayMode = 0;
        currentIndex = id;
      }
      forceRedraw = true;
      server.send(200, "text/plain", "OK"); // Resposta curta para o AJAX
    } else {
      server.send(403, "text/plain", "Negado");
    }
  });
  server.begin();
  ftpSrv.begin("creeper", "k9R7xM2pQ4vL8wT5");
}
void loop() {
  // --- VERIFICAÇÃO DO TOUCH NA TELA TODA ---
  static unsigned long last_tap = 0;
  unsigned long debounceDelay = (displayMode == 7) ? 200 : 500;
  if (touchscreen.tirqTouched() && touchscreen.touched()) {
    if (millis() - last_tap > debounceDelay) {
      TS_Point p = touchscreen.getPoint();
      if (p.z > 100 && p.z < 3500) {
        ligarTela();
        int touchX, touchY;
        converterPontoTouch(p, touchX, touchY);
        if (displayMode == 7) {
          handleWiFiTouch(touchX, touchY);
        } else {
          proximaTela();
        }
        last_tap = millis();
      }
    }
  }
  // -------------------------------------------------------
  server.handleClient();
  ftpSrv.handleFTP();
  timeClient.update();
  // --- [NOVO] LÓGICA DE FECHAMENTO AUTOMÁTICO (ABAJUR/SERVO) ---
  // Verifica se o hardware está ativo e se já passaram 30 segundos (30000 ms)
  if (hardwareAtivo && (millis() - tempoAberto > 30000)) {
    digitalWrite(PINO_RELE_LUZ, LOW); // Apaga a luz do abajur
    servoCreeper.write(0);            // Fecha a cabeça do Creeper (0 graus)
    displayMode = 0;                  // Volta o visor para o rosto normal
    forceRedraw = true;               // Avisa o sistema para redesenhar a tela
    hardwareAtivo = false;            // Desmarca a flag de atividade
    Serial.println("Tempo esgotado: Creeper fechado e luz apagada.");
  }
  // -------------------------------------------------------------
  // --- ATUALIZAÇÃO AUTOMÁTICA (CLIMA E VIRADA DO DIA) ---
  static int lastWeatherHour = -1;
  static int lastDay = -1;
  if (timeClient.isTimeSet()) {
    int currentHour = timeClient.getHours();
    int currentDay = timeClient.getDay();
    if (lastWeatherHour != currentHour) {
      updateWeather(); // Atualiza a API meteorologica a cada hora virada
      lastWeatherHour = currentHour;
    }
    if (lastDay != -1 && currentDay != lastDay) {
      forceRedraw = true; // Força redesenho total da tela principal na virada
                          // da meia-noite
      lastDay = currentDay;
    } else if (lastDay == -1) {
      lastDay = currentDay;
    }
  }
  // --------------------------------------------------------
  int pcPacket = udpPC.parsePacket();
  if (pcPacket) {
    int len = udpPC.read(bufferPC, 255);
    if (len > 0)
      bufferPC[len] = 0;
    // Quebra a string "FPS,GPU,TEMP" enviada pelo seu Python
    sscanf(bufferPC, "%d,%d,%d", &pcFPS, &pcGPU, &pcTemp);
    // Se você quiser que o FPS atualize na hora, force o redesenho:
    if (displayMode == 5)
      forceRedraw = true;
  }
  // --- LÓGICA DE WHITELIST UDP ---
  int packetSize = udpWhitelist.parsePacket();
  if (packetSize) {
    int len = udpWhitelist.read(packetBuffer, 255);
    if (len > 0)
      packetBuffer[len] = 0;
    dynamicWhitelist = String(packetBuffer);
  }
  unsigned long epoch = timeClient.getEpochTime();
  int secondsLeft = 30 - (epoch % 30);
  // --- LÓGICA DE ATUALIZAÇÃO DA TELA ---
  if (secondsLeft != lastSec || forceRedraw) {
    bool isRedraw = forceRedraw;
    if (forceRedraw) {
      tft.fillScreen(TFT_BLACK);
      forceRedraw = false;
    }
    lastSec = secondsLeft;
    // --- MODO: VAULT (SEEDS) ---
    if (currentSeedIndex >= 0) {
      if (isRedraw) {
        tft.fillScreen(TFT_BLACK);
        drawSpiderJockey(190, 5, 35);
        tft.setTextColor(TFT_ORANGE, TFT_BLACK);
        tft.drawCentreString("VAULT: " + seeds[currentSeedIndex].label, 100, 10,
                             2);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        String p = seeds[currentSeedIndex].phrase;
        int y = 45, x = 15, count = 0;
        while (p.length() > 0 && count < 12) {
          int space = p.indexOf(' ');
          String word = (space > 0) ? p.substring(0, space) : p;
          tft.drawString(String(count + 1) + "." + word, x, y, 2);
          y += 25;
          count++;
          if (count == 6) {
            x = 120;
            y = 45;
          }
          if (space < 0)
            break;
          p = p.substring(space + 1);
        }
        tft.fillRect(0, 210, 240, 30, tft.color565(40, 40, 40));
        tft.setTextColor(TFT_GREEN);
        tft.drawString("VER", 10, 215, 2);
        tft.setTextColor(TFT_YELLOW);
        tft.drawString("EDITAR", 85, 215, 2);
        tft.setTextColor(TFT_RED);
        tft.drawString("EXCLUIR", 170, 215, 2);
      }
    }
    // --- MODO 1: QR WIFI ---
    else if (displayMode == 1) {
      if (isRedraw)
        drawWiFiScreen();
    }
    // --- MODO 2: QR PIX ---
    else if (displayMode == 2) {
      if (isRedraw)
        drawPixScreen();
    }
    // --- MODO 3: METEOROLOGIA ---
    else if (displayMode == 3 ||
             displayMode == 4) { // Aceita ambos os IDs configurados
      if (isRedraw)
        drawWeatherScreen(epoch);
      drawWeatherHeader(epoch);
    } else if (displayMode == 5) {
      if (isRedraw)
        drawPCPerformance();
    }
    // --- MODO 6: QR WISE ---
    else if (displayMode == 6) {
      if (isRedraw)
        drawWiserScreen();
    }
    // --- MODO 7: CONFIGURAÇÃO WI-FI TOUCH COM TECLADO ---
    else if (displayMode == 7) {
      if (isRedraw) {
        if (wifiSetupState == 0) {
          drawWiFiScanScreen();
        } else {
          drawWiFiKeyboardScreen();
        }
      }
    }
    // --- MODO 10: ACESSO APROVADO PELA YUBIKEY ---
    else if (displayMode == 10) {
      if (isRedraw) { // O isRedraw garante que a imagem seja carregada só 1 vez
        // 1. Carrega a imagem de fundo do SD Card PRIMEIRO
        TJpgDec.drawSdJpg(0, 0, "/minecraft240.jpg");
        // 2. Desenha a borda dupla verde por cima da imagem
        tft.drawRect(0, 0, 240, 240, TFT_GREEN);
        tft.drawRect(1, 1, 238, 238, TFT_GREEN);
        // (Opcional) Se a imagem já tiver o desenho que você quer,
        // você pode remover ou comentar o SpiderJockey abaixo.
        // drawSpiderJockey(30, 40, 180);
        // 3. Coloca os textos por cima da imagem
        tft.setTextColor(TFT_CYAN, TFT_BLACK);
        tft.drawCentreString(
            "Acesso Liberado!", 120, 195,
            4); // Mudei o texto para fazer sentido com o sucesso
        tft.setTextColor(TFT_YELLOW, TFT_BLACK);
        tft.drawCentreString("YUBIKEY OK", 120, 220, 2);
      }
    }
    // --- MODO 0: CREEPER / TOTP (PADRÃO) ---
    else {
      if (currentIndex < 0) {
        if (isRedraw)
          drawCreeper();
        drawInfo(epoch); // MOSTRA HORA E IP NO MODO STANDBY
      } else {
        // --- LÓGICA INTELIGENTE DE NOME/EMAIL ---
        String accountName = accounts[currentIndex].name;
        if (accountName.indexOf('@') >= 0) {
          tft.setTextColor(TFT_CYAN, TFT_BLACK);
          tft.drawCentreString("CONTA:", 120, 5, 2);
          tft.setTextColor(TFT_WHITE, TFT_BLACK);
          int fonteEmail = (accountName.length() > 20) ? 1 : 2;
          tft.drawCentreString(accountName, 120, 25, fonteEmail);
        } else {
          tft.setTextColor(TFT_CYAN, TFT_BLACK);
          tft.drawCentreString(accountName, 120, 10, 4);
        }
        // --- EXIBIÇÃO DO TOKEN ---
        String code = calcTOTP(accounts[currentIndex].secretBase32, epoch);
        tft.setTextColor(TFT_YELLOW, TFT_BLACK);
        tft.drawCentreString(code, 120, 85, 7);
        // Barra de Tempo (30s)
        tft.fillRect(0, 155, 240, 8, TFT_BLACK);
        tft.fillRect(0, 155, (secondsLeft * 8), 8,
                     (secondsLeft < 5) ? TFT_RED : TFT_GREEN);
        if (accounts[currentIndex].password != "") {
          tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
          tft.drawCentreString("Pass: " + accounts[currentIndex].password, 120,
                               210, 2);
        }
      }
    }
  }
}
