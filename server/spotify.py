import spotipy
from spotipy.oauth2 import SpotifyOAuth
from config import SPOTIFY_CLIENT_ID, SPOTIFY_CLIENT_SECRET, SPOTIFY_REDIRECT_URI

class SpotifyManager:
    def __init__(self):
        self.sp = None
        if SPOTIFY_CLIENT_ID and SPOTIFY_CLIENT_SECRET:
            auth_manager = SpotifyOAuth(
                client_id=SPOTIFY_CLIENT_ID,
                client_secret=SPOTIFY_CLIENT_SECRET,
                redirect_uri=SPOTIFY_REDIRECT_URI,
                scope="user-read-playback-state"
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
        
        # Fetch audio features if not in cache
        if track_id not in self.audio_features_cache:
            try:
                features = self.sp.audio_features([track_id])[0]
                if features:
                    self.audio_features_cache[track_id] = {
                        "bpm": features.get("tempo", 120.0),
                        "energy": features.get("energy", 0.5),
                        "valence": features.get("valence", 0.5)
                    }
                else:
                    self.audio_features_cache[track_id] = {"bpm": 120.0, "energy": 0.5, "valence": 0.5}
            except Exception as e:
                print(f"Error fetching audio features: {e}")
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
