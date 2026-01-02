// ===== Autenticador TOTP Creeper + Gerenciador de Senhas (VERSÃO FINAL) =====
#include <WiFi.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <FS.h>
#include <SD.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <WebServer.h>
#include <ESP32FtpServer.h>
#include "mbedtls/md.h"

#define SD_CS     5

TFT_eSPI tft = TFT_eSPI();
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", -10800, 60000); 
WebServer server(80);
FtpServer ftpSrv;

// IP do seu computador para proteger o gerenciamento
const String ipAutorizado = "192.168.100.218";

struct TotpAccount { 
  String name; 
  String secretBase32; 
  String password; 
};

std::vector<TotpAccount> accounts;
int currentIndex = -1; 
int lastSec = -1;
String lastCode = "";
bool forceRedraw = true;

// --- Persistência de Tela ---
void salvarUltimaTela(int id) {
  File f = SD.open("/tela.txt", FILE_WRITE);
  if (f) { f.print(id); f.close(); }
}

int lerUltimaTela() {
  if (!SD.exists("/tela.txt")) return -1;
  File f = SD.open("/tela.txt", FILE_READ);
  if (!f) return -1;
  int id = f.readString().toInt();
  f.close();
  if (id >= (int)accounts.size()) return -1;
  return id;
}

// --- Funções de Arquivo ---
void salvarTudoNoSD() {
  SD.remove("/totp_secrets.txt");
  File f = SD.open("/totp_secrets.txt", FILE_WRITE);
  if (f) {
    for (auto const& acc : accounts) {
      f.println(acc.name + "=" + acc.secretBase32 + "=" + acc.password);
    }
    f.close();
  }
}

void loadTotpAccounts() {
  accounts.clear();
  if (!SD.begin(SD_CS)) return;
  File f = SD.open("/totp_secrets.txt", FILE_READ);
  if (f) {
    while (f.available()) {
      String line = f.readStringUntil('\n'); line.trim();
      int firstEq = line.indexOf('=');
      int secondEq = line.indexOf('=', firstEq + 1);
      if (firstEq > 0 && secondEq > 0) {
        accounts.push_back({line.substring(0, firstEq), line.substring(firstEq+1, secondEq), line.substring(secondEq+1)});
      }
    }
    f.close();
  }
}

// --- Desenho e Interface ---
void drawCustomCreeper(int x, int y, int tam) {
    int pixelSize = tam / 12; 
    tft.fillRect(x, y, tam, tam, TFT_GREEN); 
    tft.fillRect(x + 20, y + 10, pixelSize * 3, pixelSize * 3, TFT_BLACK); 
    tft.fillRect(x + 70, y + 10, pixelSize * 3, pixelSize * 3, TFT_BLACK); 
    tft.fillRect(x + 50, y + 50, pixelSize * 2, pixelSize * 2, TFT_BLACK); 
    tft.fillRect(x + 30, y + 60, pixelSize * 6, pixelSize * 3, TFT_BLACK); 
    tft.fillRect(x + 30, y + 90, pixelSize * 2, pixelSize * 2, TFT_BLACK); 
    tft.fillRect(x + 70, y + 90, pixelSize * 2, pixelSize * 2, TFT_BLACK); 
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("Minecraft", 120, 200, 2);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.drawString(WiFi.localIP().toString(), 120, 220, 2);
}

