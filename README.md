<img width="229" height="76" alt="image" src="https://github.com/user-attachments/assets/e4cbc7b1-96ca-43fa-ad09-fae37f71b348" />
<img width="1040" height="503" alt="image" src="https://github.com/user-attachments/assets/07b3348c-3310-44e3-be96-e2cc8f625813" />
ESP32-2432S028R

https://github.com/user-attachments/assets/b97c8798-70a3-4e39-a5d2-1c58f077c853



# 🟢 Creeper Auth v5.5 - Dual Stack & Crypto Vault
O Creeper Auth v5.5 é um dispositivo de segurança de hardware baseado no ESP32. Ele combina um autenticador 2FA (TOTP) físico, um cofre de chaves mestras (Seeds) e um sistema de segurança de rede híbrido (IPv4/IPv6). Tudo isso com uma interface temática do Minecraft e gerenciamento total via SD Card e Web.

# 🚀 Novidades da Versão 5.5
Suporte Dual-Stack: Agora opera em IPv4 e IPv6 simultaneamente.

Whitelist Dinâmica: Novo Agente Python que monitora sua rede e autoriza seu PC automaticamente.

Cofre de Seeds 3.0: Visualização de frases de recuperação (12/24 palavras) em 3 colunas numeradas no visor.

Gestão de Rede via Web: Altere Wi-Fi e IPs de segurança sem precisar mexer no código ou no SD.

Interface Colorida: Sistema de gerenciamento com botões coloridos para evitar exclusões acidentais.

# 💻 O Agente de Segurança (Python)
Para que as funções de Adicionar, Editar e Excluir funcionem, você deve rodar o Agente Python no seu computador. Ele funciona como uma "chave digital" que avisa ao Creeper que você é o dono legítimo do dispositivo.

# 🛠️ Pré-requisitos do Sistema
Para o reconhecimento de rede funcionar, o Python precisa de acesso de baixo nível à placa de rede:

Instalar Npcap 1.85: * Baixe e instale o Npcap 1.85.

Importante: Durante a instalação, marque a opção "Install Npcap in WinPcap API-compatible Mode".

Instalar Python 3.x: Certifique-se de que o Python está no seu PATH.

Bibliotecas Python: O script usa bibliotecas nativas, mas para scanners avançados, você pode precisar:

Bash

pip install scapy
# 🛠️ Hardware Necessário
ESP32 (30 pinos).

Display TFT 2.4" (ILI9341 ou ST7789).

Módulo Cartão Micro SD (SPI).

Cartão Micro SD (Formatado em FAT32).

# 📚 Bibliotecas do Arduino (IDE)
TFT_eSPI: (Configurar User_Setup.h para os pinos do seu display).

NTPClient e WiFiUdp.

ESP32FtpServer: Para acesso remoto aos arquivos.

mbedtls: (Nativa do ESP32).

# ⚙️ Configuração Inicial
Insira o cartão SD no PC e crie um arquivo config.txt:

Plaintext

SSID=SuaRedeWifi
PASS=SuaSenha
MODO=REDE
IP_ALVO=192.168.100.
O Creeper iniciará e mostrará o IPv4 e o IPv6 na tela.

<img width="1504" height="575" alt="image" src="https://github.com/user-attachments/assets/85b3c213-bd00-45fe-8296-be44f813e2b7" />


Execute o script agente_creeper.py no seu PC para liberar o acesso ao painel administrativo.

# 📂 Estrutura de Arquivos no SD
/config.txt: Armazena Wi-Fi e regras de IP.

/totp_secrets.txt: Armazena tokens (Nome=Secret=Senha).

/seeds.txt: Armazena frases de recuperação (Nome|Palavras).

# 🛡️ Segurança e Dicas
Backup: O cartão SD é o único lugar onde seus dados moram. Faça cópias periódicas dos arquivos .txt.

Acesso Negado: Se você vir esta mensagem na Web, certifique-se de que o Agente Python está rodando e que o IP do seu PC foi detectado por ele.

Visualização de Seeds: No cofre, as palavras são numeradas de 1 a 24 e organizadas em 3 colunas no display para facilitar a digitação em carteiras como MetaMask ou Ledger.

<img width="629" height="589" alt="image" src="https://github.com/user-attachments/assets/243d8eeb-8935-4c58-8e77-f56b20226d0e" />
exemplo 192.168.100.38,192.168.100.190,aa80::aa94:32aa:e867:623

ou tapar acesso a todos da intranet da casa ou empresa

<img width="391" height="466" alt="image" src="https://github.com/user-attachments/assets/8397a82f-05fd-4969-8b73-1cd4b8710e93" />



# 📄 Licença
Projeto desenvolvido para uso pessoal e entusiastas de segurança e Minecraft. Use com responsabilidade e mantenha seus backups em dia!
