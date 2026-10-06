import subprocess
import sys

def testar_com_ykman():
    print("=== Teste Seguro com YubiKey Manager ===")
    print("Por favor, toque na sua YubiKey (o LED deve estar piscando agora)...")
    
    try:
        # Chama o comando ykman para enviar um "desafio" ao Slot 2
        # Como o Slot 2 foi configurado para exigir toque, o terminal trava aqui e a chave pisca.
        resultado = subprocess.run(
            ["ykman", "otp", "calculate", "2", "AAAA"],
            capture_output=True,
            text=True
        )
        
        # O código de retorno 0 significa que a YubiKey foi tocada e respondeu com sucesso
        if resultado.returncode == 0:
            print("\n✅ Toque capacitivo detectado pelo hardware!")
            print("Enviando sinal de aprovação via curl...")
            
            curl_res = subprocess.run(
                ["curl", "-s", "http://192.168.100.49/aprovado?senha=R7m2k9Xq"],
                capture_output=True,
                text=True
            )
            
            if curl_res.returncode == 0:
                print(f"✅ Comando executado com sucesso!")
                print(f"Resposta do servidor: {curl_res.stdout}")
            else:
                print(f"❌ Erro ao enviar o curl: {curl_res.stderr}")
                
        else:
            print(f"\n❌ Falha na comunicação com a YubiKey.")
            print(f"Erro do ykman: {resultado.stderr.strip()}")
            print("\nVerifique se você configurou o Slot 2 rodando:")
            print("ykman otp chalresp --generate --touch --force 2")
            
    except FileNotFoundError:
        print("❌ O comando 'ykman' não foi encontrado.")
        print("Por favor, instale rodando: sudo apt install yubikey-manager")
    except KeyboardInterrupt:
        print("\n\nCancelado pelo usuário.")
        sys.exit(0)

if __name__ == "__main__":
    testar_com_ykman()
