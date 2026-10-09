#  Creeper Auth v7.2.2 - Dual Stack & Crypto Vault

Support: https://www.youtube.com/watch?v=Y1EU-4kPpXc

🟩 🟩 🟩 🟩 🟩 🟩 🟩 🟩  
🟩 ⬛ ⬛ 🟩 🟩 ⬛ ⬛ 🟩  
🟩 ⬛ ⬛ 🟩 🟩 ⬛ ⬛ 🟩  
🟩 🟩 🟩 🟩 🟩 🟩 🟩 🟩  
🟩 🟩 🟩 ⬛ ⬛ 🟩 🟩 🟩  
🟩 ⬛ ⬛ ⬛ ⬛ ⬛ ⬛ 🟩  
🟩 ⬛ ⬛ ⬛ ⬛ ⬛ ⬛ 🟩  
🟩 ⬛ ⬛ 🟩 🟩 ⬛ ⬛ 🟩  
🟩 🟩 🟩 🟩 🟩 🟩 🟩 🟩 V7.2

<img width="229" height="76" alt="image" src="https://github.com/user-attachments/assets/e4cbc7b1-96ca-43fa-ad09-fae37f71b348" />

NOTE: Depending on your model, search the code to invert colors if your screen shows inverted colors (e.g. white background). In my case, true makes the color work, and for my brother's model, it's false depending on the manufacturing batch.

<img width="1920" height="1074" alt="image" src="https://github.com/user-attachments/assets/0bf54103-3280-442c-8bf3-52341bc16ef5" />



