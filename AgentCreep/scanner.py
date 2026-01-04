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
LOG_FILE = "agente_log.txt"

def log_status(mensagem):
    timestamp = time.strftime("%d/%m/%Y %H:%M:%S")
    try:
        with open(LOG_FILE, "a") as f:
            f.write(f"[{timestamp}] {mensagem}\n")
    except: pass
    print(f"[{timestamp}] {mensagem}", flush=True)

def get_network_tables():
    """ Captura tanto a tabela ARP (IPv4) quanto a Neighbors (IPv6) """
    ips_encontrados = [] # Lista de tuplas (ip, mac)
    
    try:
        # 1. PEGAR IPv4 (ARP)
        arp_out = subprocess.check_output("arp -a", shell=True).decode('cp850')
        ips_encontrados.extend(re.findall(r"(\d+\.\d+\.\d+\.\d+)\s+([0-9a-fA-F-]{17})", arp_out))
        
        # 2. PEGAR IPv6 (Neighbors)
        ipv6_out = subprocess.check_output("netsh interface ipv6 show neighbors", shell=True).decode('cp850')
        # Procura por IPs que começam com fe80 ou 2804 e o respectivo MAC
        ips_encontrados.extend(re.findall(r"([0-9a-fA-F:]+)\s+([0-9a-fA-F-]{17})", ipv6_out))
        
    except Exception as e:
        log_status(f"Erro ao ler tabelas de rede: {e}")
    
    return ips_encontrados

def scan_e_enviar():
    if not os.path.exists(JSON_FILE): return
    
    with open(JSON_FILE) as f:
        config = json.load(f)
    
    macs_autorizados = [d['mac'].lower().replace(':', '').replace('-', '') for d in config['dispositivos']]
    
    tabela_geral = get_network_tables()
    ips_para_liberar = []

    for ip, mac in tabela_geral:
        mac_limpo = mac.lower().replace('-', '').replace(':', '')
        if mac_limpo in macs_autorizados:
            if ip not in ips_para_liberar:
                ips_para_liberar.append(ip)
                # Oculta logs repetitivos, mas registra a autorização
                if ":" in ip: # É um IPv6
                    log_status(f"Detectado IPv6 Autorizado: {ip}")
                else:
                    log_status(f"Detectado IPv4 Autorizado: {ip}")

    if ips_para_liberar:
        # Criamos uma string única com todos os IPs v4 e v6 separados por vírgula
        msg = ",".join(ips_para_liberar)
        
        # Enviamos via IPv4 para o IP fixo do ESP32
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.sendto(msg.encode(), (ESP32_IP, UDP_PORT))
        log_status(f"Whitelist Dual-Stack enviada ({len(ips_para_liberar)} IPs)")
    else:
        log_status("Nenhum dispositivo autorizado encontrado na rede.")

# --- EXECUÇÃO ---
if __name__ == "__main__":
    # Garante que o script rode na pasta correta para achar o JSON
    os.chdir(os.path.dirname(os.path.abspath(__file__)))
    log_status("=== AGENTE CREEPER V2.5 (IPv4/IPv6) INICIADO ===")
    
    while True:
        scan_e_enviar()
        time.sleep(15)