// ===== CREEPER AUTH v6.2 - DUAL STACK + NETWORK + SEED COLUMNS (VERSÃO FINAL) =====
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ESPmDNS.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <FS.h>
#include <SD.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <WebServer.h>
#include <ESP32FtpServer.h>
#include "mbedtls/md.h"
#include <vector>
#include "qrcode.h"
#include <ArduinoJson.h> // Você precisará instalar a biblioteca ArduinoJson



#define SD_CS 5
TFT_eSPI tft = TFT_eSPI();
WiFiUDP ntpUDP;
WiFiUDP udpWhitelist; 
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000); 
WebServer server(80);
FtpServer ftpSrv;

// Configurações e Whitelist
String cfgSSID = "Maria Cristina 4G";
String cfgPASS = "1247bfam";
String cfgMODO = "REDE"; 
String cfgIP   = "192.168.100."; 
String dynamicWhitelist = ""; 
String weatherApiKey = "fff7ce772990b49a7efd6b1a6827c687";
String city = "Bom Jesus dos Perdões, BR";
String classificarVento(float kmh) {
  if (kmh < 5)   return "Calmo/Brisa";
  if (kmh < 20)  return "Brisa Leve";
  if (kmh < 40)  return "Vento Moderado";
  if (kmh < 60)  return "Vento Forte";
  if (kmh < 90)  return "VENDAVAL";
  if (kmh < 117) return "TEMPESTADE";
  return "FURACAO/TORNADO";
}

char packetBuffer[255]; 

struct TotpAccount { String name; String secretBase32; String password; };
struct SeedRecord { String label; String phrase; };
struct WeatherData { 
    float temp; 
    float windSpeed; 
    String main; 
    String windDesc; // Para guardar o nome (Brisa, Vendaval, etc)
};

std::vector<TotpAccount> accounts;
std::vector<SeedRecord> seeds;

// Variáveis Globais
int displayMode = 1; // Começa no rosto do Creeper
int currentIndex = -1; 
int currentSeedIndex = -1; 
int lastSec = -1;
bool forceRedraw = true;


WiFiClientSecure client;
WeatherData weather;

bool tlsAtivado = false;

void updateWeather() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    
    // URL configurada para One Call 3.0 com coordenadas de Bom Jesus dos Perdões
    String url = "http://api.openweathermap.org/data/3.0/onecall?lat=-23.13&lon=-46.46&exclude=minutely,hourly,daily&appid=" + weatherApiKey + "&units=metric&lang=pt_br";
    
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
        weather.temp = doc["current"]["temp"];
        
        // 2. Vento (A API envia em m/s, convertemos para km/h multiplicando por 3.6)
        float windMps = doc["current"]["wind_speed"];
        weather.windSpeed = windMps * 3.6;
        
        // 3. Classificacao do vento (Brisa, Vendaval, Tornado...)
        weather.windDesc = classificarVento(weather.windSpeed);
        
        // 4. Descrição do Céu (Ex: "céu limpo", "nuvens esparsas")
        weather.main = doc["current"]["weather"][0]["description"].as<String>();
        
        // Log para o Monitor Serial
        Serial.println("--- DADOS RECEBIDOS ---");
        Serial.print("Temp: "); Serial.print(weather.temp); Serial.println(" C");
        Serial.print("Vento: "); Serial.print(weather.windSpeed); Serial.print(" km/h - "); Serial.println(weather.windDesc);
        Serial.println("-----------------------");

        forceRedraw = true; // Força a atualização do visor TFT
      } else {
        Serial.print("Erro ao processar JSON: "); Serial.println(error.c_str());
      }
    } else {
      Serial.printf("Erro na comunicacao (HTTP): %d\n", httpCode);
    }
    http.end();
  }
}

