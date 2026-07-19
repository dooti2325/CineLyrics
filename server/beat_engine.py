import os
import tempfile
import urllib.request
import librosa
import numpy as np

class BeatEngine:
    def __init__(self):
        self.cache = {}

    def analyze_track(self, track_id: str, preview_url: str):
        if not preview_url:
            return {"bpm": 120.0, "beat_strength": 0.5, "bass": 0.5}

        if track_id in self.cache:
            return self.cache[track_id]

        try:
            # Download preview
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

            # Cleanup
            try:
                os.remove(temp_path)
            except:
                pass

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

    def get_current_beat_info(self, track_id: str, preview_url: str, current_energy: float):
        base_info = self.analyze_track(track_id, preview_url)
        # Mix the static spotify energy with the librosa analyzed base info
        final_strength = (base_info["beat_strength"] + current_energy) / 2.0
        final_bass = base_info["bass"]
        
        return {
            "beat": True, # Exact real-time beat sync over websocket is tricky due to latency, ESP32 handles the precise pulse via BPM
            "beatStrength": round(final_strength, 2),
            "bass": round(final_bass, 2),
            "bpm": base_info["bpm"]
        }
