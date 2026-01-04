<img width="229" height="76" alt="image" src="https://github.com/user-attachments/assets/e4cbc7b1-96ca-43fa-ad09-fae37f71b348" />
<img width="1040" height="503" alt="image" src="https://github.com/user-attachments/assets/07b3348c-3310-44e3-be96-e2cc8f625813" />
ESP32-2432S028R


# Creeper Auth v5.2 é um dispositivo de segurança completo: faz o 2FA (TOTP) sincronizado com o tempo global, gerencia suas redes Wi-Fi e IPs de segurança pelo cartão SD, mostra data/hora e ainda serve como um cofre para suas chaves de criptomoedas.

Seu projeto no GitHub vai ficar excelente com essas funcionalidades. É um hardware útil, seguro e muito estiloso com o tema do Minecraft.

A lgumas dicas para o futuro:
Backup: Sempre guarde uma cópia do arquivo totp_secrets.txt e seeds.txt do seu SD em um lugar seguro, caso o cartão estrague.

Case: Se você for colocar em uma caixinha, lembre-se de deixar o acesso ao cartão SD livre para facilitar as edições manuais se necessário.

Atualização: Como você está usando o pool.ntp.org, ele é muito confiável, mas se um dia o relógio parar, verifique apenas a sua conexão com a internet.


# 🟢 Creeper Auth v5.2 - 2FA & Crypto Vault
Este é um autenticador de dois fatores (TOTP) físico baseado no ESP32, com tema de Minecraft (Creeper). Ele sincroniza via NTP com o tempo universal (UTC 0) e permite gerenciar tokens e frases de recuperação (Seeds) via interface Web segura.

# 🚀 Funcionalidades
TOTP 2FA: Gera códigos de 6 dígitos compatíveis com Google, Discord, GitHub, etc.

Crypto Vault: Espaço seguro para armazenar Seed Phrases (12/24 palavras).

Sincronia Global: Usa NTP para tempo universal, funcionando em qualquer lugar do mundo.

Gestão Web: Interface para Adicionar, Deletar e Editar contas e configurações de rede.

Segurança de Rede: Bloqueio por IP ou prefixo de rede (Intranet).

Armazenamento: Tudo é salvo no cartão SD (contas, seeds e Wi-Fi).

🛠️ Hardware Necessário
ESP32 (30 pinos).

Display TFT 2.4" (Driver ILI9341 ou ST7789).

Módulo de Cartão SD (SPI).

Cartão Micro SD.

📚 Bibliotecas Utilizadas
TFT_eSPI (Configurar o User_Setup.h para o seu display).

NTPClient e WiFiUdp.

ESP32FtpServer (Para acesso remoto aos arquivos do SD).

mbedtls (Nativa do ESP32 para criptografia HMAC-SHA1).

Dica para os usuários: "Lembre-se de configurar o arquivo config.txt no seu cartão SD com seu Wi-Fi ou use o painel de controle via IP exibido na tela do Creeper."

🟢 Creeper Auth: ESP32 TOTP Authenticator
Creeper Auth é um gerador de tokens de autenticação de dois fatores (2FA) estilo Google Authenticator, construído com um ESP32 e uma tela TFT, tematizado com a estética do Creeper (Minecraft).

Este dispositivo não apenas gera códigos TOTP (Time-based One-Time Password) para serviços como Discord, GitHub e Gmail, mas também funciona como um mini painel de controle acessível via navegador, protegido por IP.

🚀 Funcionalidades
Sincronização em Tempo Real: Utiliza protocolo NTP para garantir que os códigos estejam sempre perfeitamente sincronizados com o horário mundial.

Interface Minecraft: Visual customizado de um Creeper que muda para os tokens conforme a seleção.

Gerenciamento Web Seguro: Interface administrativa via navegador com CSS inspirado no Minecraft (Black & Green).

Segurança por IP: O painel de adição e exclusão de contas só pode ser acessado pelo IP do seu computador pessoal (evitando que intrusos na rede apaguem suas contas).

Persistência no SD Card: Armazena suas seeds e senhas de forma segura em um cartão MicroSD.

Servidor FTP Integrado: Permite gerenciar os arquivos do cartão SD sem precisar removê-lo do ESP32.

🛠️ Hardware Necessário
ESP32 (DevKit V1 ou similar).

Display TFT (ILI9341 ou ST7789) utilizando a biblioteca TFT_eSPI.

Módulo de Cartão MicroSD.

Conexão Wi-Fi (para sincronização de tempo e servidor web).

💻 Como Funciona
O código utiliza a biblioteca mbedtls nativa do ESP32 para realizar cálculos criptográficos HMAC-SHA1. Ele decodifica a chave Base32 fornecida pelos serviços (Discord, Google, etc.) e gera o código de 6 dígitos baseado no tempo UNIX (Epoch).

Visual do Painel Web
A interface administrativa foi desenhada para parecer um terminal de computador antigo dentro do universo Minecraft: fundo preto profundo, fontes monoespaçadas e detalhes em verde neon.

📥 Instalação
Clone este repositório.

Configure suas credenciais Wi-Fi e o IP do seu PC no código principal.

Configure o arquivo User_Setup.h da biblioteca TFT_eSPI para os pinos do seu display.

Suba o código para o seu ESP32.

Acesse o IP que aparecerá na tela do Creeper para começar a cadastrar suas contas.
