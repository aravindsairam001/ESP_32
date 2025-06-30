# ESP32 Custom SSID Setup Portal

This project creates a **captive portal-like interface** on the ESP32 using the **ESP-IDF framework** that allows you to **customize the Wi-Fi Access Point (AP) SSID** through a web browser. It stores the SSID using NVS (non-volatile storage), allowing it to persist across reboots.

---

## 📌 Features

- ✅ ESP32 Access Point (AP) mode with configurable SSID  
- ✅ Web-based form to set a new SSID  
- ✅ Persistent SSID storage using NVS  
- ✅ Option to reset NVS data from the web interface  
- ✅ Pure ESP-IDF (no Arduino)

---

## 🧠 How It Works

1. ESP32 boots and reads the saved SSID from NVS.
2. Starts an AP with the saved SSID (or default `ESP32-Setup`).
3. Hosts a web server with:
   - `/` : Home page with SSID input form
   - `/save` : Endpoint to save new SSID and restart
   - `/reset` : Endpoint to erase NVS and restart
4. The user connects to the AP and sets a new SSID via the form.
5. New SSID is stored in NVS and used from the next boot.

---

## 🧰 Requirements

- ESP32 Development Board  
- ESP-IDF (v4.0 or newer recommended)  
- USB cable and terminal/serial monitor

---