void drawUI(const String& title, const String& code, int secondsLeft, String passwordText) {
  if (forceRedraw) {
    tft.fillScreen(TFT_BLACK);
    if (currentIndex == -1) {
      drawCustomCreeper(60, 40, 120); 
    } else {
      tft.setTextColor(TFT_WHITE); tft.setTextDatum(MC_DATUM);
      tft.drawString("AUTH TOKEN", 120, 20, 2);
      tft.drawLine(10, 35, 230, 35, TFT_DARKGREY);
      tft.setTextColor(TFT_YELLOW); tft.drawString(title, 120, 70, 4);
    }
    forceRedraw = false; lastCode = "";
  }
  if (currentIndex >= 0 && !accounts.empty()) {
    if (code != lastCode) {
      tft.fillRect(10, 100, 220, 60, TFT_BLACK); 
      tft.setTextColor(TFT_GREEN); tft.drawString(code, 120, 130, 7);
      lastCode = code;
    }
    int barWidth = map(secondsLeft, 0, 30, 0, 200);
    tft.drawRect(19, 179, 202, 12, TFT_WHITE);
    tft.fillRect(20, 180, barWidth, 10, (secondsLeft < 6) ? TFT_RED : TFT_BLUE);
    tft.fillRect(20 + barWidth, 180, 200 - barWidth, 10, TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.setTextDatum(MC_DATUM);
    tft.drawString("Senha: " + passwordText, 120, 210, 2);
  }
}

// --- Lógica TOTP ---
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
    if (bitsLeft >= 8) {
      bitsLeft -= 8;
      if (outCount < maxOut) output[outCount++] = (buffer >> bitsLeft) & 0xFF;
    }
  }
  return outCount;
}

String generateTOTP(const String& secretBase32, unsigned long epoch) {
  unsigned long counter = epoch / 30;
  uint8_t msg[8];
  for (int i = 7; i >= 0; i--) { msg[i] = counter & 0xFF; counter >>= 8; }
  uint8_t key[64]; int keyLen = base32Decode(secretBase32, key, sizeof(key));
  if (keyLen <= 0) return "ERRO!";
  uint8_t hash[20];
  const mbedtls_md_info_t* mdInfo = mbedtls_md_info_from_type(MBEDTLS_MD_SHA1);
  mbedtls_md_hmac(mdInfo, key, keyLen, msg, 8, hash);
  int offset = hash[19] & 0x0F;
  uint32_t bin_code = ((hash[offset] & 0x7F) << 24) | ((hash[offset+1] & 0xFF) << 16) |
                      ((hash[offset+2] & 0xFF) << 8) | (hash[offset+3] & 0xFF);
  char buf[7]; snprintf(buf, sizeof(buf), "%06d", bin_code % 1000000);
  return String(buf);
}

