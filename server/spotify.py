import json
import spotipy
from spotipy.oauth2 import SpotifyOAuth
from spotipy.cache_handler import CacheFileHandler, MemoryCacheHandler
from config import SPOTIFY_CLIENT_ID, SPOTIFY_CLIENT_SECRET, SPOTIFY_REDIRECT_URI, SPOTIFY_CACHE_INFO

class SpotifyManager:
    def __init__(self):
        self.sp = None
        if SPOTIFY_CLIENT_ID and SPOTIFY_CLIENT_SECRET:
            cache_handler = None
            if SPOTIFY_CACHE_INFO:
                try:
                    token_info = json.loads(SPOTIFY_CACHE_INFO)
                    cache_handler = MemoryCacheHandler(token_info=token_info)
                    print("Using MemoryCacheHandler from SPOTIFY_CACHE_INFO env variable")
                except json.JSONDecodeError:
                    print("ERROR: SPOTIFY_CACHE_INFO is not valid JSON")
                    cache_handler = CacheFileHandler(cache_path=".cache")
            else:
                cache_handler = CacheFileHandler(cache_path=".cache")
                
            auth_manager = SpotifyOAuth(
                client_id=SPOTIFY_CLIENT_ID,
                client_secret=SPOTIFY_CLIENT_SECRET,
                redirect_uri=SPOTIFY_REDIRECT_URI,
                scope="user-read-playback-state",
                cache_handler=cache_handler
            )
            self.sp = spotipy.Spotify(auth_manager=auth_manager)
        self.audio_features_cache = {}
        
    def get_current_playback(self):
        if not self.sp:
            return None
        try:
            return self.sp.current_playback()
        except Exception as e:
            print(f"Error fetching playback: {e}")
            return None

    def get_track_info(self):
        playback = self.get_current_playback()
        if not playback or not playback.get('item'):
            return None
        
        item = playback['item']
        track_id = item['id']
        
        # Spotify deprecated the audio_features endpoint in Nov 2024.
        # It will return 403 Forbidden for all new apps.
        # We will just use the default fallback values to keep animations running smoothly.
        if track_id not in self.audio_features_cache:
            self.audio_features_cache[track_id] = {"bpm": 120.0, "energy": 0.5, "valence": 0.5}
                
        audio_features = self.audio_features_cache[track_id]
        
        return {
            "id": track_id,
            "name": item['name'],
            "artist": item['artists'][0]['name'],
            "album": item['album']['name'],
            "progress_ms": playback.get('progress_ms', 0),
            "is_playing": playback.get('is_playing', False),
            "duration_ms": item['duration_ms'],
            "bpm": audio_features["bpm"],
            "energy": audio_features["energy"],
            "valence": audio_features["valence"]
        }
