import os
import tempfile
import urllib.request
from urllib.parse import urlparse
import librosa
import numpy as np

def is_safe_preview_url(url: str) -> bool:
    """Validates that the preview URL strictly points to Spotify's verified audio CDN over HTTPS."""
    if not url or not isinstance(url, str):
        return False
    try:
        parsed = urlparse(url)
        if parsed.scheme != "https":
            return False
        host = parsed.netloc.lower()
        # Spotify preview CDN hosts are typically p.scdn.co or audio-ak-spotify-com.akamaized.net
        return host == "p.scdn.co" or host.endswith(".scdn.co")
    except Exception:
        return False

class BeatEngine:
    def __init__(self):
        self.cache = {}

    def analyze_track(self, track_id: str, preview_url: str):
        if not preview_url:
            return {"bpm": 120.0, "beat_strength": 0.5, "bass": 0.5}

        if track_id in self.cache:
            return self.cache[track_id]

        if not is_safe_preview_url(preview_url):
            print(f"[BeatEngine] Ignored unsafe or invalid preview URL: {preview_url}")
            return {"bpm": 120.0, "beat_strength": 0.5, "bass": 0.5}

        temp_path = None
        try:
            # Download preview to a guaranteed managed temp file
            with tempfile.NamedTemporaryFile(delete=False, suffix='.mp3') as tmp_file:
                temp_path = tmp_file.name
            
            urllib.request.urlretrieve(preview_url, temp_path)
            
            # Load with librosa
            y, sr = librosa.load(temp_path, sr=None)
            
            # Extract BPM
            tempo, _ = librosa.beat.beat_track(y=y, sr=sr)
            bpm = float(tempo[0]) if isinstance(tempo, (np.ndarray, list)) else float(tempo)
            
            # Estimate Energy (RMS)
            rms = librosa.feature.rms(y=y)[0]
            beat_strength = float(np.mean(rms))
            
            # Estimate Bass
            y_harmonic, y_percussive = librosa.effects.hpss(y)
            bass = float(np.mean(librosa.feature.rms(y=y_percussive)))

            # Normalize to 0-1 (approximate tuning for average tracks)
            beat_strength = min(1.0, beat_strength * 10)
            bass = min(1.0, bass * 15)

            result = {
                "bpm": bpm,
                "beat_strength": beat_strength,
                "bass": bass
            }
            self.cache[track_id] = result
            return result

        except Exception as e:
            print(f"Error in BeatEngine: {e}")
            return {"bpm": 120.0, "beat_strength": 0.5, "bass": 0.5}

        finally:
            # Always clean up temporary audio file to prevent disk exhaustion
            if temp_path and os.path.exists(temp_path):
                try:
                    os.remove(temp_path)
                except OSError:
                    pass

    def get_current_beat_info(self, track_id: str, preview_url: str, current_energy: float):
        base_info = self.analyze_track(track_id, preview_url)
        # Mix the static spotify energy with the librosa analyzed base info
        final_strength = (base_info["beat_strength"] + current_energy) / 2.0
        final_bass = base_info["bass"]
        
        return {
            "beat": True,
            "beatStrength": round(final_strength, 2),
            "bass": round(final_bass, 2),
            "bpm": base_info["bpm"]
        }