void drawWeatherScreen() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawCentreString("METEOROLOGIA", 120, 10, 2);

  // Temperatura em destaque
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawCentreString(String(weather.temp, 1) + " C", 120, 50, 6); // Fonte grande

  // Velocidade do Vento
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawCentreString(String(weather.windSpeed, 1) + " km/h", 120, 110, 4);

  // Classificacao do Vento (Muda de cor se for perigoso)
  uint16_t corVento = TFT_GREEN;
  if (weather.windSpeed > 40) corVento = TFT_ORANGE;
  if (weather.windSpeed > 70) corVento = TFT_RED;

  tft.setTextColor(corVento, TFT_BLACK);
  tft.drawCentreString(weather.windDesc, 120, 150, 4);

  // Condicao do Ceu
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.drawCentreString(weather.main, 120, 195, 2);
  
  // IP no rodape para controle
  tft.setTextColor(tft.color565(0, 50, 0));
  tft.drawCentreString(WiFi.localIP().toString(), 120, 225, 1);
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
    tft.drawCentreString("REDE: " + cfgSSID, tft.width() / 2, 205, 2);
    tft.drawCentreString("SENHA: " + cfgPASS, tft.width() / 2, 220, 2);
}

void drawPixScreen() {
    tft.fillScreen(TFT_BLACK);
    QRCode qrcode;
    uint8_t qData[qrcode_getBufferSize(4)];
    String pixPayload = "342.049.358-42";
    
    // Versão 4 com correção nível 2 (Q) é o equilíbrio perfeito
    qrcode_initText(&qrcode, qData, 4, 2, pixPayload.c_str());

    int esc = 6; 
    int qSize = qrcode.size * esc;
    int xOff = (tft.width() - qSize) / 2;
    int yOff = 15;

    // Fundo branco do QR
    tft.fillRect(xOff - 10, yOff - 10, qSize + 20, qSize + 20, TFT_WHITE);

    // Desenha módulos do QR
    for (uint8_t y = 0; y < qrcode.size; y++) {
        for (uint8_t x = 0; x < qrcode.size; x++) {
            if (qrcode_getModule(&qrcode, x, y)) {
                tft.fillRect(xOff + (x * esc), yOff + (y * esc), esc, esc, TFT_BLACK);
            }
        }
    }

    // --- DESENHO DO PORCO PEQUENO (8x8 Proporcional) ---
    int cSize = 32; // Tamanho total do rosto do porco
    int cx = xOff + (qSize / 2) - (cSize / 2);
    int cy = yOff + (qSize / 2) - (cSize / 2);
    int p = cSize / 8; // p = 4 pixels

    // Margem de segurança branca
    tft.fillRect(cx - 2, cy - 2, cSize + 4, cSize + 4, TFT_WHITE);

    // Cores do Porco
    uint16_t ROSA = tft.color565(255, 180, 190);
    uint16_t FOCINHO = tft.color565(255, 120, 160);

    tft.fillRect(cx, cy, cSize, cSize, ROSA); // Rosto

    // Olhos (Preto/Branco)
    tft.fillRect(cx,         cy + 2*p, p, p, TFT_BLACK); 
    tft.fillRect(cx + p,     cy + 2*p, p, p, TFT_WHITE);
    tft.fillRect(cx + 6*p,   cy + 2*p, p, p, TFT_WHITE);
    tft.fillRect(cx + 7*p,   cy + 2*p, p, p, TFT_BLACK);

    // Focinho
    tft.fillRect(cx + 2*p, cy + 4*p, 4*p, 2*p, FOCINHO);
    tft.fillRect(cx + 2*p, cy + 5*p, p, p, tft.color565(200, 80, 120)); // Narina esq
    tft.fillRect(cx + 5*p, cy + 5*p, p, p, tft.color565(200, 80, 120)); // Narina dir

    // Textos
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawCentreString("PIX - CPF", tft.width()/2, 205, 4);
    tft.drawCentreString(pixPayload, tft.width()/2, 225, 2);
}

// --- Gestão de Arquivos ---
void salvarConfig() {
  File f = SD.open("/config.txt", FILE_WRITE);
  if (f) { 
    f.println("SSID=" + cfgSSID); 
    f.println("PASS=" + cfgPASS); 
    f.println("MODO=" + cfgMODO); 
    f.println("IP_ALVO=" + cfgIP); 
    f.close(); 
  }
}

