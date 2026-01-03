// ===== CREEPER AUTH v5.2 - EDIÇÃO TOTAL & SEGURANÇA LOCAL =====
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
#include <vector>

#define SD_CS 5
TFT_eSPI tft = TFT_eSPI();
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000); 
WebServer server(80);
FtpServer ftpSrv;

// Configurações (Fallback)
String cfgSSID = "Maria Cristina 4G";
String cfgPASS = "1247bfam";
String cfgMODO = "REDE"; 
String cfgIP   = "192.168.100."; // Prefixo para Rede ou IP fixo para Único

struct TotpAccount { String name; String secretBase32; String password; };
struct SeedRecord { String label; String phrase; };

std::vector<TotpAccount> accounts;
std::vector<SeedRecord> seeds;

int currentIndex = -1; 
int currentSeedIndex = -1; 
int lastSec = -1;
bool forceRedraw = true;

// --- Gestão de Arquivos ---
void salvarConfig() {
  File f = SD.open("/config.txt", FILE_WRITE);
  if (f) { f.println("SSID=" + cfgSSID); f.println("PASS=" + cfgPASS); f.println("MODO=" + cfgMODO); f.println("IP_ALVO=" + cfgIP); f.close(); }
}

void carregarTudo() {
  if (!SD.begin(SD_CS)) return;
  // Carregar Config de Rede
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
  // Carregar Tokens 2FA
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
  // Carregar Seeds Cripto
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

// --- Telas e Visor ---
void drawCreeper() {
  tft.fillRect(60, 40, 120, 120, TFT_GREEN);
  tft.fillRect(80, 60, 25, 25, TFT_BLACK); tft.fillRect(135, 60, 25, 25, TFT_BLACK);
  tft.fillRect(105, 85, 30, 40, TFT_BLACK); tft.fillRect(90, 110, 60, 35, TFT_BLACK);
}

void drawInfo(unsigned long epoch) {
  time_t rawtime = (time_t)epoch;
  struct tm * ti = gmtime(&rawtime);
  char f_time[15];
  sprintf(f_time, "%02d/%02d %02d:%02d", ti->tm_mday, ti->tm_mon + 1, ti->tm_hour, ti->tm_min);
  
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawCentreString(f_time, 120, 175, 2);
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
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis()-start < 10000) delay(500);
  
  timeClient.begin();

  // Função de segurança recuperada v4
  auto ehMickey = []() {
    String clientIP = server.client().remoteIP().toString();
    if (cfgMODO == "REDE") return clientIP.startsWith(cfgIP);
    return clientIP == cfgIP;
  };

  String css = "<style>body{background:#000;color:#0f0;font-family:monospace;text-align:center;} .box{border:2px solid #0f0;padding:20px;display:inline-block;margin-top:20px;width:320px;} a{color:#0f0;text-decoration:none;border:1px solid #0f0;padding:5px;margin:3px;display:inline-block;} input,select{background:#111;color:#0f0;border:1px solid #0f0;padding:8px;width:100%;margin:5px 0;box-sizing:border-box;}</style>";

  // --- Rotas ---
  server.on("/", [css, ehMickey](){
    String h = "<html><head>"+css+"</head><body><div class='box'><h2>CREEPER AUTH v5.2</h2>";
    h += "<form action='/select'><select name='id'>";
    h += "<option value='-1'>VISOR CREEPER</option>";
    for(int i=0; i<accounts.size(); i++) h += "<option value='"+String(i)+"'>"+accounts[i].name+"</option>";
    h += "</select><input type='submit' value='OK'></form><br>";
    if(ehMickey()) {
      h += "<a href='/vault'>VAULT</a> <a href='/manage'>LISTA</a><br><a href='/add'>+ TOKEN</a> <a href='/network'>WI-FI & IP</a>";
    } else {
      h += "<p style='color:red'>ACESSO RESTRITO AO IP CONFIGURADO</p>";
    }
    h += "</div></body></html>";
    server.send(200, "text/html", h);
  });

  // Gestão de Rede com Edição (Recuperado e Melhorado)
  server.on("/network", [css, ehMickey](){
    if(!ehMickey()) return server.send(403, "Negado");
    String h = "<html><head>"+css+"</head><body><div class='box'><h2>CONFIG REDE</h2><form method='POST' action='/net_save'>";
    h += "SSID:<input name='ss' value='"+cfgSSID+"'>";
    h += "SENHA:<input name='pw' value='"+cfgPASS+"'>";
    h += "MODO:<select name='mo'><option value='REDE' "+(String(cfgMODO=="REDE"?"selected":""))+">REDE (Prefixo)</option>";
    h += "<option value='UNICO' "+(String(cfgMODO=="UNICO"?"selected":""))+">IP UNICO</option></select>";
    h += "IP/PREFIXO:<input name='ip' value='"+cfgIP+"'>";
    h += "<input type='submit' value='SALVAR E REINICIAR'></form><br><a href='/'>VOLTAR</a></div></body></html>";
    server.send(200, "text/html", h);
  });

  server.on("/net_save", HTTP_POST, [ehMickey](){
    if(ehMickey()){
      cfgSSID=server.arg("ss"); cfgPASS=server.arg("pw"); cfgMODO=server.arg("mo"); cfgIP=server.arg("ip");
      salvarConfig(); ESP.restart();
    }
  });

  // VAULT para Crypto Seeds
  server.on("/vault", [css, ehMickey](){
    if(!ehMickey()) return server.send(403, "Negado");
    String h = "<html><head>"+css+"</head><body><div class='box'><h2>CRYPTO VAULT</h2>";
    for(int i=0; i<seeds.size(); i++) {
       h += "<b>"+seeds[i].label+"</b> <a href='/view_seed?id="+String(i)+"'>VER</a> <a href='/del_seed?id="+String(i)+"' style='color:red'>X</a><br>";
    }
    h += "<hr><form method='POST' action='/reg_seed'>NOME:<input name='n'>SEED:<input name='s'><input type='submit' value='ADD SEED'></form><br><a href='/'>VOLTAR</a></div></body></html>";
    server.send(200, "text/html", h);
  });

  // Gerenciamento de Tokens (Com Edição v4)
  server.on("/manage", [css, ehMickey](){
    if(!ehMickey()) return server.send(403, "Negado");
    String h = "<html><head>"+css+"</head><body><div class='box'><h2>TOKENS 2FA</h2>";
    for(int i=0; i<accounts.size(); i++) {
      h += accounts[i].name + " <a href='/edit?id="+String(i)+"' style='color:#ff0'>[E]</a> <a href='/del?id="+String(i)+"' style='color:#f00'>[X]</a><br>";
    }
    h += "<br><a href='/'>VOLTAR</a></div></body></html>";
    server.send(200, "text/html", h);
  });

  server.on("/edit", [css, ehMickey](){
    int id = server.arg("id").toInt();
    TotpAccount acc = accounts[id];
    String h = "<html><head>"+css+"</head><body><div class='box'><h2>EDITAR</h2><form method='POST' action='/update?id="+String(id)+"'>";
    h += "NOME:<input name='u' value='"+acc.name+"'>SEED:<input name='s' value='"+acc.secretBase32+"'>PASS:<input name='p' value='"+acc.password+"'><input type='submit' value='OK'></form></div></body></html>";
    server.send(200, "text/html", h);
  });

  server.on("/update", HTTP_POST, [ehMickey](){
    int id = server.arg("id").toInt();
    String s = server.arg("s"); s.toUpperCase(); s.replace(" ", "");
    accounts[id] = {server.arg("u"), s, server.arg("p")};
    SD.remove("/totp_secrets.txt"); File f = SD.open("/totp_secrets.txt", FILE_WRITE);
    for(auto const& a : accounts) f.println(a.name + "=" + a.secretBase32 + "=" + a.password);
    f.close(); forceRedraw = true;
    server.send(200, "text/html", "<script>location.href='/manage';</script>");
  });

  // Rotas Auxiliares (Add, Del, Select...)
  server.on("/add", [css](){
    String h = "<html><head>"+css+"</head><body><div class='box'><h2>NOVO TOKEN</h2><form method='POST' action='/reg'>NOME:<input name='u'>SEED:<input name='s'>PASS:<input name='p'><input type='submit' value='OK'></form></div></body></html>";
    server.send(200, "text/html", h);
  });

  server.on("/reg", HTTP_POST, [](){
    String s = server.arg("s"); s.toUpperCase(); s.replace(" ", "");
    accounts.push_back({server.arg("u"), s, server.arg("p")});
    File f = SD.open("/totp_secrets.txt", FILE_APPEND); f.println(server.arg("u") + "=" + s + "=" + server.arg("p")); f.close();
    server.send(200, "text/html", "<script>location.href='/';</script>");
  });

  server.on("/del", [](){ accounts.erase(accounts.begin() + server.arg("id").toInt()); SD.remove("/totp_secrets.txt"); File f = SD.open("/totp_secrets.txt", FILE_WRITE); for(auto const& a : accounts) f.println(a.name + "=" + a.secretBase32 + "=" + a.password); f.close(); server.send(200, "text/html", "<script>location.href='/manage';</script>"); });
  server.on("/reg_seed", HTTP_POST, [](){ seeds.push_back({server.arg("n"), server.arg("s")}); SD.remove("/seeds.txt"); File f = SD.open("/seeds.txt", FILE_WRITE); for(auto const& sd : seeds) f.println(sd.label + "|" + sd.phrase); f.close(); server.send(200, "text/html", "<script>location.href='/vault';</script>"); });
  server.on("/del_seed", [](){ seeds.erase(seeds.begin() + server.arg("id").toInt()); SD.remove("/seeds.txt"); File f = SD.open("/seeds.txt", FILE_WRITE); for(auto const& sd : seeds) f.println(sd.label + "|" + sd.phrase); f.close(); server.send(200, "text/html", "<script>location.href='/vault';</script>"); });
  server.on("/view_seed", [](){ currentSeedIndex = server.arg("id").toInt(); forceRedraw = true; server.send(200, "text/html", "<script>location.href='/vault';</script>"); });
  server.on("/select", [](){ currentIndex = server.arg("id").toInt(); currentSeedIndex = -1; forceRedraw = true; server.send(200, "text/html", "<script>location.href='/';</script>"); });

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
    
    if (currentSeedIndex >= 0) {
      tft.fillScreen(TFT_BLACK); tft.setTextColor(TFT_ORANGE); tft.drawCentreString("SEED PHRASE", 120, 30, 2);
      tft.setTextColor(TFT_WHITE); tft.drawCentreString(seeds[currentSeedIndex].label, 120, 60, 4);
      tft.setTextSize(1); String p = seeds[currentSeedIndex].phrase;
      for(int i=0; i<p.length(); i+=18) tft.drawCentreString(p.substring(i, i+18), 120, 100+(i/18*20), 2);
    } else {
      if (currentIndex == -1) {
        drawCreeper();
        drawInfo(epoch);
      } else {
        tft.setTextColor(TFT_YELLOW, TFT_BLACK); tft.drawCentreString(accounts[currentIndex].name, 120, 30, 4);
        tft.setTextColor(TFT_GREEN, TFT_BLACK); tft.drawCentreString(calcTOTP(accounts[currentIndex].secretBase32, epoch), 120, 100, 7);
        tft.fillRect(20, 170, 200, 10, TFT_BLACK);
        tft.fillRect(20, 170, map(secondsLeft, 0, 30, 0, 200), 10, (secondsLeft < 6) ? TFT_RED : TFT_BLUE);
        tft.setTextColor(TFT_WHITE, TFT_BLACK); tft.drawCentreString("P: " + accounts[currentIndex].password, 120, 195, 2);
        drawInfo(epoch);
      }
    }
    lastSec = secondsLeft;
  }
}
