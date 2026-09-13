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

## 2. ESP32 Setup (Arduino IDE)

### Prerequisites
- Arduino IDE installed
- ESP32 board package installed in Arduino IDE
- Libraries to install via Arduino Library Manager:
  - **U8g2** (by oliver)
  - **ArduinoJson** (by Benoit Blanchon)
  - **WebSockets** (by Markus Sattler)

### Configuration
1. Open `esp32/CineLyric/CineLyric.ino` in the Arduino IDE.
2. Open `config.h` in the IDE (it should be in a tab).
3. Update `WIFI_SSID` and `WIFI_PASSWORD` with your Wi-Fi credentials.
4. Update `WEBSOCKET_HOST` with your computer's local IP address where the Python server is running.
5. The default pins for I2C are SDA=21, SCL=22. Adjust in `config.h` if needed.

### Wiring

| OLED Pin | ESP32 Pin |
| -------- | --------- |
| VCC      | 3.3V      |
| GND      | GND       |
| SDA      | GPIO21    |
| SCL      | GPIO22    |

### Flashing
1. Connect your ESP32 via USB.
2. Select your board (e.g., "DOIT ESP32 DEVKIT V1") and port in the Arduino IDE.
3. Click "Upload".

## 3. Usage
1. Make sure the Python server is running.
2. Power on the ESP32. It should connect to Wi-Fi and then the WebSocket server.
3. Open Spotify on your phone or PC and start playing a song.
4. The lyrics should appear on the OLED with animations!
# CineLyrics
