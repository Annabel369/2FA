#pragma once
#include <Arduino.h>

// ====================================================================
// BIBLIOTECAS DO SISTEMA
// ====================================================================
// ===== CREEPER AUTH v7.2.2 - DUAL STACK + NETWORK + SEED COLUMNS (VERSÃO
// FINAL)
// =====
#include "mbedtls/md.h"
#include "qrcode.h"
#include <ArduinoJson.h> 
#include <ESP32FtpServer.h> // Você precisará instalar a biblioteca ESP32FtpServer ela vem com tudo
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
String cfgSSID = "Maria Cristina 4G";
String cfgPASS = "1247bfam";
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

// --- VARIÁVEIS E FUNÇÕES DO PORTAL DE CONFIGURAÇÃO WI-FI TOUCH (MODO 7) ---
int wifiSetupState = 0; // 0 = Selecionar SSID (Scan), 1 = Teclado de Senha
std::vector<String> scannedSSIDs;
std::vector<int32_t> scannedRSSI;
String selectedSSID = "";
String inputWifiPass = "";
bool shiftActive = false;
int wifiScanPage = 0;

String versaoAtual = "2.2";
String urlVersaoGitHub = "https://raw.githubusercontent.com/Annabel369/2FATouch/main/version.json";
String versaoNova = "";
bool updateDisponivel = false;

WiFiClientSecure client;
WeatherData weather;
bool tlsAtivado = false;

