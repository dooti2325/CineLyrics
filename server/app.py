import asyncio
import time
import json
import os
from fastapi import FastAPI, WebSocket, WebSocketDisconnect, Query, status
from fastapi.responses import HTMLResponse, FileResponse, RedirectResponse
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

@app.get("/login")
async def login():
    """Initiates Spotify OAuth flow by redirecting to Spotify's authorization page."""
    auth_url = spotify_manager.get_auth_url()
    if not auth_url:
        return HTMLResponse("<h3>Spotify Client ID or Secret not configured in .env</h3>", status_code=500)
    return RedirectResponse(auth_url)

@app.get("/callback")
async def callback(code: str = Query(None), error: str = Query(None)):
    """Handles OAuth redirect callback from Spotify."""
    return await handle_auth_code(code, error)

@app.get("/")
async def root(code: str = Query(None), error: str = Query(None)):
    """Handles homepage or root OAuth redirect if configured without /callback."""
    if code or error:
        return await handle_auth_code(code, error)

    # Render Dashboard Homepage
    is_auth = spotify_manager.is_authenticated()
    auth_badge = '<span style="color: #34d399; font-weight: bold;">Connected 🟢</span>' if is_auth else '<span style="color: #f87171; font-weight: bold;">Not Connected 🔴</span>'
    
    track_info = spotify_manager.get_track_info() if is_auth else None
    if track_info and track_info.get('is_playing'):
        track_html = f"""
        <div style="background: rgba(56, 189, 248, 0.1); border: 1px solid rgba(56, 189, 248, 0.3); border-radius: 10px; padding: 15px; margin: 20px 0;">
            <p style="margin: 0; font-size: 0.9rem; color: #38bdf8;">🎵 Currently Playing:</p>
            <h2 style="margin: 6px 0; font-size: 1.3rem;">{track_info['name']}</h2>
            <p style="margin: 0; color: #94a3b8;">{track_info['artist']} &bull; {track_info['album']}</p>
        </div>
        """
    elif is_auth:
        track_html = """
        <div style="background: rgba(255, 255, 255, 0.04); border: 1px solid rgba(255, 255, 255, 0.08); border-radius: 10px; padding: 15px; margin: 20px 0;">
            <p style="margin: 0; color: #94a3b8;">Spotify is connected. Start playing any song on your Spotify app to see real-time lyrics & animations!</p>
        </div>
        """
    else:
        track_html = """
        <div style="background: rgba(248, 113, 113, 0.1); border: 1px solid rgba(248, 113, 113, 0.3); border-radius: 10px; padding: 20px; margin: 20px 0;">
            <p style="margin: 0 0 12px 0; color: #f87171;">Spotify is not connected. Click below to link your Spotify account:</p>
            <a href="/login" style="display: inline-block; background: #1db954; color: white; padding: 10px 20px; border-radius: 20px; font-weight: bold; text-decoration: none;">🔗 Connect Spotify Account</a>
        </div>
        """

    return HTMLResponse(
        f"""
        <!DOCTYPE html>
        <html>
            <head>
                <title>CineLyric & DeskBuddy Dashboard</title>
                <meta name="viewport" content="width=device-width, initial-scale=1">
                <style>
                    body {{ font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; background: #0c0f17; color: #f1f5f9; padding: 30px; max-width: 800px; margin: auto; line-height: 1.5; }}
                    h1 {{ color: #38bdf8; margin-bottom: 4px; }}
                    .card {{ background: rgba(22, 28, 45, 0.7); border: 1px solid rgba(255, 255, 255, 0.08); border-radius: 14px; padding: 20px; margin-bottom: 20px; }}
                    code {{ background: #1e293b; padding: 3px 6px; border-radius: 4px; color: #38bdf8; font-size: 0.9em; }}
                    a {{ color: #38bdf8; text-decoration: none; }}
                    .btn {{ display: inline-block; background: rgba(255,255,255,0.08); border: 1px solid rgba(255,255,255,0.15); padding: 8px 16px; border-radius: 8px; color: #f1f5f9; margin-right: 10px; font-weight: 500; }}
                    .btn:hover {{ background: rgba(255,255,255,0.15); }}
                </style>
            </head>
            <body>
                <h1>🎬 CineLyric & DeskBuddy</h1>
                <p style="color: #94a3b8; margin-top: 0;">Real-time cinematic lyrics & emotion engine</p>

                <div class="card">
                    <div style="display: flex; justify-content: space-between; align-items: center;">
                        <span style="font-size: 1.1rem; font-weight: 600;">Spotify Status</span>
                        <span>{auth_badge}</span>
                    </div>
                    {track_html}
                    <div>
                        <a class="btn" href="/login">Re-authenticate Spotify</a>
                        <a class="btn" href="/ble">Open Web Bluetooth Controller</a>
                    </div>
                </div>

                <div class="card">
                    <h3 style="margin-top: 0; color: #818cf8;">📡 Endpoints & Provisioning</h3>
                    <p>WebSocket URL: <code>ws://&lt;host&gt;:8000/ws</code> or <code>wss://cinelyrics.onrender.com/ws</code></p>
                    <p>Active Clients Connected: <b>{len(ws_manager.active_connections)}</b> / {ws_manager.max_connections}</p>
                </div>
            </body>
        </html>
        """
    )

