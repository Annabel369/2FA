VOCÊ ESTA NA VERSÃO ANTIGA V6 QUE GUARDO PARA LEMBRA DA PRIMEIRAS TENTATIVAS


## 🚀 Novidades e Atualizações da Versão (v7.2.2)

NEW VERSION CUSTOM 7.2.2 https://github.com/Annabel369/2FATouch


### 🖥️ Suporte ao Hardware ESP32-2432S028R (CYD - Cheap Yellow Display)
* **Mapeamento de Pinos Sem Conflitos:** Ajustado para funcionar perfeitamente com a placa CYD de 2.8" sem interferir no áudio (DAC) ou na comunicação com o cartão SD.
* **Configuração da Tela (`TFT_eSPI`):** O arquivo de configuração necessário para o display da CYD foi disponibilizado diretamente no repositório. Basta copiar o arquivo `User_Setup.h` e substituir na sua biblioteca `TFT_eSPI`:
  👉 [Acessar User_Setup.h no Repositório](https://github.com/Annabel369/PanelMinecraft/blob/main/User_Setup.h)
* **Dependência FTP:** Atualizada a integração do servidor de arquivos local para transferência de logs e mídias via SD usando a biblioteca [ESP32FTPServer](https://github.com/Annabel369/ESP32FTPServer).

---

### 🌐 Rede Dual-Stack (IPv4 & IPv6) e Filtro de Dispositivos
* **Suporte Nativo mDNS / IPv6:** Acesso facilitado na rede local digitando `http://creeper.local/` (compatível com Dual-Stack IPv4/IPv6).
* **Whitelist Dinâmica de Dispositivos:** Verificação e liberação automática de acesso via código para endereços IPv6 específicos registrados (PCs, Celulares e Tablets da rede local):

```cpp
// Verificação de Dispositivos IPv6 Específicos
if (clientIP == "fe80::seu_ipv6_pc_aqui" || 
    clientIP == "fe80::seu_ipv6_celular_aqui" || 
    clientIP == "192.168.100.38" || 
    clientIP == "aa80::aa94:32aa:e867:623") {
  Serial.println("Acesso Liberado: Dispositivo Reconhecido");
  return true;
}
