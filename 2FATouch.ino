// ===== CREEPER AUTH v7.2.2 - DUAL STACK + NETWORK + SEED COLUMNS (VERSÃO FINAL) =====
#include "allconfigs.h"
#include <2FATouchCore.h>

void setup() {
  Serial.begin(115200);
  
  // ==========================================
  // IDIOMA GLOBAL / GLOBAL LANGUAGE
  // ==========================================
  // Descomente a linha do idioma desejado:
  // LANG_EN_US = Inglês / English
  // LANG_PT_BR = Português / Portuguese
  
  GlobalLanguage = LANG_PT_BR; // Ativação em Inglês
  //GlobalLanguage = LANG_PT_BR; // Ativação em Português


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
  tft.invertDisplay(true); // Inverte as cores da tela conforme solicitado
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
  Serial.println(tr("\n--- REDE CONECTADA ---", "\n--- NETWORK CONNECTED ---"));
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
  if (MDNS.begin("creeper")) {
    Serial.println("MDNS responder iniciado: http://creeper.local");
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
        css + "</head><body><div class='box'><h2>" + tr("GERENCIAR TOKENS", "MANAGE TOKENS") + "</h2>";
    for (int i = 0; i < accounts.size(); i++) {
      h += "<div style='margin-bottom:10px;'>" + accounts[i].name + " <br>";
      h += "<a href='/edit?id=" + String(i) + "' class='edit'>[E] " + tr("EDITAR", "EDIT") + "</a> ";
      h += "<a href='/del?id=" + String(i) +
           "' class='del'>[X] " + tr("EXCLUIR", "DELETE") + "</a></div>";
    }
    h += "<hr><a href='/add'>+ " + tr("NOVO TOKEN", "NEW TOKEN") + "</a><br>"
         "<a href='/'>" + tr("VOLTAR", "BACK") + "</a></div>"
         "<footer>Copyright 2025-2026 " + tr("Criado por Amauri Bueno dos Santos com apoio da Gemini.", "Created by Amauri Bueno dos Santos with support from Gemini.") + 
         " <a href='https://github.com/Annabel369/2FATouch' target='_blank' "
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
        Serial.printf("RAW X: %d \ RAW Y: %d\n", p.x, p.y);
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