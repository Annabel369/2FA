import tkinter as tk
from tkinter import ttk, messagebox, filedialog, scrolledtext
import socket
import threading
import time
import os
import urllib.request
import urllib.error
import ssl


# ── Cliente Socket Raw para ESP32FtpServer ───────────────────────────────────
class ESP32RawFTP:
    def __init__(self, host, user="creeper", passwd="1234", timeout=10):
        self.host = host
        self.user = user
        self.passwd = passwd
        self.timeout = timeout
        self.cmd_sock = None

    def connect(self):
        self.cmd_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.cmd_sock.settimeout(self.timeout)
        self.cmd_sock.connect((self.host, 21))
        self._get_resp()

        # Login
        self.sendcmd(f"USER {self.user}")
        self.sendcmd(f"PASS {self.passwd}")

    def sendcmd(self, cmd):
        self.cmd_sock.sendall((cmd + "\r\n").encode())
        return self._get_resp()

    def _get_resp(self):
        try:
            return self.cmd_sock.recv(2048).decode(errors="ignore")
        except socket.timeout:
            return ""

    def _open_data_connection(self):
        """Abre a conexão no socket de dados passivo (PASV)."""
        pasv_resp = self.sendcmd("PASV")
        
        # Define porta padrão da biblioteca ESP32 (50009) como fallback
        data_port = 50009
        if "(" in pasv_resp and ")" in pasv_resp:
            try:
                nums = pasv_resp.split("(")[1].split(")")[0].split(",")
                data_port = (int(nums[4]) << 8) + int(nums[5])
            except Exception:
                pass

        data_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        data_sock.settimeout(self.timeout)
        data_sock.connect((self.host, data_port))
        return data_sock

    def list_files(self):
        # 1. Abre Socket de Dados
        data_sock = self._open_data_connection()

        # 2. Solicita a listagem no socket de controle
        self.cmd_sock.sendall(b"LIST\r\n")

        # 3. Lê a resposta do socket de dados
        raw_data = ""
        while True:
            try:
                chunk = data_sock.recv(2048).decode(errors="ignore")
                if not chunk:
                    break
                raw_data += chunk
            except socket.timeout:
                break

        data_sock.close()
        self._get_resp()  # Limpa resposta 226 Transfer Complete

        # Parse dos nomes dos arquivos
        files = []
        for line in raw_data.splitlines():
            line = line.strip()
            if not line:
                continue
            parts = line.split()
            if parts:
                files.append(parts[-1])  # Pega a última coluna (nome do arquivo)
        
        return files, raw_data

    def upload_file(self, local_path, remote_filename):
        """Envia um arquivo local para o Cartão SD do ESP32."""
        data_sock = self._open_data_connection()
        self.sendcmd(f"STOR {remote_filename}")

        with open(local_path, "rb") as f:
            while True:
                chunk = f.read(2048)
                if not chunk:
                    break
                data_sock.sendall(chunk)

        data_sock.close()
        return self._get_resp()

    def delete_file(self, filename):
        """Remove um arquivo do Cartão SD do ESP32."""
        return self.sendcmd(f"DELE {filename}")

    def quit(self):
        try:
            self.sendcmd("QUIT")
        except Exception:
            pass
        if self.cmd_sock:
            self.cmd_sock.close()