void carregarTudo() {
  if (!SD.begin(SD_CS)) return;
  if (SD.exists("/config.txt")) {
    File f = SD.open("/config.txt", FILE_READ);
    while (f.available()) {
      String line = f.readStringUntil('\n'); line.trim();
      if (line.startsWith("SSID=")) cfgSSID = line.substring(5);
      else if (line.startsWith("PASS=")) cfgPASS = line.substring(5);
      else if (line.startsWith("MODO=")) cfgMODO = line.substring(5);
      else if (line.startsWith("IP_ALVO=")) cfgIP = line.substring(8);
    }
    f.close();
  }
  accounts.clear();
  File f2 = SD.open("/totp_secrets.txt", FILE_READ);
  if (f2) {
    while (f2.available()) {
      String line = f2.readStringUntil('\n'); line.trim();
      int e1 = line.indexOf('='); int e2 = line.indexOf('=', e1 + 1);
      if (e1 > 0 && e2 > 0) accounts.push_back({line.substring(0, e1), line.substring(e1+1, e2), line.substring(e2+1)});
    }
    f2.close();
  }
  seeds.clear();
  File fs = SD.open("/seeds.txt", FILE_READ);
  if (fs) {
    while (fs.available()) {
      String line = fs.readStringUntil('\n'); line.trim();
      int p = line.indexOf('|');
      if (p > 0) seeds.push_back({line.substring(0, p), line.substring(p+1)});
    }
    fs.close();
  }
}

void drawSpiderJockey(int x, int y, int tam) {
    int p = tam / 10; // Unidade de pixel
    
    // Corpo da Aranha (Marrom escuro)
    uint16_t MARROM = tft.color565(60, 40, 30);
    tft.fillRect(x, y + 5*p, tam, 4*p, MARROM); // Abdômen
    tft.fillRect(x + 2*p, y + 4*p, 4*p, 3*p, MARROM); // Cabeça da aranha
    
    // Olhos Vermelhos da Aranha
    tft.fillRect(x + 3*p, y + 5*p, 1, 1, TFT_RED);
    tft.fillRect(x + 5*p, y + 5*p, 1, 1, TFT_RED);
    
    // Pernas da Aranha
    for(int i=0; i<4; i++) {
        tft.drawLine(x + 2*p, y + 6*p, x - 2*p, y + 4*p + (i*2), MARROM); // Esquerda
        tft.drawLine(x + 6*p, y + 6*p, x + tam, y + 4*p + (i*2), MARROM); // Direita
    }

    // Esqueleto (Cinza claro)
    uint16_t CINZA = tft.color565(200, 200, 200);
    tft.fillRect(x + 3*p, y, 3*p, 3*p, CINZA); // Cabeça
    tft.fillRect(x + 4*p, y + 3*p, p, 3*p, CINZA); // Coluna/Corpo
    
    // Olhos do Esqueleto
    tft.fillRect(x + 3*p + 1, y + 1, 1, 1, TFT_BLACK);
    tft.fillRect(x + 5*p - 1, y + 1, 1, 1, TFT_BLACK);
    
    // Arco (Amarelo queimado)
    tft.drawCircle(x + 6*p, y + 3*p, 4, tft.color565(150, 120, 50));
}

// --- TOTP Lógica ---
int base32CharToVal(char c) {
  if (c >= 'A' && c <= 'Z') return c - 'A';
  if (c >= '2' && c <= '7') return 26 + (c - '2');
  return -1;
}

int base32Decode(const String& input, uint8_t* output, int maxOut) {
  String s = input; s.toUpperCase(); s.replace(" ", ""); 
  int buffer = 0, bitsLeft = 0, outCount = 0;
  for (size_t i = 0; i < s.length(); i++) {
    int val = base32CharToVal(s[i]);
    if (val < 0) continue;
    buffer <<= 5; buffer |= val & 0x1F; bitsLeft += 5;
    if (bitsLeft >= 8) { bitsLeft -= 8; if (outCount < maxOut) output[outCount++] = (buffer >> bitsLeft) & 0xFF; }
  }
  return outCount;
}