![WIN_20260106_03_42_48_Pro](https://github.com/user-attachments/assets/9bae5c3f-6ea4-4f8b-a3c6-ab38e6009a8d)

    // --- XPT2046 TOUCH SURGICAL CALIBRATION SETTINGS ---
    const int TOUCH_MIN_RAW_X = 2350;// default 200 or 300, tolerance 1000
    const int TOUCH_MAX_RAW_X = 3650;// default 3700 or 3000 or 3250
    const int TOUCH_MIN_RAW_Y = 200;
    const int TOUCH_MAX_RAW_Y = 3700;
    const bool TOUCH_INVERT_X = false; // Change to true if horizontal touch is mirrored
    const bool TOUCH_INVERT_Y = false; // Change to true if vertical touch is mirrored
    const bool TOUCH_SWAP_XY = false; // Change to true if X and Y axes are swapped

<img width="1040" height="503" alt="image" src="https://github.com/user-attachments/assets/07b3348c-3310-44e3-be96-e2cc8f625813" />
ESP32-2432S028R

https://github.com/user-attachments/assets/b97c8798-70a3-4e39-a5d2-1c58f077c853

# Dependencies
https://github.com/Annabel369/ESP32FTPServer


Configuration file for the TFT_eSPI library. Must be placed in the directory where the library is installed.

lv_conf.h
Configuration file for the LVGL library. Must be placed in the Arduino libraries directory.

Source: https://randomnerdtutorials.com/lvgl-cheap-yellow-display-esp32-2432s028r/

DNS NAME IPV6 if not, only via IPv4:

http://IP/login.html

or

http://creeper.local/login.html

<img width="1244" height="565" alt="image" src="https://github.com/user-attachments/assets/13e27c97-57c9-4a0f-b830-3d750f9c219d" />

    // 6. Specific IPv6 Devices Verification (Mickey's Devices)
    // Simply add the full IPv6 that appears in the Serial monitor inside the quotes
    if (clientIP == "fe80::your_pc_ipv6_here" || 
        clientIP == "fe80::your_phone_ipv6_here" || 
        clientIP == "fe80::your_tablet_ipv6_here" || 
        clientIP == "fe80::your_laptop_ipv6_here") {
      Serial.println("Access Granted: Recognized IPv6 Device");
      return true;
    }



https://github.com/Annabel369/PanelMinecraft/blob/main/User_Setup.h
# Copy the User_Setup.h file provided earlier and replace the existing file.
<img width="786" height="675" alt="image" src="https://github.com/user-attachments/assets/77f1cb7a-b2fc-4b38-a4ad-369ca865f97d" />

Look for a 3D printing project that simulates the original Cinepolis Creeper project

<img width="1628" height="778" alt="image" src="https://github.com/user-attachments/assets/207e916e-f8be-487d-a34d-79fa48163d60" />

https://www.crealitycloud.com/pt/model-detail/minecraft-creeper-bank-secret-storage?source=3&profileId=68dd2950aaaa058eab1acdcb


https://www.crealitycloud.com/pt/model-detail/minecraft-creeper-printing-model?source=5

           🟩🟩🟩  
           🟩🟩  
          🟩  
🟧🟧🟧🟧🟧🟧🟧🟧  
🟧⬛⬛🟧🟧⬛⬛🟧  
🟧⬛⬛🟧🟧⬛⬛🟧  
🟧🟧🟧🟧🟧🟧🟧🟧  
🟧🟧🟧⬛⬛🟧🟧🟧  
🟧⬛⬛⬛⬛⬛⬛🟧  
🟧⬛⬛⬛⬛⬛⬛🟧  
🟧⬛⬛🟧🟧⬛⬛🟧  
🟧🟧🟧🟧🟧🟧🟧🟧  



# 🟢 Creeper Auth v7.2.2 - Dual Stack & Crypto Vault
Creeper Auth v5.5 is an ESP32-based hardware security device. It combines a physical 2FA (TOTP) authenticator, a master key vault (Seeds), and a hybrid network security system (IPv4/IPv6). All of this comes with a Minecraft-themed interface and full management via SD Card and Web.

# 🚀 New Features in v7.2.2
Dual-Stack Support: Now operates on IPv4 and IPv6 simultaneously.

Dynamic Whitelist: New Python Agent that monitors your network and automatically authorizes your PC.

Seeds Vault 3.0: Visualization of recovery phrases (12/24 words) in 3 numbered columns on the display.

Web Network Management: Change Wi-Fi and security IPs without needing to modify the code or the SD card.

Colorful Interface: Management system with colorful buttons to prevent accidental deletions.

# 💻 The Security Agent (Python)
For the Add, Edit, and Delete functions to work, you must run the Python Agent on your computer. It acts as a "digital key" that tells the Creeper you are the legitimate owner of the device.

# 🛠️ System Prerequisites
For network recognition to work, Python needs low-level access to the network card:

Install Npcap 1.85: * Download and install Npcap 1.85.

Important: During installation, check the option "Install Npcap in WinPcap API-compatible Mode".

Install Python 3.x: Make sure Python is in your PATH.

Python Libraries: The script uses native libraries, but for advanced scanners, you might need:

Bash

pip install scapy
# 🛠️ Required Hardware
ESP32 (30 pins).

2.4" TFT Display (ILI9341 or ST7789).

Micro SD Card Module (SPI).

Micro SD Card (Formatted in FAT32).

# 📚 Arduino Libraries (IDE)


ESP32FtpServer: For remote file access. It comes with all of them together.

    ArduinoJson
    ESP32FtpServer
    ESP32Servo
    NTPClient
    SD
    TFT_eSPI
    TJpg_Decoder
    XPT2046_Touchscreen

Custom tuning using (nanu) that I developed and a text editor!


https://github.com/Annabel369/wnano


<img width="973" height="123" alt="image" src="https://github.com/user-attachments/assets/1b93315a-6d86-4f75-bfc9-715caf4bcf32" />

<img width="1017" height="511" alt="image" src="https://github.com/user-attachments/assets/111e1644-220c-43ba-b1aa-5229c66f0e0f" />
Do as shown in the photo, put a comment // on line 27 and on line 28 put:

     #include "../ESP32FtpServer/src/User_Setup_Custom.h"




# ⚙️ Initial Setup
Insert the SD card into your PC and create a config.txt file:

Plaintext

SSID=YourWiFiNetwork
PASS=YourPassword
MODO=REDE
IP_ALVO=192.168.100.
The Creeper will boot and show its IPv4 and IPv6 on the screen.

<img width="1504" height="575" alt="image" src="https://github.com/user-attachments/assets/85b3c213-bd00-45fe-8296-be44f813e2b7" />


Run the agente_creeper.py script on your PC to unlock access to the admin panel.

# 📂 SD File Structure
/config.txt: Stores Wi-Fi and IP rules.

/totp_secrets.txt: Stores tokens (Name=Secret=Password).

/seeds.txt: Stores recovery phrases (Name|Words).

# 🛡️ Security and Tips
Backup: The SD card is the only place your data lives. Make periodic backups of your .txt files.

Access Denied: If you see this message on the Web, make sure the Python Agent is running and your PC's IP was detected.

Viewing Seeds: In the vault, words are numbered from 1 to 24 and organized in 3 columns on the display to make typing them into wallets like MetaMask or Ledger easier.

<img width="629" height="589" alt="image" src="https://github.com/user-attachments/assets/243d8eeb-8935-4c58-8e77-f56b20226d0e" />
Example: 192.168.100.38,192.168.100.190,aa80::aa94:32aa:e867:623

Or block access to everyone on the home or corporate intranet.

<img width="391" height="466" alt="image" src="https://github.com/user-attachments/assets/8397a82f-05fd-4969-8b73-1cd4b8710e93" />

# FTP Access: You can store things, delete and take out (but you don't have access to the Original files generated by the system)



ftp://creeper:1234@192.168.100.49/

<img width="1191" height="327" alt="image" src="https://github.com/user-attachments/assets/c1a8d30b-6931-47af-a48e-48e8c2db86a6" />




# 📄 License
Project developed for personal use and for security and Minecraft enthusiasts. Use responsibly and keep your backups up to date!

<img width="1109" height="970" alt="image" src="https://github.com/user-attachments/assets/80c89aca-2570-4485-b574-4aa815d71cb5" />
# 🟩 Creeper Auth v7.2.2 - Physical Vault with YubiKey

This project transforms an ESP32 module with a touch screen (CYD - *Cheap Yellow Display*) into a **Physical 2FA Authenticator** inspired by the Creeper (Minecraft). The system requires a physical touch on a **YubiKey** to validate access, mechanically opening the Creeper's head through a Servo Motor and turning on an internal light via a Relay.

---

## 🛠️ Hardware Used

*   **Board:** ESP32-2432S028R (known as CYD - Cheap Yellow Display).
*   **Mechanics:** Servo Motor (e.g. SG90 or MG90S) acting as a mechanical arm to open the head.
*   **Lighting:** Relay Module triggering a lamp.
*   **Security:** YubiKey (configured with a Challenge-Response slot).

---

## ⚠️ Crucial Hardware Tips (For the CYD board)

The **ESP32-2432S028R** board has many internal components (screen, SD, touch, audio) that occupy most of the native ESP32 pins. To avoid burning components or causing conflicts (like a white screen or audio static), follow these strict rules:

### 1. Safe Pinout (Rear Connector P3)
Never use pin `26` on this board for external hardware, as it is permanently connected to the DAC (audio). 
On the back of the board, locate the white 4-pin connector (usually labeled **P3**). It exposes two perfectly safe pins to use:
*   **GPIO 22:** Used for the Relay Module signal (Light).
*   **GPIO 27:** Used for the PWM signal of the Servo Motor.

### 2. Power Supply (The "Catch")
The **P3** connector only provides **3.3V**. If you connect the Servo Motor or Relay directly to the `VCC` of P3, they will "jitter", freeze, or reset the ESP32 due to lack of electrical current.
*   **Signal (Data):** Connect the Yellow/Orange wires (signal) of the Servo and Relay to pins **22 and 27** of P3.
*   **Power (5V):** Pull the Red (VCC) and Black (GND) wires from your Servo/Relay directly from the **P1** connector (near the USB port), which provides native **5V**, or solder directly to the `VBUS` pin of the USB input. 

---

## 💻 Software Dependencies

For the Servo Motor to work on the ESP32 architecture without a compilation error (`LEDC_MAX_BIT_WIDTH` timer conflict), **DO NOT use the standard Arduino `Servo.h` library.**

1. Go to the **Library Manager** in the Arduino IDE.
2. Search and install the library: **`ESP32Servo`** (by Kevin Harrington, John K. Bennett).
3. In the code, the initial setup should be done like this:

```cpp
#include <ESP32Servo.h> 

Servo servoCreeper;
const int PINO_RELE_LUZ = 22; 
const int PINO_SERVO = 27;    

void setup() {
  // Ajuste de timers do ESP32 para o Servo
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  
  servoCreeper.setPeriodHertz(50); // Frequência de 50Hz
  servoCreeper.attach(PINO_SERVO, 500, 2400); 
  servoCreeper.write(0); // Inicia fechado
}
```

---

## 🔒 How the YubiKey Automation Works

The system does not open the door with a simple open web command. It requires a combined password physically validated by hardware.

1. **Waiting for Touch:** A Python script (`testa_yubikey_ykman.py`) runs on the local PC and "locks" waiting for the capacitive touch on the physical YubiKey.
2. **Triggering the Request:** After validating the challenge locally, the PC triggers a silent HTTP GET command to the ESP32 passing the secret credential:
   ```http
   GET http://<CREEPER_IP>/aprovado?senha=YourPasswordHere
   ```
3. **ESP32 Action:** The ESP32 receives the request and validates the password. If correct:
   * 🖼️ Loads the image from the SD Card and displays the success message on the screen.
   * 💡 Activates the Relay (GPIO 22) to turn on the internal light.
   * ⚙️ Turns the Servo Motor (GPIO 27) to 90 degrees, mechanically opening the head.
4. **Automatic Closing:** The main loop of the ESP32 monitors the time. Exactly **30 seconds** after opening, it cuts the power to the Relay, returns the Servo to 0 degrees, and returns the display to the standard Creeper face.

---

## 🛒 Parts and Hardware Guide (Mechanics and Automation)

Below is the complete list of physical components needed to assemble the Creeper mechanics (head opening, lighting) and the secret door automation.

---

Before starting this project, I recommend a 12V to 5V and 3.3V Distributor on the same GND to connect parts with different voltages without leaving the communication GND.

Buck Converter DC-DC 12V to 3.3V 5V 12V Triple Output 800mA High efficiency power supply for Arduino ESP8266 ESP32 Breadboard

<img width="1587" height="699" alt="image" src="https://github.com/user-attachments/assets/2198e241-821e-4f80-b3ca-3277d7886401" />

I also think a wire kit like this is important since it only comes with 1 wire that can be defective.

5 pcs/lot JST 1.25mm to DuPont 2. 54mm-1P cable connection/long terminal wire 10/20/30cm DuPont wire 2p 3p 4p 5p-12p


<img width="1382" height="443" alt="image" src="https://github.com/user-attachments/assets/073f3713-da71-4e00-82d8-2793353429e1" />

or wires

5 pcs/lot JST 1.25mm to DuPont 2. 54mm-1P cable connection/long terminal wire 10/20/30cm DuPont wire 2p 3p 4p 5p-12p

<img width="1596" height="586" alt="image" src="https://github.com/user-attachments/assets/a1a53d34-9dc8-45d7-92d0-0f4b67992f95" />




## 1. Relay Module (For the Light and Lock)

<img width="1596" height="586" alt="image" src="https://github.com/user-attachments/assets/38970eb4-7f46-44e0-8ad2-b5a7c6f03650" />

To connect directly to the CYD (ESP32) board pin, the ideal is to use a relay that works well with **3.3V** logic signals.

*   **What to look for in stores:** `1 Channel 3.3V Optocoupled Relay Module` or `3V Arduino Relay Module`.

> 💡 **Tip:** 5V relay modules with "Optocoupler" usually also work if you connect the 5V to the `VCC` pin and the 3.3V signal (from the ESP32) to the `IN` pin. However, opting for the native 3.3V module is safer and avoids voltage problems.

---

## 2. The Motor (Mechanical Arm)

<img width="1596" height="586" alt="image" src="https://github.com/user-attachments/assets/d8f96498-f808-473f-a319-89f6cef088d4" />

To lift the Creeper's head or open the door, you won't need a whole robotic arm. Just a strong Servo Motor and a metal rod will solve the problem cleanly and hidden.

*   **The Motor:** Search for `Micro Servo MG90S`. 
    *   *Note:* The acronym "MG" means *Metal Gear*. **Do not buy** the blue SG90 model (plastic), as its gears can strip or break with the continuous weight of the Creeper's head.
*   **The Rod (The "arm"):** Search for `RC Airplane Pushrod`. It is a thin, strong steel wire with a terminal (*Linkage Stopper*) that attaches to the servo motor horn and pushes/pulls the Creeper lid.

---

## 3. Door Lock (Secret Lock)
To automate the secret bedroom door with built-in security and aesthetics, the best option are solenoid type locks.

*   **What to look for in stores:** `12V Solenoid Electromagnetic Mini Lock` or `12V Solenoid Tongue Lock`.

*   
<img width="1596" height="586" alt="image" src="https://github.com/user-attachments/assets/b05e09eb-5d9d-40a7-8c3c-64f326f80edd" />




> ⚡ **Power Warning:** These locks draw a lot of current (Amperes) and operate at **12 Volts**. Your ESP32 CANNOT power them directly. You will need to use an external 12V power supply plugged into the wall. The Relay Module will only act as the "switch" to release this power.

### 🔌 Wiring Diagram (Solenoid Lock)

1. **12V Power Supply:** Plugged into the wall outlet.
2. **Positive Path:** The positive wire (+12V) from the power supply enters the Relay through the **`COM`** (Common) terminal and exits through the **`NO`** (Normally Open) terminal, going to the positive wire of the solenoid lock.
3. **Negative Path:** The negative wire (GND) of the lock connects directly to the negative wire of the 12V power supply.

🎯 **Final Action:** When the YubiKey is touched and validated, the ESP32 will open the Creeper's head and activate the Relay. The relay circuit closes, allowing the 12V to pass, pulling the metal tongue of the lock, unlocking the secret door instantly!

FPS OPTION https://github.com/Annabel369/FPSNVIA/tree/main