# ── Aplicação Tkinter ─────────────────────────────────────────────────────────
class ESP32ManagerApp:
    def __init__(self, root):
        self.root = root
        self.root.title("ESP32 Manager (2FATouch)")
        self.root.geometry("750x680")

        self.ip_var = tk.StringVar(value="192.168.100.49")
        self.tls_var = tk.BooleanVar(value=False)  # False = HTTP, True = HTTPS/TLS
        self.ftp_user = "creeper"
        self.ftp_pass = "1234"
        self.ftp_lock = threading.Lock()
        self.sending_udp = False

        self.setup_ui()

    def log(self, msg):
        timestamp = time.strftime("%H:%M:%S")
        full = f"[{timestamp}] {msg}\n"
        self.root.after(0, self._append_log, full)

    def _append_log(self, text):
        self.log_area.config(state=tk.NORMAL)
        self.log_area.insert(tk.END, text)
        self.log_area.see(tk.END)
        self.log_area.config(state=tk.DISABLED)

    def get_protocol_prefix(self):
        return "https" if self.tls_var.get() else "http"

    def setup_ui(self):
        header_frame = ttk.Frame(self.root, padding=10)
        header_frame.pack(fill=tk.X)

        ttk.Label(header_frame, text="ESP32 IP:").pack(side=tk.LEFT, padx=5)
        ttk.Entry(header_frame, textvariable=self.ip_var, width=15).pack(side=tk.LEFT, padx=5)

        # Toggle de Segurança (TLS vs HTTP)
        self.tls_chk = ttk.Checkbutton(
            header_frame, 
            text="Usar Conexão Segura (TLS/HTTPS)", 
            variable=self.tls_var, 
            command=self.update_tls_status
        )
        self.tls_chk.pack(side=tk.LEFT, padx=15)

        # Indicador de Status Visual
        self.lbl_status_tls = tk.Label(header_frame, text="🔓 MODO INTRANET (HTTP)", fg="#666666", font=("Helvetica", 9, "bold"))
        self.lbl_status_tls.pack(side=tk.RIGHT, padx=5)

        self.notebook = ttk.Notebook(self.root)
        self.notebook.pack(fill=tk.BOTH, expand=True, padx=10, pady=5)

        self.create_info_tab()
        self.create_cert_tab()
        self.create_functions_tab()
        self.create_ftp_tab()
        self.create_pc_monitor_tab()

        log_frame = ttk.LabelFrame(self.root, text="Log de Operações", padding=5)
        log_frame.pack(fill=tk.X, padx=10, pady=(0, 8))
        self.log_area = scrolledtext.ScrolledText(
            log_frame, height=6, state=tk.DISABLED, font=("Courier", 9)
        )
        self.log_area.pack(fill=tk.X)
        self.log("Aplicativo iniciado. Pronto.")

    def update_tls_status(self):
        if self.tls_var.get():
            self.lbl_status_tls.config(text="🔒 CONEXÃO SEGURA (TLS)", fg="#00aa00")
            self.log("Modo de Conexão alterado para: HTTPS / TLS")
        else:
            self.lbl_status_tls.config(text="🔓 MODO INTRANET (HTTP)", fg="#666666")
            self.log("Modo de Conexão alterado para: HTTP Intranet")

    def create_info_tab(self):
        frame = ttk.Frame(self.notebook, padding=15)
        self.notebook.add(frame, text="Informações e Chaves")
        info_text = (
            "📌 Informações do Sistema:\n\n"
            "🔑 Chave PIX (2FATouch.ino):\n810924f7-69b3-4116-8d8f-692e4a25c251\n\n"
            "💸 Wiser Pay:\nwise.com/pay/me/bryanbuenodossantoss\n\n"
            "📶 Wi-Fi (Padrão):\nSSID: Maria Cristina 4G\nSenha: 1247bfam\n\n"
            "© Copyright / Validade (getFooter):\n2025-2026 Criado por Amauri Bueno dos Santos"
        )
        t = tk.Text(frame, font=("Helvetica", 10))
        t.pack(fill=tk.BOTH, expand=True)
        t.insert(tk.END, info_text)
        t.config(state=tk.DISABLED)

    def create_cert_tab(self):
        frame = ttk.Frame(self.notebook, padding=15)
        self.notebook.add(frame, text="Certificados")
        cert_path = "/home/astral/Arduino/libraries/ESP32FtpServer/src/ESP32FtpServerCert.h"
        cert_content = "Certificado não encontrado."
        try:
            if os.path.exists(cert_path):
                with open(cert_path, "r") as f:
                    cert_content = f.read()
        except Exception as e:
            cert_content = f"Erro ao ler certificado: {e}"

        t = tk.Text(frame, font=("Helvetica", 10))
        t.pack(fill=tk.BOTH, expand=True)
        t.insert(tk.END, "📌 Certificado do Servidor FTP (ESP32FtpServerCert.h):\n\n" + cert_content)
        t.config(state=tk.DISABLED)

    def create_functions_tab(self):
        frame = ttk.Frame(self.notebook, padding=15)
        self.notebook.add(frame, text="Funções")
        ttk.Label(frame, text="Mudar Tela do ESP32:").grid(row=0, column=0, pady=10, sticky=tk.W)
        self.screen_var = tk.StringVar(value="CREEPER")
        screens = ["CREEPER", "WIFI", "PIX", "CLIMA", "WISE"]
        ttk.Combobox(frame, textvariable=self.screen_var, values=screens, state="readonly").grid(row=0, column=1, pady=10, padx=10)
        ttk.Button(frame, text="Enviar Comando", command=self._thread(self.change_esp_screen)).grid(row=0, column=2, pady=10, padx=10)

    def change_esp_screen(self):
        modo = self.screen_var.get()
        ip = self.ip_var.get()
        proto = self.get_protocol_prefix()
        url = f"{proto}://{ip}/exibir?modo={modo}"
        self.log(f"Requisição HTTP/HTTPS GET → {url}")
        try:
            ctx = ssl.create_default_context()
            ctx.check_hostname = False
            ctx.verify_mode = ssl.CERT_NONE

            req = urllib.request.urlopen(url, timeout=5, context=ctx if self.tls_var.get() else None)
            response = req.read().decode('utf-8')
            self.log(f"Resposta: {response}")
            self.root.after(0, messagebox.showinfo, "Sucesso", f"Comando enviado!\n{response}")
        except Exception as e:
            self.log(f"ERRO HTTP/HTTPS: {e}")
            self.root.after(0, messagebox.showerror, "Erro HTTP", str(e))

    def create_ftp_tab(self):
        frame = ttk.Frame(self.notebook, padding=15)
        self.notebook.add(frame, text="Cliente FTP")

        btn_frame = ttk.Frame(frame)
        btn_frame.pack(fill=tk.X, pady=5)

        ttk.Button(btn_frame, text="🔄 Listar Arquivos", command=self._thread(self.list_ftp_files)).pack(side=tk.LEFT, padx=4)
        ttk.Button(btn_frame, text="⬆️ Upload File", command=self.upload_ftp_file).pack(side=tk.LEFT, padx=4)
        ttk.Button(btn_frame, text="⬇️ Download (HTTP/HTTPS)", command=self.download_http_file).pack(side=tk.LEFT, padx=4)
        ttk.Button(btn_frame, text="❌ Excluir", command=self.delete_ftp_file).pack(side=tk.LEFT, padx=4)

        self.ftp_listbox = tk.Listbox(frame, width=60, height=15, font=("Courier", 10))
        self.ftp_listbox.pack(fill=tk.BOTH, expand=True, pady=5)

    def create_pc_monitor_tab(self):
        frame = ttk.Frame(self.notebook, padding=15)
        self.notebook.add(frame, text="Monitor de PC (UDP)")
        fields = [("FPS (0-999):", "fps_var", 60),
                  ("GPU Load % (0-100):", "gpu_var", 45),
                  ("GPU Temp °C (0-100):", "temp_var", 65)]
        for row, (label, attr, default) in enumerate(fields):
            ttk.Label(frame, text=label).grid(row=row, column=0, pady=6, sticky=tk.W)
            var = tk.IntVar(value=default)
            setattr(self, attr, var)
            ttk.Entry(frame, textvariable=var, width=10).grid(row=row, column=1, pady=6)

        self.udp_btn_var = tk.StringVar(value="Iniciar Envio UDP")
        ttk.Button(frame, textvariable=self.udp_btn_var, command=self.toggle_udp).grid(row=3, column=0, columnspan=2, pady=15)

    def _thread(self, fn):
        def wrapper(*args, **kwargs):
            threading.Thread(target=fn, args=args, kwargs=kwargs, daemon=True).start()
        return wrapper

    # ── Operações FTP Raw ────────────────────────────────────────────────────
    def list_ftp_files(self):
        with self.ftp_lock:
            try:
                ip = self.ip_var.get()
                self.log(f"Conectando via Sockets no ESP32 em {ip}...")
                
                client = ESP32RawFTP(ip, self.ftp_user, self.ftp_pass, timeout=8)
                client.connect()
                
                self.log("Buscando lista de arquivos...")
                files, raw = client.list_files()
                client.quit()

                self.log(f"Linhas brutas recebidas:\n{raw.strip()}")
                self.log(f"{len(files)} arquivo(s) processado(s).")

                def _update_ui():
                    self.ftp_listbox.delete(0, tk.END)
                    for f in files:
                        self.ftp_listbox.insert(tk.END, f)
                    if not files:
                        messagebox.showinfo("FTP", "Conexão OK, mas o diretório está vazio.")

                self.root.after(0, _update_ui)

            except Exception as e:
                self.log(f"ERRO ao listar FTP: {e}")
                self.root.after(0, messagebox.showerror, "Erro FTP", f"Erro na listagem:\n{e}")

    def upload_ftp_file(self):
        local_path = filedialog.askopenfilename(title="Selecione o arquivo para enviar ao ESP32")
        if not local_path:
            return

        remote_name = os.path.basename(local_path)
        self._thread(self._do_ftp_upload)(local_path, remote_name)

    def _do_ftp_upload(self, local_path, remote_filename):
        with self.ftp_lock:
            try:
                ip = self.ip_var.get()
                self.log(f"Enviando '{remote_filename}' para o ESP32...")
                client = ESP32RawFTP(ip, self.ftp_user, self.ftp_pass, timeout=12)
                client.connect()
                resp = client.upload_file(local_path, remote_filename)
                client.quit()

                self.log(f"Upload concluído: {resp.strip()}")
                self.root.after(0, messagebox.showinfo, "Sucesso", f"Arquivo '{remote_filename}' enviado com sucesso!")
                self.list_ftp_files()
            except Exception as e:
                self.log(f"ERRO no Upload FTP: {e}")
                self.root.after(0, messagebox.showerror, "Erro Upload", str(e))

    def delete_ftp_file(self):
        selection = self.ftp_listbox.curselection()
        if not selection:
            messagebox.showwarning("Excluir", "Selecione um arquivo na lista acima.")
            return

        filename = self.ftp_listbox.get(selection[0])
        if messagebox.askyesno("Confirmar Exclusão", f"Deseja realmente excluir '{filename}' do ESP32?"):
            self._thread(self._do_ftp_delete)(filename)

    def _do_ftp_delete(self, filename):
        with self.ftp_lock:
            try:
                ip = self.ip_var.get()
                self.log(f"Deletando '{filename}' do ESP32...")
                client = ESP32RawFTP(ip, self.ftp_user, self.ftp_pass, timeout=8)
                client.connect()
                resp = client.delete_file(filename)
                client.quit()

                self.log(f"Exclusão finalizada: {resp.strip()}")
                self.list_ftp_files()
            except Exception as e:
                self.log(f"ERRO ao excluir arquivo: {e}")
                self.root.after(0, messagebox.showerror, "Erro Excluir", str(e))

    # ── Download via HTTP / HTTPS ───────────────────────────────────────────
    def download_http_file(self):
        selection = self.ftp_listbox.curselection()
        if not selection:
            messagebox.showwarning("Download", "Selecione um arquivo na lista acima.")
            return

        filename = self.ftp_listbox.get(selection[0])
        save_path = filedialog.asksaveasfilename(initialfile=filename)

        if save_path:
            self._thread(self._do_http_download)(filename, save_path)

    def _do_http_download(self, filename, save_path):
        proto = self.get_protocol_prefix()
        url = f"{proto}://{self.ip_var.get()}/{filename.lstrip('/')}"
        self.log(f"Baixando por {proto.upper()}: {url}")
        try:
            ctx = ssl.create_default_context()
            ctx.check_hostname = False
            ctx.verify_mode = ssl.CERT_NONE

            with urllib.request.urlopen(url, timeout=30, context=ctx if self.tls_var.get() else None) as resp:
                data = resp.read()
            with open(save_path, "wb") as f:
                f.write(data)
            self.log(f"Download concluído: {filename} ({len(data)} bytes)")
            self.root.after(0, messagebox.showinfo, "Sucesso", f"Arquivo '{filename}' salvo com sucesso!")
        except Exception as e:
            self.log(f"ERRO no download {proto.upper()}: {e}")
            self.root.after(0, messagebox.showerror, "Erro Download", str(e))

    def toggle_udp(self):
        if not self.sending_udp:
            self.sending_udp = True
            self.udp_btn_var.set("Parar Envio UDP")
            threading.Thread(target=self.udp_loop, daemon=True).start()
        else:
            self.sending_udp = False
            self.udp_btn_var.set("Iniciar Envio UDP")

    def udp_loop(self):
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.log("UDP: envio iniciado (porta 5005)")
        while self.sending_udp:
            try:
                payload = f"{self.fps_var.get()},{self.gpu_var.get()},{self.temp_var.get()}"
                sock.sendto(payload.encode(), (self.ip_var.get(), 5005))
            except Exception as e:
                self.log(f"ERRO UDP: {e}")
            time.sleep(1)
        self.log("UDP: envio parado.")


if __name__ == "__main__":
    root = tk.Tk()
    app = ESP32ManagerApp(root)
    root.mainloop()