// ===== CREEPER AUTH v3.0 - FINAL & ESTILIZADO =====
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

#define SD_CS 5
TFT_eSPI tft = TFT_eSPI();
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000); 
WebServer server(80);
FtpServer ftpSrv;

struct TotpAccount { 
  String name; 
  String secretBase32; 
  String password; 
};

std::vector<TotpAccount> accounts;
int currentIndex = -1; 
int lastSec = -1;
bool forceRedraw = true;

// --- Lógica de Arquivo (Limpa e Segura) ---
void salvarNoSD() {
  SD.remove("/totp_secrets.txt");
  File f = SD.open("/totp_secrets.txt", FILE_WRITE);
  if (f) {
    for (auto const& acc : accounts) {
      f.println(acc.name + "=" + acc.secretBase32 + "=" + acc.password);
    }
    f.close();
  }
}

void carregarContas() {
  accounts.clear();
  if (!SD.begin(SD_CS)) return;
  File f = SD.open("/totp_secrets.txt", FILE_READ);
  if (!f) return;
  while (f.available()) {
    String line = f.readStringUntil('\n'); line.trim();
    int e1 = line.indexOf('=');
    int e2 = line.indexOf('=', e1 + 1);
    if (e1 > 0 && e2 > 0) {
      accounts.push_back({line.substring(0, e1), line.substring(e1+1, e2), line.substring(e2+1)});
    }
  }
  f.close();
}

// --- TOTP Criptografia ---
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

// --- Desenho TFT ---
void drawCreeper() {
  tft.fillRect(60, 40, 120, 120, TFT_GREEN);
  tft.fillRect(80, 60, 25, 25, TFT_BLACK); tft.fillRect(135, 60, 25, 25, TFT_BLACK);
  tft.fillRect(105, 85, 30, 40, TFT_BLACK);
  tft.fillRect(90, 110, 60, 35, TFT_BLACK);
  tft.setTextColor(TFT_WHITE); tft.drawCentreString("MINECRAFT AUTH", 120, 10, 2);
  tft.setTextColor(TFT_GREEN); tft.drawCentreString(WiFi.localIP().toString(), 120, 210, 2);
}

