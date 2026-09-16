import asyncio
import time
import json
import os
from fastapi import FastAPI, WebSocket, WebSocketDisconnect
from fastapi.responses import HTMLResponse, FileResponse
from anyascii import anyascii

from spotify import SpotifyManager
from lyrics import LyricsManager
from ai_director import AIDirector
from websocket_server import WebSocketConnectionManager

app = FastAPI()
ws_manager = WebSocketConnectionManager()
spotify_manager = SpotifyManager()
lyrics_manager = LyricsManager()
ai_director = AIDirector()

# Global state
current_track_id = None
parsed_lyrics = []
last_lyric_time = -1

@app.on_event("startup")
async def startup_event():
    # Start the background polling task
    asyncio.create_task(spotify_polling_loop())

async def spotify_polling_loop():
    global current_track_id, parsed_lyrics, last_lyric_time
    last_playing_state = False
    
    while True:
        try:
            # Only poll if we have clients
            if ws_manager.active_connections:
                track_info = spotify_manager.get_track_info()
                
                if track_info:
                    current_playing_state = track_info['is_playing']
                    state_changed = (current_playing_state != last_playing_state)
                    last_playing_state = current_playing_state
                    
                    if current_playing_state:
                        # If track changed, fetch new lyrics
                        if track_info['id'] != current_track_id:
                            print(f"New track detected: {track_info['name']} by {track_info['artist']}")
                            current_track_id = track_info['id']
                            parsed_lyrics = lyrics_manager.fetch_lyrics(
                                track_info['name'], 
                                track_info['artist'],
                                track_info['album'],
                                track_info['duration_ms']
                            )
                            last_lyric_time = -999
                            print(f"Fetched {len(parsed_lyrics)} lyric lines")
                        
                        # Sync logic
                        progress_seconds = track_info['progress_ms'] / 1000.0
                        if parsed_lyrics:
                            current_lyric, next_lyric = lyrics_manager.get_current_and_next_line(parsed_lyrics, progress_seconds)
                        else:
                            # Fallback: display track name and artist if no synced lyrics
                            current_lyric = {"time": -1, "text": f"{track_info['name']} - {track_info['artist']}"}
                            next_lyric = {"time": -1, "text": ""}
                        
                        if not current_lyric:
                            current_lyric = {"time": -2, "text": f"{track_info['name']} - {track_info['artist']}"}
                        
                        # If we moved to a new lyric line or state changed, broadcast it
                        if current_lyric['time'] != last_lyric_time or state_changed:
                            last_lyric_time = current_lyric['time']
                            payload = ai_director.compose_packet(current_lyric, next_lyric, track_info)
                            print(f"Broadcasting: {payload['title']} | {payload['lyric']} [{payload['animation']}]")
                            await ws_manager.broadcast(payload)
                else:
                    if state_changed:
                        print("Spotify playback paused/stopped.")
                        # Send a stop event to the client
                        payload = {
                            "time": -1,
                            "lyric": "",
                            "next": "",
                            "animation": "fade",
                            "progress": 0,
                            "title": "",
                            "artist": "",
                            "bpm": 120.0,
                            "energy": 0.5,
                            "duration": 0,
                            "is_playing": False,
                            "beatStrength": 0.5,
                            "bass": 0.5,
                            "emotion": "Calm",
                            "scene": "Verse",
                            "secondary": "None",
                            "font": "Medium",
                            "x": 64,
                            "y": 32,
                            "shake": False,
                            "invert": False,
                            "particles": "None"
                        }
                        await ws_manager.broadcast(payload)
                        
        except Exception as e:
            print(f"Error in polling loop: {e}")
            
        # Wait before next poll (approx 2Hz is usually good enough for lyrics sync)
        await asyncio.sleep(0.5)

@app.websocket("/ws")
async def websocket_endpoint(websocket: WebSocket):
    await ws_manager.connect(websocket)
    try:
        # Immediately push current playing song to the new client!
        track_info = spotify_manager.get_track_info()
        if track_info and track_info.get('is_playing'):
            progress_seconds = track_info['progress_ms'] / 1000.0
            if parsed_lyrics:
                cur_l, nxt_l = lyrics_manager.get_current_and_next_line(parsed_lyrics, progress_seconds)
            else:
                cur_l = {"time": -1, "text": f"{track_info['name']} - {track_info['artist']}"}
                nxt_l = {"time": -1, "text": ""}
            if not cur_l:
                cur_l = {"time": -1, "text": f"{track_info['name']} - {track_info['artist']}"}
            payload = ai_director.compose_packet(cur_l, nxt_l, track_info)
            await websocket.send_text(json.dumps(payload))
            print(f"[WS] Pushed initial playback to new client: {payload['title']} | {payload['lyric']}")
    except Exception as e:
        print(f"[WS] Error pushing initial state: {e}")

    try:
        while True:
            # We don't expect messages from the ESP32, but we need to keep connection open
            data = await websocket.receive_text()
    except WebSocketDisconnect:
        ws_manager.disconnect(websocket)
    except Exception as e:
        print(f"WebSocket Error: {e}")
        ws_manager.disconnect(websocket)

@app.get("/")
async def get():
    return HTMLResponse(
        """
        <html>
            <head>
                <title>CineLyric Server</title>
            </head>
            <body>
                <h1>CineLyric Server is running!</h1>
                <p>Connect your ESP32 via WebSocket to ws://&lt;ip&gt;:8000/ws</p>
                <script>
                    var ws = new WebSocket(`ws://${location.host}/ws`);
                    ws.onmessage = function(event) {
                        console.log("Received: " + event.data);
                        var data = JSON.parse(event.data);
                        document.body.innerHTML += "<p>[" + data.animation + "] " + data.current + "</p>";
                    };
                </script>
            </body>
        </html>
        """
    )

@app.get("/ble")
async def get_ble():
    current_dir = os.path.dirname(__file__)
    for candidate in [
        os.path.join(current_dir, "ble_test.html"),
        os.path.join(current_dir, "..", "tools", "ble_test.html")
    ]:
        if os.path.exists(candidate):
            return FileResponse(os.path.abspath(candidate), media_type="text/html")
    return HTMLResponse("<h3>ble_test.html not found</h3>", status_code=404)

