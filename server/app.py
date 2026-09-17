import asyncio
import time
import json
import os
from fastapi import FastAPI, WebSocket, WebSocketDisconnect, Query, status
from fastapi.responses import HTMLResponse, FileResponse
from anyascii import anyascii

from spotify import SpotifyManager
from lyrics import LyricsManager
from ai_director import AIDirector
from websocket_server import WebSocketConnectionManager
from config import DEVICE_WS_TOKEN

app = FastAPI(title="CineLyric Server")
ws_manager = WebSocketConnectionManager(max_connections=10)
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
            # Only poll if we have active clients
            if ws_manager.active_connections:
                track_info = spotify_manager.get_track_info()
                
                if track_info:
                    current_playing_state = track_info['is_playing']
                    state_changed = (current_playing_state != last_playing_state)
                    last_playing_state = current_playing_state
                    
                    if current_playing_state:
                        # If track changed, asynchronously fetch new lyrics
                        if track_info['id'] != current_track_id:
                            print(f"[Loop] New track detected: {track_info['name']} by {track_info['artist']}")
                            current_track_id = track_info['id']
                            parsed_lyrics = await lyrics_manager.fetch_lyrics(
                                track_info['name'], 
                                track_info['artist'],
                                track_info['album'],
                                track_info['duration_ms']
                            )
                            last_lyric_time = -999
                            print(f"[Loop] Fetched {len(parsed_lyrics)} lyric lines")
                        
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
                            print(f"[Broadcast] {payload['title']} | {payload['lyric']} [{payload['animation']}]")
                            await ws_manager.broadcast(payload)
                else:
                    if state_changed:
                        print("[Loop] Spotify playback paused/stopped.")
                        last_playing_state = False
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
            print(f"[Loop] Error in polling loop: {e}")
            
        # Wait before next poll
        await asyncio.sleep(0.5)

@app.websocket("/ws")
async def websocket_endpoint(websocket: WebSocket, token: str = Query(None)):
    # Check optional client authorization token
    if DEVICE_WS_TOKEN and token != DEVICE_WS_TOKEN:
        print("[WS] Unauthorized WebSocket connection attempt rejected.")
        await websocket.close(code=status.WS_1008_POLICY_VIOLATION)
        return

    connected = await ws_manager.connect(websocket)
    if not connected:
        return

    try:
        # Immediately push current playing song to the new client
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
            print(f"[WS] Initial state pushed: {payload['title']} | {payload['lyric']}")
    except Exception as e:
        print(f"[WS] Error pushing initial state: {e}")

    try:
        while True:
            # Keep connection alive
            await websocket.receive_text()
    except WebSocketDisconnect:
        ws_manager.disconnect(websocket)
    except Exception as e:
        print(f"[WS] Connection error: {e}")
        ws_manager.disconnect(websocket)

@app.get("/")
async def get():
    return HTMLResponse(
        """
        <html>
            <head>
                <title>CineLyric Server</title>
                <style>
                    body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; background: #0b0f19; color: #f1f5f9; padding: 40px; }
                    h1 { color: #38bdf8; }
                    code { background: #1e293b; padding: 4px 8px; border-radius: 4px; color: #38bdf8; }
                    a { color: #818cf8; text-decoration: none; }
                </style>
            </head>
            <body>
                <h1>CineLyric Server is running!</h1>
                <p>WebSocket endpoint: <code>ws://&lt;host&gt;:8000/ws</code></p>
                <p>BLE Web Controller: <a href="/ble">Open Web Bluetooth Controller</a></p>
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