void setup() {
  Serial.begin(115200);
  SPI.begin(18, 19, 23, SD_CS); 
  loadTotpAccounts();
  
  tft.init(); 
  tft.setRotation(0);
  tft.invertDisplay(true); // <--- CORRIGE CORES INVERTIDAS
  tft.fillScreen(TFT_BLACK);
  
  WiFi.begin("Maria Cristina 4G", "1247bfam");
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  
  timeClient.begin();
  timeClient.update();
  currentIndex = lerUltimaTela();
  forceRedraw = true;

  // Middleware de segurança (SÓ VOCÊ ACESSA SETTINGS)
  auto ehMickey = []() { return server.client().remoteIP().toString() == ipAutorizado; };

  // --- CSS ESTILO CREEPER (MINECRAFT) ---
  String style = "<style>"
    "body { background-color: #0a0a0a; color: #00FF00; font-family: 'Courier New', monospace; text-align: center; margin: 0; padding: 20px; }"
    "h2 { color: #00FF00; text-shadow: 2px 2px #004400; letter-spacing: 2px; }"
    "select, input { background: #1a1a1a; color: #00FF00; border: 2px solid #00FF00; padding: 10px; margin: 10px; border-radius: 5px; font-weight: bold; }"
    "a { color: #00FF00; text-decoration: none; border: 1px solid #00FF00; padding: 5px 10px; border-radius: 3px; transition: 0.3s; }"
    "a:hover { background: #00FF00; color: #000; }"
    ".card { border: 2px solid #00FF00; background: #111; padding: 20px; display: inline-block; border-radius: 10px; box-shadow: 0 0 15px rgba(0, 255, 0, 0.2); }"
    "table { margin: auto; border-collapse: collapse; width: 100%; max-width: 400px; }"
    "td { padding: 10px; border-bottom: 1px solid #222; }"
    ".del-btn { color: #ff4444 !important; border-color: #ff4444 !important; }"
    ".del-btn:hover { background: #ff4444 !important; color: #fff !important; }"
    "</style>";

  server.on("/", [style, ehMickey](){
    String h = "<html><head>" + style + "</head><body>";
    h += "<div class='card'><h2>CREEPER AUTH</h2>";
    h += "<form action='/select'><select name='id'>";
    h += "<option value='-1'>[ MODO CREEPER ]</option>";
    for (int i=0; i<(int)accounts.size(); i++) h += "<option value='"+String(i)+"' "+(currentIndex==i?"selected":"")+">"+accounts[i].name+"</option>";
    h += "</select><br><input type='submit' value='MUDAR TELA'></form><hr>";
    if(ehMickey()) {
      h += "<p><a href='/add'>+ ADICIONAR</a> &nbsp; <a href='/manage'>SETTINGS</a></p>";
    } else {
      h += "<p style='color:#444; font-size:10px;'>READ-ONLY MODE</p>";
    }
    h += "</div></body></html>";
    server.send(200, "text/html", h);
  });

  

  server.on("/delete", [ehMickey](){
    if(ehMickey() && server.hasArg("id")){
      int id = server.arg("id").toInt();
      if(id >= 0 && id < (int)accounts.size()){
        accounts.erase(accounts.begin() + id);
        salvarTudoNoSD();
        currentIndex = -1; forceRedraw = true;
      }
    }
    server.send(200, "text/html", "Removido! <a href='/manage'>Voltar</a>");
  });

  server.on("/manage", [style, ehMickey](){
    if(!ehMickey()){ server.send(403, "Acesso Negado"); return; }
    String h = "<html><head>" + style + "</head><body>";
    h += "<div class='card'><h2>GERENCIAR</h2><table>";
    if(accounts.empty()) h += "<tr><td>Vazio...</td></tr>";
    for (int i=0; i<(int)accounts.size(); i++) {
      h += "<tr><td>"+accounts[i].name+"</td><td><a href='/delete?id="+String(i)+"' class='del-btn' onclick='return confirm(\"Apagar?\")'>X</a></td></tr>";
    }
    h += "</table><br><a href='/'>VOLTAR</a></div></body></html>";
    server.send(200, "text/html", h);
  });

  // --- ROTA ADICIONAR (ADD) ---
  server.on("/add", [style, ehMickey](){
    if(!ehMickey()){ server.send(403, "Negado"); return; }
    String h = "<html><head>" + style + "</head><body><div class='card'>";
    h += "<h2>NOVO TOKEN</h2><form method='POST' action='/register'>";
    h += "NOME:<br><input name='user'><br>SEED:<br><input name='seed'><br>SENHA:<br><input name='pass'><br>";
    h += "<input type='submit' value='SALVAR NO SD'></form><br><a href='/'>VOLTAR</a></div></body></html>";
    server.send(200, "text/html", h);
  });

  server.on("/register", HTTP_POST, [ehMickey](){
    if(ehMickey()){
      String u = server.arg("user"); 
      String s = server.arg("seed"); s.replace(" ", ""); s.toUpperCase();
      String p = server.arg("pass");
      File f = SD.open("/totp_secrets.txt", FILE_APPEND);
      if (f) { f.println(u + "=" + s + "=" + p); f.close(); }
      loadTotpAccounts();
    }
    server.send(200, "text/html", "Salvo! <a href='/'>Voltar</a>");
  });

  server.on("/select", [](){
    if (server.hasArg("id")) { currentIndex = server.arg("id").toInt(); salvarUltimaTela(currentIndex); forceRedraw = true; }
    server.send(200, "text/html", "OK! <a href='/'>Voltar</a>");
  });

  server.begin();
  ftpSrv.begin("creeper", "1234");
}

void loop() {
  server.handleClient();
  ftpSrv.handleFTP();
  if (millis() % 3600000 == 0) timeClient.update();
  unsigned long epoch = timeClient.getEpochTime();
  int secondsLeft = 30 - (epoch % 30);
  if (secondsLeft != lastSec || forceRedraw) {
    String name = "", code = "", pass = "";
    if (currentIndex >= 0 && currentIndex < (int)accounts.size()) {
      name = accounts[currentIndex].name;
      code = generateTOTP(accounts[currentIndex].secretBase32, epoch);
      pass = accounts[currentIndex].password;
    }
    drawUI(name, code, secondsLeft, pass);
    lastSec = secondsLeft;
  }
}