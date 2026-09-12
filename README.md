# CineLyric – Cinematic Lyrics Animation Engine

A real-time cinematic lyrics display system using an ESP32 and a 1.3" I2C OLED (SH1106). It connects to a Python FastAPI backend that syncs with Spotify and fetches lyrics from LRCLIB, sending them over WebSocket to the ESP32.

## Project Structure

- `server/` - Python FastAPI backend
- `esp32/CineLyric/` - Arduino IDE project for ESP32

## 1. Backend Setup

The backend connects to Spotify to track what you're currently playing, fetches synchronized lyrics from LRCLIB, and broadcasts them via WebSockets.

### Prerequisites
- Python 3.9+
- A Spotify Developer account

### Spotify Credentials
1. Go to the [Spotify Developer Dashboard](https://developer.spotify.com/dashboard).
2. Create a new App.
3. Add `http://127.0.0.1:8888/callback` as a Redirect URI in the app settings.
4. Copy your `Client ID` and `Client Secret`.

### Installation
1. Open a terminal in the `server/` directory.
2. Install dependencies:
   ```bash
   pip install -r requirements.txt
   ```
3. Copy `.env.example` to `.env` and fill in your Spotify credentials:
   ```bash
   cp .env.example .env
   ```
4. Start the server:
   ```bash
   uvicorn app:app --host 0.0.0.0 --port 8000
   ```
5. Note your computer's local IP address (e.g., `192.168.1.X`), you will need it for the ESP32 config.
6. The first time you run this and play a song, it will open a browser window asking you to log in to Spotify and authorize the app.

## 2. ESP32-C3 Setup (Arduino IDE)

### Prerequisites
- Arduino IDE installed
- ESP32 board package installed in Arduino IDE (`esp32` by Espressif, version 2.0.0+)
- Libraries to install via Arduino Library Manager:
  - **U8g2** (by oliver)
  - **ArduinoJson** (by Benoit Blanchon)
  - **WebSockets** (by Markus Sattler)

### Configuration
1. Open `esp32/CineLyric/CineLyric.ino` in the Arduino IDE.
2. Open `config.h` in the IDE (it should be in a tab).
3. Update `WIFI_SSID` and `WIFI_PASSWORD` with your Wi-Fi credentials.
4. Update `WEBSOCKET_HOST` with your server host.
5. Check pins in `config.h`:
   - Default I2C: **SDA = GPIO 8**, **SCL = GPIO 9** (or **GPIO 4 & 5** on some boards like SuperMini)
   - Touch/Button: **GPIO 3** (`TOUCH_ACTIVE_HIGH true` for TTP223 touch sensor, or `false` for tactile button)

### Wiring (ESP32-C3)

| Component Pin | ESP32-C3 Pin | Notes |
| ------------- | ------------ | ----- |
| OLED VCC      | 3.3V         | Power |
| OLED GND      | GND          | Ground |
| OLED SDA      | GPIO 8       | Or GPIO 4 (check `config.h`) |
| OLED SCL      | GPIO 9       | Or GPIO 5 (check `config.h`) |
| Touch / Button| GPIO 3       | Digital module (TTP223) or push button |

> **Note on ESP32-C3 Touch**: The ESP32-C3 does not feature internal analog touch peripherals (`touchRead`). Instead, connect an external digital capacitive touch sensor module (like **TTP223**) or a standard tactile push button to `GPIO 3`.

### Flashing (Arduino IDE Settings)
1. Connect your ESP32-C3 via USB.
2. Under **Tools > Board > esp32**, select **"ESP32C3 Dev Module"** (or your specific board model, e.g., "XIAO_ESP32C3").
3. Configure the following critical settings under **Tools**:
   - **USB CDC On Boot**: **Enabled** *(Required for Serial Monitor over USB on ESP32-C3)*
   - **CPU Frequency**: 160MHz
   - **Flash Frequency**: 80MHz
   - **Flash Mode**: QIO or DIO
   - **Upload Speed**: 460800 or 921600
4. Select the correct COM port.
5. Click **"Upload"**. *(If upload fails to connect, hold the `BOOT` button on the board, click Upload, and release when flashing starts).*

## 3. Usage
1. Make sure the Python server is running.
2. Power on the ESP32-C3. It will connect to Wi-Fi and then the WebSocket server.
3. Open Spotify on your phone or PC and start playing a song.
4. The lyrics will appear on the OLED with animations!
5. Tap the touch sensor / button to toggle animations or switch modes (Lyrics -> Visuals -> Robo Face).
# CineLyrics