String calcTOTP(const String& secret, unsigned long epoch) {
  unsigned long counter = epoch / 30;
  uint8_t msg[8];
  for (int i = 7; i >= 0; i--) { msg[i] = counter & 0xFF; counter >>= 8; }
  uint8_t key[64]; int keyLen = base32Decode(secret, key, sizeof(key));
  if (keyLen <= 0) return "ERRO";
  uint8_t hash[20];
  mbedtls_md_hmac(mbedtls_md_info_from_type(MBEDTLS_MD_SHA1), key, keyLen, msg, 8, hash);
  int offset = hash[19] & 0x0F;
  uint32_t bin_code = ((hash[offset] & 0x7F) << 24) | ((hash[offset+1] & 0xFF) << 16) | ((hash[offset+2] & 0xFF) << 8) | (hash[offset+3] & 0xFF);
  char buf[7]; snprintf(buf, sizeof(buf), "%06d", bin_code % 1000000);
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
    for(int i = 60; i <= 80; i += 10) {
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
  time_t rawtime = (time_t)epoch;
  struct tm * ti = gmtime(&rawtime);
  char f_time[20];
  
  // Ajuste para Horário de Brasília (UTC-3)
  sprintf(f_time, "%02d/%02d %02d:%02d", ti->tm_mday, ti->tm_mon + 1, (ti->tm_hour + 21)%24, ti->tm_min);
  
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawCentreString(f_time, 120, 175, 2);
  
  // Mostra o IP em Verde Matrix no visor
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawCentreString(WiFi.localIP().toString(), 120, 215, 2);
}

// --- Setup ---
void setup() {
  Serial.begin(115200);
  SPI.begin(18, 19, 23, SD_CS);
  tft.init(); tft.setRotation(0); tft.invertDisplay(true); tft.fillScreen(TFT_BLACK);
  carregarTudo();
  
  WiFi.begin(cfgSSID.c_str(), cfgPASS.c_str());
  WiFi.enableIPv6(); 
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis()-start < 10000) delay(500);

  // --- PRINT NO CONSOLE (Monitor Serial) ---

  Serial.println("\n--- REDE CONECTADA ---");
  Serial.print("IPv4: "); Serial.println(WiFi.localIP());
  
  // No Core 3.x, usamos linkLocalIPv6() para o endereço fe80::
  Serial.print("IPv6: "); Serial.println(WiFi.linkLocalIPv6());
  Serial.println("-------------------------");
  Serial.println("FTP PORTA 21 User: creeper, Pass: 1234");
  Serial.println("ftp://creeper:1234@192.168.100.49/");
  Serial.println("-------------------------");
  // No setup, após conectar no Wi-Fi:
if (MDNS.begin("creeper")) {
  Serial.println("MDNS responder iniciado: http://creeper.local");
}

  timeClient.begin();
  udpWhitelist.begin(1234);

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
    String h = "<!DOCTYPE html><html lang='pt'><head><link rel='shortcut icon' type='image/x-icon' href='https://www.minecraft.net/etc.clientlibs/minecraftnet/clientlibs/clientlib-site/resources/favicon.ico'><meta name='viewport' content='width=device-width, initial-scale=1.0'><meta charset='UTF-8'>" + css + "</head><body>";
    
    h += "<div class='box'>";
    

// --- SEU NOVO BLOCO DO CREEPER ---
    h += "<div id='creeper-container'>";
    h += "<div id='creeper-body'></div>";
    h += "<div class='pixel' style='left: 30px; top: 10px;'></div><div class='pixel' style='left: 40px; top: 10px;'></div><div class='pixel' style='left: 70px; top: 10px;'></div><div class='pixel' style='left: 80px; top: 10px;'></div>";
    h += "<div class='pixel' style='left: 20px; top: 20px;'></div><div class='pixel' style='left: 30px; top: 20px;'></div><div class='pixel' style='left: 40px; top: 20px;'></div><div class='pixel' style='left: 70px; top: 20px;'></div><div class='pixel' style='left: 80px; top: 20px;'></div><div class='pixel' style='left: 90px; top: 20px;'></div>";
    h += "<div class='pixel' style='left: 20px; top: 30px;'></div><div class='pixel' style='left: 30px; top: 30px;'></div><div class='pixel' style='left: 40px; top: 30px;'></div><div class='pixel' style='left: 70px; top: 30px;'></div><div class='pixel' style='left: 80px; top: 30px;'></div><div class='pixel' style='left: 90px; top: 30px;'></div>";
    h += "<div class='pixel' style='left: 50px; top: 50px;'></div><div class='pixel' style='left: 60px; top: 50px;'></div>";
    h += "<div class='pixel' style='left: 30px; top: 60px;'></div><div class='pixel' style='left: 40px; top: 60px;'></div><div class='pixel' style='left: 50px; top: 60px;'></div><div class='pixel' style='left: 60px; top: 60px;'></div><div class='pixel' style='left: 70px; top: 60px;'></div><div class='pixel' style='left: 80px; top: 60px;'></div>";
    h += "<div class='pixel' style='left: 30px; top: 70px;'></div><div class='pixel' style='left: 40px; top: 70px;'></div><div class='pixel' style='left: 50px; top: 70px;'></div><div class='pixel' style='left: 60px; top: 70px;'></div><div class='pixel' style='left: 70px; top: 70px;'></div><div class='pixel' style='left: 80px; top: 70px;'></div>";
    h += "<div class='pixel' style='left: 30px; top: 80px;'></div><div class='pixel' style='left: 40px; top: 80px;'></div><div class='pixel' style='left: 50px; top: 80px;'></div><div class='pixel' style='left: 60px; top: 80px;'></div><div class='pixel' style='left: 70px; top: 80px;'></div><div class='pixel' style='left: 80px; top: 80px;'></div>";
    h += "<div class='pixel' style='left: 30px; top: 90px;'></div><div class='pixel' style='left: 40px; top: 90px;'></div><div class='pixel' style='left: 70px; top: 90px;'></div><div class='pixel' style='left: 80px; top: 90px;'></div>";
    h += "<div class='pixel' style='left: 30px; top: 100px;'></div><div class='pixel' style='left: 40px; top: 100px;'></div><div class='pixel' style='left: 70px; top: 100px;'></div><div class='pixel' style='left: 80px; top: 100px;'></div>";
    h += "</div>"; 
// ------------------------------------


if (tlsAtivado) {
    h += "<p style='color:#0f0; font-size:0.8em;'>🔒 CONEXÃO SEGURA (TLS)</p>";
} else {
    h += "<p style='color:#666; font-size:0.8em;'>🔓 MODO INTRANET (HTTP)</p>";
}

h += "<h2>CREEPER AUTH v6.2</h2>";

// --- NOVO BLOCO: CONTROLO DO VISOR FÍSICO ---
    h += "<div style='border:1px solid #444; padding:10px; margin-bottom:15px;'>";
    h += "<p>VISOR DO DISPOSITIVO:</p>";
    h += "<a href='https://home.openweathermap.org/api_keys' class='edit'>Api Meteorologica</a> ";
    h += "</div>";
// --------------------------------------------


    h += "<form action='/select'><select name='id'><option value='-1'>VISOR CREEPER</option>";

// Botão WiFi
h += "  <option value='-2'>VISOR: QR CODE WIFI</option>";

// Botão PIX
h += "  <option value='-3'>VISOR: QR CODE PIX</option>";

// NOVO: Botão Meteorologia
h += "  <option value='-4'>VISOR: METEOROLOGIA (API)</option>";

for(int i=0; i<accounts.size(); i++) {
    h += "<option value='"+String(i)+"'>"+accounts[i].name+"</option>";
}

h += "</select><input type='submit' value='EXIBIR NO VISOR'></form><br>";

    if(ehMickey()) {
      h += "<a href='/vault'>VAULT</a> <a href='/manage'>TOKENS</a><br><a href='/network'>WI-FI & IP</a>";
    } else {
      h += "<p style='color:red'>ACESSO NEGADO: IP PROTEGIDO</p>";
    }

    h += "</div><footer>'Copyright' 2025-2026 Criado por Amauri Bueno dos Santos com apoio da Gemini. https://github.com/Annabel369/2FA</footer></body></html>";
    server.send(200, "text/html", h);
});

  server.on("/manage", [css, ehMickey](){
    if(!ehMickey()) return server.send(403, "Negado");
    String h = "<!DOCTYPE html><html lang='pt'><head><link rel='shortcut icon' type='image/x-icon' href='https://www.minecraft.net/etc.clientlibs/minecraftnet/clientlibs/clientlib-site/resources/favicon.ico'>><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>"+css+"</head><body><div class='box'><h2>GERENCIAR TOKENS</h2>";
    for(int i=0; i<accounts.size(); i++) {
      h += "<div style='margin-bottom:10px;'>" + accounts[i].name + " <br>";
      h += "<a href='/edit?id="+String(i)+"' class='edit'>[E] EDITAR</a> ";
      h += "<a href='/del?id="+String(i)+"' class='del'>[X] EXCLUIR</a></div>";
    }
    h += "<hr><a href='/add'>+ NOVO TOKEN</a><br><a href='/'>VOLTAR</a></div><footer>'Copyright' 2025-2026 Criado por Amauri Bueno dos Santos com apoio da Gemini. https://github.com/Annabel369/2FA</footer></body></html>";
    server.send(200, "text/html", h);
  });

  server.on("/exibir", []() {
    String modo = server.arg("modo");
    
    currentIndex = -1;
    currentSeedIndex = -1;

    if (modo == "WIFI") {
        displayMode = 1; // QR WiFi
    } 
    else if (modo == "PIX") {
        displayMode = 2; // QR PIX
    }
    else if (modo == "CLIMA") {
        displayMode = 4; // Nova tela de Meteorologia
        updateWeather(); // Busca os dados da API com sua chave Default
    }
    else {
        displayMode = 0; // Volta para o Rosto do Creeper
    }

    forceRedraw = true; 
    server.send(200, "text/html", "Modo " + modo + " Ativado no Visor");
});

  // --- NOVA ROTA: FORMULÁRIO PARA ADICIONAR ---
  server.on("/add", [css, ehMickey](){
    if(!ehMickey()) return server.send(403, "Negado");
    String h = "<!DOCTYPE html><html lang='pt'><head><link rel='shortcut icon' type='image/x-icon' href='https://www.minecraft.net/etc.clientlibs/minecraftnet/clientlibs/clientlib-site/resources/favicon.ico'><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>"+css+"</head><body><div class='box'><h2>NOVO TOKEN</h2><form method='POST' action='/reg'>";
    h += "NOME (Ex: Discord):<input name='u'>SECRET (Base32):<input name='s'>SENHA (Opcional):<input name='p'><input type='submit' value='CRIAR TOKEN'></form><br><a href='/manage'>VOLTAR</a></div></body></html>";
    server.send(200, "text/html", h);
  });

  // --- NOVA ROTA: REGISTRAR NO SD ---
  server.on("/reg", HTTP_POST, [ehMickey](){
    if(ehMickey()){
      String s = server.arg("s"); s.toUpperCase(); s.replace(" ", "");
      accounts.push_back({server.arg("u"), s, server.arg("p")});
      File f = SD.open("/totp_secrets.txt", FILE_APPEND); 
      if(f) { f.println(server.arg("u") + "=" + s + "=" + server.arg("p")); f.close(); }
      server.send(200, "text/html", "<script>location.href='/manage';</script>");
    }
  });

  server.on("/edit", [css, ehMickey](){
    if(!ehMickey()) return server.send(403, "Negado");
    int id = server.arg("id").toInt();
    TotpAccount acc = accounts[id];
    String h = "<!DOCTYPE html><html lang='pt'><head><link rel='shortcut icon' type='image/x-icon' href='https://www.minecraft.net/etc.clientlibs/minecraftnet/clientlibs/clientlib-site/resources/favicon.ico'><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>"+css+"</head><body><div class='box'><h2>EDITAR TOKEN</h2><form method='POST' action='/update?id="+String(id)+"'>'";
    h += "NOME:<input name='u' value='"+acc.name+"'>SECRET:<input name='s' value='"+acc.secretBase32+"'>PASS:<input name='p' value='"+acc.password+"'><input type='submit' value='SALVAR ALTERAÇÕES'></form></div></body></html>";
    server.send(200, "text/html", h);
  });

  server.on("/update", HTTP_POST, [ehMickey](){
    if(ehMickey()){
      int id = server.arg("id").toInt();
      String s = server.arg("s"); s.toUpperCase(); s.replace(" ", "");
      accounts[id] = {server.arg("u"), s, server.arg("p")};
      SD.remove("/totp_secrets.txt"); File f = SD.open("/totp_secrets.txt", FILE_WRITE);
      for(auto const& a : accounts) f.println(a.name + "=" + a.secretBase32 + "=" + a.password);
      f.close(); forceRedraw = true;
      server.send(200, "text/html", "<script>location.href='/manage';</script>");
    }
  });

  server.on("/network", [css, ehMickey](){
    if(!ehMickey()) return server.send(403, "Negado");
    String h = "<!DOCTYPE html><html lang='pt'><head><link rel='shortcut icon' type='image/x-icon' href='https://www.minecraft.net/etc.clientlibs/minecraftnet/clientlibs/clientlib-site/resources/favicon.ico'><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>"+css+"</head><body><div class='box'><h2>CONFIG REDE</h2><form method='POST' action='/net_save'>";
    h += "SSID:<input name='ss' value='"+cfgSSID+"'>PASS:<input name='pw' value='"+cfgPASS+"'>";
    h += "MODO:<select name='mo'><option value='REDE' "+(String(cfgMODO=="REDE"?"selected":""))+">REDE (Prefixo)</option>";
    h += "<option value='UNICO' "+(String(cfgMODO=="UNICO"?"selected":""))+">IP UNICO</option></select>";
    h += "IP/PREFIXO:<input name='ip' value='"+cfgIP+"'><input type='submit' value='SALVAR E REINICIAR'></form><br><a href='/'>VOLTAR</a></div><footer>'Copyright' 2025-2026 Criado por Amauri Bueno dos Santos com apoio da Gemini. https://github.com/Annabel369/2FA</footer></body></html>";
    server.send(200, "text/html", h);
  });

  server.on("/net_save", HTTP_POST, [ehMickey](){
    if(ehMickey()){
      cfgSSID=server.arg("ss"); cfgPASS=server.arg("pw"); cfgMODO=server.arg("mo"); cfgIP=server.arg("ip");
      salvarConfig(); delay(1000); ESP.restart();
    }
  });

  server.on("/vault", [css, ehMickey](){
    if(!ehMickey()) return server.send(403, "Negado");
    
    String h = "<!DOCTYPE html><html lang='pt'><head><link rel='shortcut icon' type='image/x-icon' href='https://www.minecraft.net/etc.clientlibs/minecraftnet/clientlibs/clientlib-site/resources/favicon.ico'><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>" + css + "</head><body>";
    h += "<div class='box'><h2>CRYPTO VAULT</h2>";
    
    // --- LISTAGEM DAS SEEDS SALVAS ---
    h += "<div style='text-align:left; margin-bottom:20px; border-bottom:1px solid #444; padding-bottom:10px;'>";
    if (seeds.size() == 0) {
        h += "<p style='color:#666;'>Nenhuma semente salva.</p>";
    } else {
        for(int i=0; i<seeds.size(); i++) {
          h += "<div style='margin-bottom:10px; display:flex; justify-content:space-between; align-items:center;'>";
          h += "<span><b>" + seeds[i].label + "</b></span>";
          h += "<div>";
          h += "<a href='/view_seed?id="+String(i)+"' style='color:#0f0;'>[VER]</a> ";
          h += "<a href='/del_seed?id="+String(i)+"' class='del'>[X]</a>";
          h += "</div></div>";
        }
    }
    h += "</div>";
    // ---------------------------------

    h += "<form method='POST' action='/reg_seed'>NOME:<input name='n'>SEED (12 Palavras):<input name='s'><input type='submit' value='ADD SEED'></form>";
    h += "<br><a href='/'>VOLTAR</a></div>";
    h += "<footer>'Copyright' 2025-2026 Criado por Amauri Bueno dos Santos com apoio da Gemini.</footer></body></html>";
    
    server.send(200, "text/html", h);
  });

  server.on("/view_seed", [ehMickey](){
    if(ehMickey()){ currentSeedIndex = server.arg("id").toInt(); forceRedraw = true; server.send(200, "text/html", "<script>location.href='/vault';</script>"); }
  });

  server.on("/del_seed", [ehMickey](){
    if(ehMickey()){
      int id = server.arg("id").toInt();
      if(id >= 0 && id < seeds.size()) seeds.erase(seeds.begin() + id);
      SD.remove("/seeds.txt"); File f = SD.open("/seeds.txt", FILE_WRITE);
      for(auto const& sd : seeds) f.println(sd.label + "|" + sd.phrase);
      f.close(); server.send(200, "text/html", "<script>location.href='/vault';</script>");
    }
  });

  server.on("/reg_seed", HTTP_POST, [ehMickey](){
    if(ehMickey()){
      seeds.push_back({server.arg("n"), server.arg("s")});
      SD.remove("/seeds.txt"); File f = SD.open("/seeds.txt", FILE_WRITE);
      for(auto const& sd : seeds) f.println(sd.label + "|" + sd.phrase);
      f.close(); server.send(200, "text/html", "<script>location.href='/vault';</script>");
    }
  });

  server.on("/del", [ehMickey](){
    if(ehMickey()){
      int id = server.arg("id").toInt();
      if(id >= 0 && id < accounts.size()) accounts.erase(accounts.begin() + id);
      SD.remove("/totp_secrets.txt"); File f = SD.open("/totp_secrets.txt", FILE_WRITE);
      for(auto const& a : accounts) f.println(a.name + "=" + a.secretBase32 + "=" + a.password);
      f.close(); forceRedraw = true;
      server.send(200, "text/html", "<script>location.href='/manage';</script>");
    }
  });

  server.on("/select", [ehMickey]() {
    if(ehMickey()) {
        int id = server.arg("id").toInt();
        
        // Limpa índices para não misturar as telas
        currentIndex = -1;
        currentSeedIndex = -1;

        if (id == -1) {
            displayMode = 0;      // Modo Rosto Creeper (2FA)
        } 
        else if (id == -2) {
            displayMode = 1;      // Modo QR Code WiFi
        } 
        else if (id == -3) {
            displayMode = 2;      // Modo QR Code PIX
        } 
        else if (id == -4) {
            displayMode = 3;      // NOVO: Modo Meteorologia (Vento/Chuva)
            updateWeather();      // Chama a função para buscar os dados da API agora
        }
        else {
            displayMode = 0;      
            currentIndex = id;    // Mostra Token da conta selecionada
        }
        
        forceRedraw = true; 
        server.send(200, "text/html", "<script>location.href='/';</script>"); 
    }
});

  server.begin();
  ftpSrv.begin("creeper", "1234");
}