// --- Setup ---
void setup() {
  Serial.begin(115200);
  SPI.begin(18, 19, 23, SD_CS);
  tft.init(); tft.setRotation(0); 
  tft.invertDisplay(true); // Garante o Verde do Creeper
  tft.fillScreen(TFT_BLACK);
  
  carregarContas();
  
  WiFi.begin("Maria Cristina 4G", "1247bfam");
  while (WiFi.status() != WL_CONNECTED) delay(500);
  timeClient.begin();

  // Segurança: Libera para toda a sua rede 192.168.100.x
  auto ehMickey = []() {
    return server.client().remoteIP().toString().startsWith("192.168.100.");
  };

  // --- Rotas Web Estilizadas ---
  String css = "<style>body{background:#000;color:#0f0;font-family:monospace;text-align:center;}"
               ".box{border:2px solid #0f0;padding:20px;display:inline-block;margin-top:20px;}"
               "a{color:#0f0;text-decoration:none;border:1px solid #0f0;padding:5px;}"
               "input,select{background:#111;color:#0f0;border:1px solid #0f0;padding:5px;}</style>";

  server.on("/", [css, ehMickey](){
    String h = "<html><head>"+css+"</head><body><div class='box'><h2>CREEPER AUTH</h2>";
    h += "<form action='/select'><select name='id'><option value='-1'>CREEPER</option>";
    for(int i=0; i<accounts.size(); i++) h += "<option value='"+String(i)+"'>"+accounts[i].name+"</option>";
    h += "</select> <input type='submit' value='OK'></form><br>";
    if(ehMickey()) h += "<a href='/add'>+ ADD</a> | <a href='/manage'>SETTINGS</a>";
    h += "</div></body></html>";
    server.send(200, "text/html", h);
  });

  server.on("/manage", [css, ehMickey](){
    if(!ehMickey()) return server.send(403, "Negado");
    String h = "<html><head>"+css+"</head><body><div class='box'><h2>GERENCIAR</h2>";
    for(int i=0; i<(int)accounts.size(); i++) {
      h += "<div style='margin-bottom:15px; border-bottom:1px solid #222; padding-bottom:5px;'>";
      h += "<b>" + accounts[i].name + "</b><br>";
      h += "<a href='/edit?id="+String(i)+"' style='color:#ff0'>[EDITAR]</a> "; 
      h += "<a href='/del?id="+String(i)+"' style='color:#f00' onclick='return confirm(\"Apagar?\")'>[EXCLUIR]</a>";
      h += "</div>";
    }
    h += "<br><a href='/'>VOLTAR AO INICIO</a></div></body></html>";
    server.send(200, "text/html", h);
  });

  server.on("/del", [ehMickey](){
    if(ehMickey() && server.hasArg("id")){
      int id = server.arg("id").toInt();
      if(id>=0 && id<accounts.size()){ accounts.erase(accounts.begin()+id); salvarNoSD(); currentIndex=-1; forceRedraw=true; }
    }
    server.send(200, "text/html", "<script>location.href='/manage';</script>");
  });

  server.on("/add", [css, ehMickey](){
    if(!ehMickey()) return server.send(403, "Negado");
    String h = "<html><head>"+css+"</head><body><div class='box'><h2>NOVO TOKEN</h2>"
               "<form method='POST' action='/reg'>NOME:<br><input name='u'><br>SEED:<br><input name='s'><br>PASS:<br><input name='p'><br><br>"
               "<input type='submit' value='GUARDAR'></form></div></body></html>";
    server.send(200, "text/html", h);
  });

  server.on("/edit", [css, ehMickey](){
    if(!ehMickey()) return server.send(403, "Negado");
    if(!server.hasArg("id")) return server.send(400, "ID faltando");
    
    int id = server.arg("id").toInt();
    if(id < 0 || id >= (int)accounts.size()) return server.send(404, "Conta nao encontrada");
    
    TotpAccount acc = accounts[id];
    String h = "<html><head>"+css+"</head><body><div class='box'><h2>EDITAR TOKEN</h2>";
    h += "<form method='POST' action='/update?id="+String(id)+"'>";
    h += "NOME:<br><input name='u' value='"+acc.name+"'><br>";
    h += "SEED:<br><input name='s' value='"+acc.secretBase32+"'><br>";
    h += "PASS:<br><input name='p' value='"+acc.password+"'><br><br>";
    h += "<input type='submit' value='SALVAR ALTERACOES'></form><br>";
    h += "<a href='/manage'>VOLTAR</a></div></body></html>";
    server.send(200, "text/html", h);
  });

  // --- ROTA PARA PROCESSAR A ATUALIZAÇÃO ---
  server.on("/update", HTTP_POST, [ehMickey](){
    if(ehMickey() && server.hasArg("id")){
      int id = server.arg("id").toInt();
      if(id >= 0 && id < (int)accounts.size()){
        String s = server.arg("s"); s.toUpperCase(); s.replace(" ", "");
        accounts[id] = {server.arg("u"), s, server.arg("p")};
        salvarNoSD(); // Salva a edição no arquivo do SD
        forceRedraw = true; // Atualiza a tela TFT imediatamente
      }
    }
    server.send(200, "text/html", "<script>location.href='/manage';</script>");
  });

  server.on("/reg", HTTP_POST, [ehMickey](){
    if(ehMickey()){
      String s = server.arg("s"); s.toUpperCase(); s.replace(" ", "");
      accounts.push_back({server.arg("u"), s, server.arg("p")});
      salvarNoSD();
    }
    server.send(200, "text/html", "<script>location.href='/';</script>");
  });

  server.on("/select", [](){
    currentIndex = server.arg("id").toInt();
    forceRedraw = true;
    server.send(200, "text/html", "<script>location.href='/';</script>");
  });

  server.begin();
  ftpSrv.begin("creeper", "1234");
}

void loop() {
  server.handleClient();
  ftpSrv.handleFTP();
  timeClient.update();
  unsigned long epoch = timeClient.getEpochTime();
  int secondsLeft = 30 - (epoch % 30);

  if (secondsLeft != lastSec || forceRedraw) {
    if (forceRedraw) { tft.fillScreen(TFT_BLACK); forceRedraw = false; }
    if (currentIndex == -1) {
      drawCreeper();
    } else if (currentIndex < accounts.size()) {
      tft.setTextColor(TFT_YELLOW, TFT_BLACK); tft.drawCentreString(accounts[currentIndex].name, 120, 40, 4);
      tft.setTextColor(TFT_GREEN, TFT_BLACK); tft.drawCentreString(calcTOTP(accounts[currentIndex].secretBase32, epoch), 120, 110, 7);
      tft.fillRect(20, 180, 200, 10, TFT_BLACK);
      tft.fillRect(20, 180, map(secondsLeft, 0, 30, 0, 200), 10, (secondsLeft < 6) ? TFT_RED : TFT_BLUE);
      tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawCentreString("Senha: " + accounts[currentIndex].password, 120, 210, 2);
    }
    lastSec = secondsLeft;
  }
}
