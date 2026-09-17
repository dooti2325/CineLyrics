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
4. Start the server locally:
   ```bash
   uvicorn app:app --host 0.0.0.0 --port 8000
   ```
5. The first time you run this and play a song, it will open a browser window asking you to log in to Spotify and authorize the app.

### Cloud Deployment (Render)
CineLyric backend is also deployed live on **Render**:
- **Live URL**: [https://cinelyrics.onrender.com](https://cinelyrics.onrender.com/)
- **Secure WebSocket**: `wss://cinelyrics.onrender.com/ws`
- **Hosted Web Bluetooth Controller & Wi-Fi Provisioner**: [https://cinelyrics.onrender.com/ble](https://cinelyrics.onrender.com/ble)

> [!TIP]
> **HTTPS for Web Bluetooth:** The Web Bluetooth API strictly requires a Secure Context (HTTPS). You can use [https://cinelyrics.onrender.com/ble](https://cinelyrics.onrender.com/ble) on any mobile phone (Chrome for Android) or PC/Mac to pair and configure DeskBuddy without hosting a local web server!

#### Render Environment Variables
On Render dashboard, configure the following Environment Variables:
- `SPOTIFY_CLIENT_ID`: Your Spotify Developer Client ID
- `SPOTIFY_CLIENT_SECRET`: Your Spotify Developer Client Secret
- `SPOTIFY_REDIRECT_URI`: `https://cinelyrics.onrender.com/callback` (or your callback URI registered in Spotify Dashboard)
- `SPOTIFY_CACHE_INFO`: JSON string containing your OAuth token payload (enables persistent token caching across ephemeral Render restarts)
- `DEVICE_WS_TOKEN`: (Optional) Shared token for WebSocket authorization

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
2. The firmware is pre-configured to connect securely to the live Render cloud backend:
   ```cpp
   WEBSOCKET_HOST = "cinelyrics.onrender.com"; // Default Render cloud URL
   WEBSOCKET_PORT = 443;                       // Port 443 with SSL (WSS)
   ```
   *(If you wish to test with a local server, change `WEBSOCKET_HOST` to your local LAN IP and `WEBSOCKET_PORT` to 8000).*
3. **Wi-Fi Setup:** You can either:
   - **Method A (Zero Code / BLE Provisioning):** Power on the ESP32, open [https://cinelyrics.onrender.com/ble](https://cinelyrics.onrender.com/ble), pair with DeskBuddy, and submit your Wi-Fi credentials under Card 6.
   - **Method B (Compile-time fallback):** Copy `esp32/CineLyric/secrets.h.example` to `secrets.h` and fill in `SECRET_WIFI_SSID` and `SECRET_WIFI_PASSWORD`.
4. The default pins for I2C OLED are SDA=21, SCL=22. Adjust in `config.h` if needed.

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

## 4. DeskBuddy BLE

DeskBuddy broadcasts over Bluetooth Low Energy as `DeskBuddy-BLE`, offering 6 prioritized GATT services:

- **STATUS**: Read & notify DeskBuddy battery, BLE connection state, and device state (`IDLE`, `ACTIVE`, `SLEEPING`).
- **COMMAND**: Remote control to `wake`, `sleep`, or trigger custom emotion animations (`happy`, `sad`, `shock`, `love`, `laugh`, `vibe`, etc.).
- **NOTIFICATION**: Receive incoming notifications (`app`, `title`, `message`, `timestamp`) and render an animated pop-up banner on the OLED with auto-dismiss.
- **PHONE**: Phone battery percentage, charging state, and connection telemetry.
- **MEDIA**: Remote media triggers (`play/pause`, `next`, `previous`).
- **WIFI PROVISIONING**: Dynamically configure Wi-Fi SSID and Password over BLE, saved to ESP32 Flash (NVS Preferences) so any new user can connect DeskBuddy to their network without reflashing code!

### Testing DeskBuddy BLE & Wi-Fi Provisioning
Open `tools/ble_test.html` (or navigate to `http://localhost:8000/ble`) directly in any Web-Bluetooth supported browser (Google Chrome or Microsoft Edge on PC/Mac/Android):
1. Click **"Connect DeskBuddy"** to pair.
2. In **Card 6 (Wi-Fi Configuration)**, enter your Wi-Fi SSID & Password and click **"Save & Connect Buddy to Wi-Fi"**.
3. View real-time connection status on the OLED screen and Web Bluetooth console!

### Standalone Offline Mode
If Wi-Fi is unconfigured, out of range, or temporarily down, DeskBuddy automatically enters **Standalone Mode** (`MODE_FACE`), displaying interactive animated expressions and remaining fully controllable over BLE.
