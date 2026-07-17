import asyncio
import time
from fastapi import FastAPI, WebSocket, WebSocketDisconnect
from fastapi.responses import HTMLResponse
from anyascii import anyascii

from spotify import SpotifyManager
from lyrics import LyricsManager
from animation import AnimationSelector
from websocket_server import WebSocketConnectionManager

app = FastAPI()
ws_manager = WebSocketConnectionManager()
spotify_manager = SpotifyManager()
lyrics_manager = LyricsManager()
animation_selector = AnimationSelector()

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
    
    while True:
        try:
            # Only poll if we have clients
            if ws_manager.active_connections:
                track_info = spotify_manager.get_track_info()
                
                if track_info and track_info['is_playing']:
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
                        last_lyric_time = -1
                        print(f"Fetched {len(parsed_lyrics)} lyric lines")
                    if current_track_id and not parsed_lyrics:
                        # No lyrics found for the new track
                        await ws_manager.broadcast({
                            "time": -1,
                            "current": "No lyrics found",
                            "next": "",
                            "animation": "fade",
                            "progress": track_info['progress_ms'] / track_info['duration_ms'],
                            "title": anyascii(track_info['name']),
                            "artist": anyascii(track_info['artist']),
                            "bpm": track_info.get('bpm', 120.0),
                            "energy": track_info.get('energy', 0.5),
                            "dur": 5000
                        })
                    
                    # Sync logic
                    progress_seconds = track_info['progress_ms'] / 1000.0
                    current_lyric, next_lyric = lyrics_manager.get_current_and_next_line(parsed_lyrics, progress_seconds)
                    
                    if not current_lyric:
                        current_lyric = {"time": -2, "text": ""}
                    
                    # If we moved to a new lyric line, broadcast it
                    if current_lyric['time'] != last_lyric_time:
                        last_lyric_time = current_lyric['time']
                        
                        animation = animation_selector.select_animation(current_lyric, next_lyric, track_info)
                        
                        duration_ms = 5000
                        if next_lyric:
                            duration_ms = int((next_lyric['time'] - current_lyric['time']) * 1000)
                            
                        payload = {
                            "time": current_lyric['time'],
                            "current": current_lyric['text'],
                            "next": next_lyric['text'] if next_lyric else "",
                            "animation": animation,
                            "progress": track_info['progress_ms'] / track_info['duration_ms'],
                            "title": anyascii(track_info['name']),
                            "artist": anyascii(track_info['artist']),
                            "bpm": track_info.get('bpm', 120.0),
                            "energy": track_info.get('energy', 0.5),
                            "dur": duration_ms
                        }
                        
                        print(f"Broadcasting: {payload['current']} [{animation}]")
                        await ws_manager.broadcast(payload)
                else:
                    print("Spotify not playing or no track info.")
                        
        except Exception as e:
            print(f"Error in polling loop: {e}")
            
        # Wait before next poll (approx 2Hz is usually good enough for lyrics sync)
        await asyncio.sleep(0.5)

@app.websocket("/ws")
async def websocket_endpoint(websocket: WebSocket):
    await ws_manager.connect(websocket)
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