void loop() {
  server.handleClient();
  ftpSrv.handleFTP();
  timeClient.update();

  // --- LÓGICA DE WHITELIST UDP ---
  int packetSize = udpWhitelist.parsePacket();
  if (packetSize) {
    int len = udpWhitelist.read(packetBuffer, 255);
    if (len > 0) packetBuffer[len] = 0;
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
          tft.drawCentreString("VAULT: " + seeds[currentSeedIndex].label, 100, 10, 2);
          
          tft.setTextColor(TFT_WHITE, TFT_BLACK);
          String p = seeds[currentSeedIndex].phrase;
          int y = 45, x = 15, count = 0;
          
          while (p.length() > 0 && count < 12) {
            int space = p.indexOf(' ');
            String word = (space > 0) ? p.substring(0, space) : p;
            tft.drawString(String(count + 1) + "." + word, x, y, 2);
            y += 25; count++;
            if (count == 6) { x = 120; y = 45; }
            if (space < 0) break;
            p = p.substring(space + 1);
          }
          tft.fillRect(0, 210, 240, 30, tft.color565(40, 40, 40));
          tft.setTextColor(TFT_GREEN);  tft.drawString("VER", 10, 215, 2);
          tft.setTextColor(TFT_YELLOW); tft.drawString("EDITAR", 85, 215, 2);
          tft.setTextColor(TFT_RED);    tft.drawString("EXCLUIR", 170, 215, 2);
      }
    } 
    // --- MODO 1: QR WIFI ---
    else if (displayMode == 1) {
       if (isRedraw) drawWiFiScreen();
    }
    // --- MODO 2: QR PIX ---
    else if (displayMode == 2) {
       if (isRedraw) drawPixScreen();
    }
    // --- MODO 3: METEOROLOGIA ---
    else if (displayMode == 3 || displayMode == 4) { // Aceita ambos os IDs configurados
       if (isRedraw) drawWeatherScreen();
    }
    // --- MODO 0: CREEPER / TOTP (PADRÃO) ---
    else {
      if (currentIndex < 0) {
        if (isRedraw) drawCreeper(); 
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
        tft.fillRect(0, 155, (secondsLeft * 8), 8, (secondsLeft < 5) ? TFT_RED : TFT_GREEN);
        
        if (accounts[currentIndex].password != "") {
           tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
           tft.drawCentreString("Pass: " + accounts[currentIndex].password, 120, 210, 2);
        }
      }
    }
  }
}
