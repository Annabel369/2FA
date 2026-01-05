import os
import json
import socket
import time
import subprocess
import re

# --- CONFIGURAÇÃO ---
ESP32_IP = "192.168.100.49" 
UDP_PORT = 1234
JSON_FILE = "permitidos.json"
LOG_DIR = "logs"
DIAS_PARA_MANTER = 7

# Variável para evitar logs repetidos (Memória do Agente)
ultima_whitelist = ""

def log_status(mensagem, forcar=False):
    """Gera logs apenas se houver mudança ou se for forçado."""
    if not os.path.exists(LOG_DIR): os.makedirs(LOG_DIR)
    
    data_atual = time.strftime("%d-%m-%Y")
    nome_arquivo = os.path.join(LOG_DIR, f"log_{data_atual}.txt")
    timestamp = time.strftime("%d/%m/%Y %H:%M:%S")
    linha_log = f"[{timestamp}] {mensagem}\n"
    
    try:
        with open(nome_arquivo, "a", encoding="utf-8") as f:
            f.write(linha_log)
    except: pass
    
    print(f"[{timestamp}] {mensagem}", flush=True)

def get_network_tables():
    ips_encontrados = [] 
    try:
        arp_out = subprocess.check_output("arp -a", shell=True).decode('cp850')
        ips_encontrados.extend(re.findall(r"(\d+\.\d+\.\d+\.\d+)\s+([0-9a-fA-F-]{17})", arp_out))
        ipv6_out = subprocess.check_output("netsh interface ipv6 show neighbors", shell=True).decode('cp850')
        ips_encontrados.extend(re.findall(r"([0-9a-fA-F:]+)\s+([0-9a-fA-F-]{17})", ipv6_out))
    except: pass
    return ips_encontrados

def scan_e_enviar():
    global ultima_whitelist
    if not os.path.exists(JSON_FILE): return
    
    try:
        with open(JSON_FILE) as f: config = json.load(f)
    except: return
    
    macs_autorizados = [d['mac'].lower().replace(':', '').replace('-', '') for d in config['dispositivos']]
    tabela_geral = get_network_tables()
    ips_atuais = []

    for ip, mac in tabela_geral:
        mac_limpo = mac.lower().replace('-', '').replace(':', '')
        if mac_limpo in macs_autorizados:
            if ip not in ips_atuais:
                ips_atuais.append(ip)

    ips_atuais.sort() # Ordena para comparar as strings
    whitelist_string = ",".join(ips_atuais)

    # --- LÓGICA DE SILÊNCIO ---
    if whitelist_string != ultima_whitelist:
        # Houve mudança! Alguém entrou ou saiu.
        if whitelist_string == "":
            log_status("ALERTA: Nenhum dispositivo autorizado detectado na rede!")
        else:
            log_status(f"MUDANÇA NA REDE: IPs Autorizados agora: {whitelist_string}")
        
        ultima_whitelist = whitelist_string
    
    # O envio UDP continua acontecendo sempre para garantir o ESP32 atualizado
    if whitelist_string:
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            sock.sendto(whitelist_string.encode(), (ESP32_IP, UDP_PORT))
        except: pass

if __name__ == "__main__":
    os.chdir(os.path.dirname(os.path.abspath(__file__)))
    log_status("=== AGENTE CREEPER V2.9 (MODO INTELIGENTE) INICIADO ===", forcar=True)
    
    while True:
        scan_e_enviar()
        time.sleep(15)