async def handle_auth_code(code: str, error: str):
    if error:
        return HTMLResponse(
            f"""
            <html>
                <body style="font-family: sans-serif; background: #0c0f17; color: #f1f5f9; padding: 40px; text-align: center;">
                    <h1 style="color: #f87171;">❌ Spotify Authorization Error</h1>
                    <p style="color: #94a3b8;">{error}</p>
                    <a href="/" style="color: #38bdf8;">Return to Dashboard</a>
                </body>
            </html>
            """, status_code=400
        )

    if code:
        success, message, token_json = spotify_manager.exchange_code(code)
        if success:
            return HTMLResponse(
                f"""
                <html>
                    <body style="font-family: sans-serif; background: #0c0f17; color: #f1f5f9; padding: 40px; text-align: center;">
                        <h1 style="color: #34d399;">✅ Spotify Connected Successfully!</h1>
                        <p style="color: #94a3b8;">CineLyric is now authorized and ready to stream synced lyrics to your DeskBuddy OLED.</p>
                        <div style="max-width: 650px; margin: 30px auto; background: #162032; padding: 20px; border-radius: 12px; text-align: left; border: 1px solid rgba(255,255,255,0.1);">
                            <p style="color: #38bdf8; font-weight: bold; margin-top: 0;">🚀 For Render Deployment (Permanent Session):</p>
                            <p style="font-size: 0.85rem; color: #94a3b8;">Because Render instances sleep after inactivity, copy this token and add it as an Environment Variable named <code>SPOTIFY_CACHE_INFO</code> on your Render dashboard:</p>
                            <textarea style="width: 100%; height: 90px; background: #0b0f19; color: #34d399; font-family: monospace; font-size: 0.8rem; padding: 8px; border-radius: 6px; border: 1px solid #334155;" readonly>{token_json}</textarea>
                        </div>
                        <p style="margin-top: 25px;"><a href="/" style="background: #2563eb; color: white; padding: 10px 20px; border-radius: 8px; text-decoration: none; font-weight: bold;">Go to Dashboard</a></p>
                    </body>
                </html>
                """
            )
        else:
            return HTMLResponse(
                f"""
                <html>
                    <body style="font-family: sans-serif; background: #0c0f17; color: #f1f5f9; padding: 40px; text-align: center;">
                        <h1 style="color: #f87171;">❌ Token Exchange Failed</h1>
                        <p style="color: #94a3b8;">{message}</p>
                        <p><a href="/login" style="color: #38bdf8;">Try Again</a></p>
                    </body>
                </html>
                """, status_code=400
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
