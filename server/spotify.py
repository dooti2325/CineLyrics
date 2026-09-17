import os
import json
import spotipy
from spotipy.oauth2 import SpotifyOAuth
from spotipy.cache_handler import CacheFileHandler, MemoryCacheHandler
from config import SPOTIFY_CLIENT_ID, SPOTIFY_CLIENT_SECRET, SPOTIFY_REDIRECT_URI, SPOTIFY_CACHE_INFO

class SpotifyManager:
    def __init__(self):
        self.sp = None
        self.auth_manager = None
        self.cache_handler = None
        self.audio_features_cache = {}
        self._init_spotify()

    def _init_spotify(self):
        if SPOTIFY_CLIENT_ID and SPOTIFY_CLIENT_SECRET:
            if SPOTIFY_CACHE_INFO:
                try:
                    token_info = json.loads(SPOTIFY_CACHE_INFO)
                    self.cache_handler = MemoryCacheHandler(token_info=token_info)
                    print("[Spotify] Using MemoryCacheHandler from SPOTIFY_CACHE_INFO")
                except json.JSONDecodeError:
                    print("[Spotify] ERROR: SPOTIFY_CACHE_INFO is not valid JSON, falling back to .cache")
                    self.cache_handler = CacheFileHandler(cache_path=".cache")
            else:
                self.cache_handler = CacheFileHandler(cache_path=".cache")

            self.auth_manager = SpotifyOAuth(
                client_id=SPOTIFY_CLIENT_ID,
                client_secret=SPOTIFY_CLIENT_SECRET,
                redirect_uri=SPOTIFY_REDIRECT_URI,
                scope="user-read-playback-state",
                cache_handler=self.cache_handler,
                open_browser=False
            )
            
            # Check if we have a valid cached token
            cached_token = self.cache_handler.get_cached_token()
            if cached_token:
                try:
                    # Validate/refresh token
                    if self.auth_manager.validate_token(cached_token):
                        self.sp = spotipy.Spotify(auth_manager=self.auth_manager)
                        print("[Spotify] Authenticated successfully with cached token.")
                    else:
                        print("[Spotify] Cached token is expired or invalid. Please re-authenticate at /login")
                        self.sp = None
                except Exception as e:
                    print(f"[Spotify] Could not validate cached token ({e}). Re-auth required at /login")
                    self.sp = None
            else:
                print("[Spotify] No cached token found. Re-auth required at /login")
                self.sp = None

    def get_auth_url(self) -> str:
        """Returns the Spotify OAuth authorization URL."""
        if not self.auth_manager:
            self._init_spotify()
        if not self.auth_manager:
            return ""
        return self.auth_manager.get_authorize_url()

    def exchange_code(self, code: str):
        """Exchanges authorization code for an access token and persists it."""
        if not self.auth_manager:
            self._init_spotify()
        if not self.auth_manager:
            return False, "Spotify credentials not set", ""

        try:
            token_info = self.auth_manager.get_access_token(code, as_dict=True)
            self.sp = spotipy.Spotify(auth_manager=self.auth_manager)
            token_json = json.dumps(token_info)
            print("[Spotify] Successfully exchanged authorization code for token!")
            return True, "Authenticated successfully", token_json
        except Exception as e:
            err_msg = str(e)
            print(f"[Spotify] Token exchange failed: {err_msg}")
            return False, err_msg, ""

    def is_authenticated(self) -> bool:
        if not self.sp or not self.auth_manager or not self.cache_handler:
            return False
        token = self.cache_handler.get_cached_token()
        return bool(token and self.auth_manager.validate_token(token))

    def get_current_playback(self):
        if not self.sp:
            return None
        try:
            return self.sp.current_playback()
        except Exception as e:
            err_msg = str(e)
            if "invalid_client" in err_msg or "invalid_grant" in err_msg:
                print(f"[Spotify] Auth token rejected by Spotify ({err_msg}). Clearing stale cache.")
                if os.path.exists(".cache"):
                    try:
                        os.remove(".cache")
                    except OSError:
                        pass
                self.sp = None
            else:
                print(f"[Spotify] Error fetching playback: {e}")
            return None

    def get_track_info(self):
        playback = self.get_current_playback()
        if not playback or not playback.get('item'):
            return None

        item = playback['item']
        track_id = item.get('id', '')

        if track_id not in self.audio_features_cache:
            self.audio_features_cache[track_id] = {"bpm": 120.0, "energy": 0.5, "valence": 0.5}

        audio_features = self.audio_features_cache[track_id]

        artist_name = item['artists'][0]['name'] if item.get('artists') else "Unknown Artist"
        album_name = item['album']['name'] if item.get('album') else ""

        return {
            "id": track_id,
            "name": item.get('name', 'Unknown Track'),
            "artist": artist_name,
            "album": album_name,
            "progress_ms": playback.get('progress_ms', 0),
            "is_playing": playback.get('is_playing', False),
            "duration_ms": item.get('duration_ms', 0),
            "preview_url": item.get('preview_url'),
            "bpm": audio_features["bpm"],
            "energy": audio_features["energy"],
            "valence": audio_features["valence"]
        }
