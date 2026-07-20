from emotion_ai import EmotionAI
from scene_detector import SceneDetector
from beat_engine import BeatEngine

class AIDirector:
    def __init__(self):
        self.emotion_ai = EmotionAI()
        self.scene_detector = SceneDetector()
        self.beat_engine = BeatEngine()
        
        # Decision Matrix based on the architectural plan
        self.animation_matrix = {
            "Happy": {"High": "Bounce", "Medium": "Slide Up", "Low": "Pulse"},
            "Sad": {"High": "Fade", "Medium": "Fade", "Low": "Fade"},
            "Powerful": {"High": "Zoom", "Medium": "Shake", "Low": "Slide Left"},
            "Love": {"High": "Pulse", "Medium": "Drift", "Low": "Drift"},
            "Fear": {"High": "Glitch", "Medium": "Shake", "Low": "Fade"},
            "Angry": {"High": "Glitch", "Medium": "Shake", "Low": "Shake"},
            "Calm": {"High": "Wave", "Medium": "Wave", "Low": "Fade"}
        }

    def determine_animation(self, emotion: str, beat_strength: float) -> str:
        beat_level = "Medium"
        if beat_strength > 0.7:
            beat_level = "High"
        elif beat_strength < 0.4:
            beat_level = "Low"
            
        anim = "Fade"
        if emotion in self.animation_matrix:
            if beat_level in self.animation_matrix[emotion]:
                anim = self.animation_matrix[emotion][beat_level]
            else:
                anim = list(self.animation_matrix[emotion].values())[1]
                
        return anim.lower()

    def determine_font(self, text: str) -> str:
        length = len(text)
        if length < 5:
            return "Huge"
        elif length < 10:
            return "Large"
        elif length < 20:
            return "Medium"
        else:
            return "Small"

    def compose_packet(self, current_lyric: dict, next_lyric: dict, track_info: dict) -> dict:
        progress_ms = track_info.get("progress_ms", 0)
        duration_ms = track_info.get("duration_ms", 0)
        track_id = track_info.get("id", "")
        preview_url = track_info.get("preview_url", "")
        static_energy = track_info.get("energy", 0.5)

        beat_data = self.beat_engine.get_current_beat_info(track_id, preview_url, static_energy)
        
        emotion = self.emotion_ai.analyze(current_lyric.get("text", ""))
        
        scene = self.scene_detector.detect_scene(progress_ms, duration_ms, beat_data["beatStrength"])
        
        animation = self.determine_animation(emotion, beat_data["beatStrength"])
        
        font = self.determine_font(current_lyric.get("text", ""))
        
        shake = (scene == "Chorus" and beat_data["beatStrength"] > 0.8)
        invert = False
        
        particles = "None"
        if scene == "Chorus" and emotion in ["Happy", "Powerful"]:
            particles = "Burst"
        elif scene == "Outro":
            particles = "Drift"
            
        secondary = "Pulse" if beat_data["beatStrength"] > 0.6 else "None"

        dur = 5000
        if next_lyric and next_lyric.get("time", -1) != -1:
            dur = int((next_lyric["time"] - current_lyric["time"]) * 1000)
            
        return {
            "time": current_lyric.get("time", -1),
            "lyric": current_lyric.get("text", ""),
            "next": next_lyric.get("text", "") if next_lyric else "",
            "beat": True,
            "beatStrength": beat_data["beatStrength"],
            "bass": beat_data["bass"],
            "emotion": emotion,
            "scene": scene,
            "animation": animation.replace(" ", "_"),
            "secondary": secondary,
            "font": font,
            "x": 64,
            "y": 32,
            "shake": shake,
            "invert": invert,
            "particles": particles,
            "duration": dur,
            "title": track_info.get("name", ""),
            "artist": track_info.get("artist", ""),
            "bpm": beat_data["bpm"],
            "energy": static_energy,
            "is_playing": track_info.get("is_playing", False)
        }
