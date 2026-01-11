// ===== CREEPER AUTH v6.2 - DUAL STACK + NETWORK + SEED COLUMNS (VERSÃO FINAL) =====
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
char packetBuffer[255]; 

struct TotpAccount { String name; String secretBase32; String password; };
struct SeedRecord { String label; String phrase; };

std::vector<TotpAccount> accounts;
std::vector<SeedRecord> seeds;

// Variáveis Globais
int displayMode = 1; // Começa no rosto do Creeper
int currentIndex = -1; 
int currentSeedIndex = -1; 
int lastSec = -1;
bool forceRedraw = true;


WiFiClientSecure client;

bool tlsAtivado = false;

void drawWiFiScreen() {
    tft.fillScreen(TFT_BLACK); // Fundo preto para destaque
    
    QRCode qrcode; 
    // Aumentamos para Versão 4 para caber mais informação com margem de erro melhor
    uint8_t qData[qrcode_getBufferSize(4)]; 
    String wifiPayload = "WIFI:S:" + cfgSSID + ";T:WPA;P:" + cfgPASS + ";;";
    
    // Inicializa Versão 4, Nível de correção M (Médio) para permitir o logo central
    qrcode_initText(&qrcode, qData, 4, 1, wifiPayload.c_str());

    // Ajuste de escala para ocupar a tela (Escala 6 ou 7 dependendo do display)
    int esc = 6; 
    int xOff = (240 - (qrcode.size * esc)) / 2;
    int yOff = 15; // Jogado mais para o topo

    // 1. Desenha o "Fundo Branco" (Quiet Zone) para o QR Code funcionar
    tft.fillRect(xOff - 10, yOff - 10, (qrcode.size * esc) + 20, (qrcode.size * esc) + 20, TFT_WHITE);

    // 2. Desenha os módulos pretos do QR Code
    for (uint8_t y = 0; y < qrcode.size; y++) {
        for (uint8_t x = 0; x < qrcode.size; x++) {
            if (qrcode_getModule(&qrcode, x, y)) {
                tft.fillRect(xOff + (x * esc), yOff + (y * esc), esc, esc, TFT_BLACK);
            }
        }
    }

    // 3. Desenha o Logo do Creeper com a coluna extra pedida
    int cSize = 48; // Tamanho do bloco do logo
    int cx = xOff + (qrcode.size * esc / 2) - (cSize / 2);
    int cy = yOff + (qrcode.size * esc / 2) - (cSize / 2);
    
    // Fundo branco do logo para não vazar o QR Code por trás
    tft.fillRect(cx - 2, cy - 2, cSize + 4, cSize + 4, TFT_WHITE); 
    
    // Proporção baseada em 8 colunas (para ter a coluna extra no meio)
    int p = cSize / 8; 

    // Olhos (2x2)
    tft.fillRect(cx + p,     cy + p, 2 * p, 2 * p, TFT_BLACK); // Esquerdo
    tft.fillRect(cx + 5 * p, cy + p, 2 * p, 2 * p, TFT_BLACK); // Direito
    
    // Nariz e Boca (Ajustado com a coluna central extra)
    tft.fillRect(cx + 3 * p, cy + 3 * p, 2 * p, 3 * p, TFT_BLACK); // Centro/Nariz largo
    tft.fillRect(cx + 2 * p, cy + 4 * p, 4 * p, 3 * p, TFT_BLACK); // Boca principal
    tft.fillRect(cx + 2 * p, cy + 6 * p, p, p, TFT_BLACK);         // Canto baixo esq
    tft.fillRect(cx + 5 * p, cy + 6 * p, p, p, TFT_BLACK);         // Canto baixo dir

    // Informações de texto na base (Fonte menor para não poluir)
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawCentreString("REDE: " + cfgSSID, 120, 210, 2);
    tft.drawCentreString("SENHA: " + cfgPASS, 120, 225, 2);
}

