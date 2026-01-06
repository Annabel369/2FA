// ===== CREEPER AUTH v5.5 - DUAL STACK + NETWORK + SEED COLUMNS (VERSÃO FINAL) =====
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

int currentIndex = -1; 
int currentSeedIndex = -1; 
int lastSec = -1;
bool forceRedraw = true;

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

// --- Interface Visor ---
void drawCreeper() {
  tft.fillRect(60, 40, 120, 120, TFT_GREEN);
  tft.fillRect(80, 60, 25, 25, TFT_BLACK); tft.fillRect(135, 60, 25, 25, TFT_BLACK);
  tft.fillRect(105, 85, 30, 40, TFT_BLACK); tft.fillRect(90, 110, 60, 35, TFT_BLACK);
}

void drawInfo(unsigned long epoch) {
  time_t rawtime = (time_t)epoch;
  struct tm * ti = gmtime(&rawtime);
  char f_time[20];
  sprintf(f_time, "%02d/%02d %02d:%02d", ti->tm_mday, ti->tm_mon + 1, (ti->tm_hour + 21)%24, ti->tm_min);
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
  WiFi.enableIPv6(); 
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis()-start < 10000) delay(500);

  // --- PRINT NO CONSOLE (Monitor Serial) ---

  Serial.println("\n--- REDE CONECTADA ---");
  Serial.print("IPv4: "); Serial.println(WiFi.localIP());
  
  // No Core 3.x, usamos linkLocalIPv6() para o endereço fe80::
  Serial.print("IPv6: "); Serial.println(WiFi.linkLocalIPv6());
  Serial.println("-------------------------");

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

    Serial.println("!!! ACESSO NEGADO !!!");
    return false;
  };

  String css = R"rawliteral(
<style>
  body{background:#000;color:#0f0;font-family:monospace;text-align:center;margin:0;}
  footer{margin-top:40px;font-size:0.85em;color:#666;}
  .box{border:2px solid #0f0;padding:20px;display:inline-block;margin-top:20px;width:320px;}
  a{color:#0f0;text-decoration:none;border:1px solid #0f0;padding:5px;margin:3px;display:inline-block;}
  .edit{color:#ff0;border-color:#ff0;}
  .del{color:#f00;border-color:#f00;}
  input,select{background:#111;color:#0f0;border:1px solid #0f0;padding:8px;width:100%;margin:5px 0;box-sizing:border-box;}
  #creeper-container{width:120px;height:120px;position:relative;margin:20px auto; display:block;} /* Centralizei com margin: auto */
  #creeper-body{width:100%;height:100%;background-color:#00FF00;position:absolute;}
  .pixel{background-color:#000;position:absolute;width:10px;height:10px;}
</style>
)rawliteral";

  // --- Rotas ---
  server.on("/", [css, ehMickey]() {
    // Cabeçalho e CSS
    String h = "<!DOCTYPE html><html lang='pt'><head><link rel='shortcut icon' type='image/x-icon' href='https://www.minecraft.net/etc.clientlibs/minecraftnet/clientlibs/clientlib-site/resources/favicon.ico'><meta charset='UTF-8'>" + css + "</head><body>";
    
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

    h += "<h2>CREEPER AUTH v5.5</h2>";
    h += "<form action='/select'><select name='id'><option value='-1'>VISOR CREEPER</option>";
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
    String h = "<!DOCTYPE html><html lang='pt'><head><link rel='shortcut icon' type='image/x-icon' href='https://www.minecraft.net/etc.clientlibs/minecraftnet/clientlibs/clientlib-site/resources/favicon.ico'>><head><meta charset='UTF-8'>"+css+"</head><body><div class='box'><h2>GERENCIAR TOKENS</h2>";
    for(int i=0; i<accounts.size(); i++) {
      h += "<div style='margin-bottom:10px;'>" + accounts[i].name + " <br>";
      h += "<a href='/edit?id="+String(i)+"' class='edit'>[E] EDITAR</a> ";
      h += "<a href='/del?id="+String(i)+"' class='del'>[X] EXCLUIR</a></div>";
    }
    h += "<hr><a href='/add'>+ NOVO TOKEN</a><br><a href='/'>VOLTAR</a></div><footer>'Copyright' 2025-2026 Criado por Amauri Bueno dos Santos com apoio da Gemini. https://github.com/Annabel369/2FA</footer></body></html>";
    server.send(200, "text/html", h);
  });

  // --- NOVA ROTA: FORMULÁRIO PARA ADICIONAR ---
  server.on("/add", [css, ehMickey](){
    if(!ehMickey()) return server.send(403, "Negado");
    String h = "<!DOCTYPE html><html lang='pt'><head><link rel='shortcut icon' type='image/x-icon' href='https://www.minecraft.net/etc.clientlibs/minecraftnet/clientlibs/clientlib-site/resources/favicon.ico'><head><meta charset='UTF-8'>"+css+"</head><body><div class='box'><h2>NOVO TOKEN</h2><form method='POST' action='/reg'>";
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
    String h = "<!DOCTYPE html><html lang='pt'><head><link rel='shortcut icon' type='image/x-icon' href='https://www.minecraft.net/etc.clientlibs/minecraftnet/clientlibs/clientlib-site/resources/favicon.ico'><head><meta charset='UTF-8'>"+css+"</head><body><div class='box'><h2>EDITAR TOKEN</h2><form method='POST' action='/update?id="+String(id)+"'>'";
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
    String h = "<!DOCTYPE html><html lang='pt'><head><link rel='shortcut icon' type='image/x-icon' href='https://www.minecraft.net/etc.clientlibs/minecraftnet/clientlibs/clientlib-site/resources/favicon.ico'><head><meta charset='UTF-8'>"+css+"</head><body><div class='box'><h2>CONFIG REDE</h2><form method='POST' action='/net_save'>";
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
    String h = "<html><head><meta charset='UTF-8'>"+css+"</head><body><div class='box'><h2>CRYPTO VAULT</h2>";
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

  server.on("/select", [ehMickey](){
    if(ehMickey()){ currentIndex = server.arg("id").toInt(); currentSeedIndex = -1; forceRedraw = true; server.send(200, "text/html", "<script>location.href='/';</script>"); }
  });

  server.begin();
  ftpSrv.begin("creeper", "1234");
}

void loop() {
  server.handleClient();
  ftpSrv.handleFTP();
  timeClient.update();

  int packetSize = udpWhitelist.parsePacket();
  if (packetSize) {
    int len = udpWhitelist.read(packetBuffer, 255);
    if (len > 0) packetBuffer[len] = 0;
    dynamicWhitelist = String(packetBuffer);
  }

  unsigned long epoch = timeClient.getEpochTime();
  int secondsLeft = 30 - (epoch % 30);

  if (secondsLeft != lastSec || forceRedraw) {
    if (forceRedraw) { tft.fillScreen(TFT_BLACK); forceRedraw = false; }
    
    if (currentSeedIndex >= 0) {
      tft.fillScreen(TFT_BLACK);
      tft.setTextColor(TFT_ORANGE); tft.drawCentreString(seeds[currentSeedIndex].label, 120, 10, 2);
      
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
        int col = i / 8; 
        int row = i % 8;
        int x = 10 + (col * 80);
        int y = 45 + (row * 22);
        tft.setTextColor(TFT_DARKGREY); tft.setCursor(x, y); tft.print(String(i+1));
        tft.setTextColor(TFT_WHITE); tft.setCursor(x + 15, y); tft.print(words[i]);
      }
    } else {
      if (currentIndex == -1) { drawCreeper(); drawInfo(epoch); }
      else {
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