void setupTLS() {
  // Verifica se os arquivos existem antes de tentar abrir
  if (!SD.exists("/certs/public.pem") || !SD.exists("/certs/private.pem")) {
    Serial.println("Certificados não encontrados. Ignorando TLS...");
    tlsAtivado = false;
    return;
  }

  File cert = SD.open("/certs/public.pem");
  File key  = SD.open("/certs/private.pem");

  if (cert && key) {
    // Aqui assumimos que você está usando um objeto de servidor seguro
    // Se for o 'client' de uma conexão de saída:
    client.setCertificate(cert.readString().c_str());
    client.setPrivateKey(key.readString().c_str());
    tlsAtivado = true;
    Serial.println("TLS configurado com sucesso.");
  } else {
    Serial.println("Erro ao ler arquivos de certificado.");
    tlsAtivado = false;
  }

  cert.close();
  key.close();
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
    //h += "<div style='border:1px solid #444; padding:10px; margin-bottom:15px;'>";
    //h += "<p>VISOR DO DISPOSITIVO:</p>";
    //h += "<a href='/exibir?modo=WIFI' class='edit'>GERAR QR WIFI</a> ";
    //h += "<a href='/exibir?modo=CREEPER'>ROSTO NORMAL</a>";
    //h += "</div>";
// --------------------------------------------


    h += "<form action='/select'><select name='id'><option value='-1'>VISOR CREEPER</option>";
    h += "  <option value='-2'>VISOR: QR CODE WIFI</option>";
    for(int i=0; i<accounts.size(); i++) h += "<option value='"+String(i)+"'>"+accounts[i].name+"</option>";
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
    if (modo == "WIFI") {
        displayMode = 2; // Segunda tela
    } else {
        displayMode = 1; // Rosto grande
    }
    server.send(200, "text/html", "Modo Alterado");
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
    String h = "<html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>"+css+"</head><body><div class='box'><h2>CRYPTO VAULT</h2>";
    for(int i=0; i<seeds.size(); i++) h += "<b>"+seeds[i].label+"</b> <a href='/view_seed?id="+String(i)+"'>VER</a> <a href='/del_seed?id="+String(i)+"' class='del'>X</a><br>";
    h += "<hr><form method='POST' action='/reg_seed'>NOME:<input name='n'>SEED:<input name='s'><input type='submit' value='ADD SEED'></form><br><a href='/'>VOLTAR</a></div></body></html>";
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
        
        if (id == -1) {
            displayMode = 1;      // Modo Rosto
            currentIndex = -1;    // Tira qualquer conta da tela
            currentSeedIndex = -1;
        } 
        else if (id == -2) {
            displayMode = 2;      // Modo QR Code
            currentIndex = -1;    // Tira qualquer conta da tela
            currentSeedIndex = -1;
        } 
        else {
            displayMode = 1;      // Volta pro modo normal para exibir o Token
            currentIndex = id;    // Define qual conta mostrar
            currentSeedIndex = -1;
        }
        
        forceRedraw = true; 
        // Redireciona de volta para a home em vez de ficar em página branca
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

  int packetSize = udpWhitelist.parsePacket();
  #ifdef ESP32
  if (packetSize) {
    int len = udpWhitelist.read(packetBuffer, 255);
    if (len > 0) packetBuffer[len] = 0;
    dynamicWhitelist = String(packetBuffer);
  }
  #endif

  unsigned long epoch = timeClient.getEpochTime();
  int secondsLeft = 30 - (epoch % 30);

  // --- LÓGICA DE ATUALIZAÇÃO DA TELA ---
  if (secondsLeft != lastSec || forceRedraw) {
    
    // Guardamos o estado do forceRedraw antes de resetá-lo para usar no QR Code
    bool isRedraw = forceRedraw;

    if (forceRedraw) { 
      tft.fillScreen(TFT_BLACK); 
      forceRedraw = false; 
    }

    // 1. MODO SEEDS (Frase de Recuperação)
    if (currentSeedIndex >= 0) {
      // (Seu código original de Seeds permanece igual)
      tft.fillScreen(TFT_BLACK);
      tft.setTextColor(TFT_ORANGE); 
      tft.drawCentreString(seeds[currentSeedIndex].label, 120, 10, 2);
      String phrase = seeds[currentSeedIndex].phrase;
      int wordCount = 0;
      String words[24];
      int start = 0, end = phrase.indexOf(' ');
      while (end != -1 && wordCount < 23) {
        words[wordCount++] = phrase.substring(start, end);
        start = end + 1; end = phrase.indexOf(' ', start);
      }
      words[wordCount++] = phrase.substring(start);
      tft.setTextSize(1);
      for(int i=0; i<wordCount; i++) {
        int col = i / 8; int row = i % 8;
        int x = 10 + (col * 80); int y = 45 + (row * 22);
        tft.setTextColor(TFT_DARKGREY); tft.setCursor(x, y); tft.print(String(i+1));
        tft.setTextColor(TFT_WHITE); tft.setCursor(x + 15, y); tft.print(words[i]);
      }
    } 
    
    // 2. MODO TOTP (Contas Ativas)
    else if (currentIndex != -1) {
      tft.setTextColor(TFT_YELLOW, TFT_BLACK); 
      tft.drawCentreString(accounts[currentIndex].name, 120, 30, 4);
      tft.setTextColor(TFT_GREEN, TFT_BLACK); 
      tft.drawCentreString(calcTOTP(accounts[currentIndex].secretBase32, epoch), 120, 100, 7);
      tft.fillRect(20, 170, 200, 10, TFT_BLACK);
      tft.fillRect(20, 170, map(secondsLeft, 0, 30, 0, 200), 10, (secondsLeft < 6) ? TFT_RED : TFT_BLUE);
      tft.setTextColor(TFT_WHITE, TFT_BLACK); 
      tft.drawCentreString("P: " + accounts[currentIndex].password, 120, 195, 2);
      drawInfo(epoch);
    } 

    // 3. MODO STANDBY (Melhorado para não piscar)
    else {
      if (displayMode == 1) {
        drawCreeper(); 
        drawInfo(epoch);
      } 
      else if (displayMode == 2) {
        // SÓ desenha o QR Code se a tela acabou de mudar (isRedraw)
        // Isso impede que ele redesenhe a cada segundo e fique piscando
        if (isRedraw) {
          drawWiFiScreen(); 
        }
      }
    }

    lastSec = secondsLeft;
  }
}
