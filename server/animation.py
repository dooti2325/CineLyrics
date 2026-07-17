import random

class AnimationSelector:
    def __init__(self):
        # 10 cinematic animations
        self.animations = [
            "fade", "slide_left", "slide_up", "zoom", "typewriter",
            "karaoke", "shake", "pulse", "wave", "glitch"
        ]

    def select_animation(self, current_lyric, next_lyric, track_info):
        """
        Determines the best animation for the current lyric using AI Mood Detection
        based on Spotify's Audio Features (Energy and Valence).
        """
        text = current_lyric["text"] if current_lyric else ""
        if not text:
            return "fade"
            
        energy = track_info.get("energy", 0.5)
        valence = track_info.get("valence", 0.5)
        
        # High Energy + Low Valence (Rock/Rap/Aggressive)
        if energy > 0.7 and valence < 0.4:
            return random.choice(["shake", "glitch", "typewriter"])
            
        # Low Energy (Acoustic/Sad/Slow)
        if energy < 0.4:
            return random.choice(["fade", "slide_up", "wave"])
            
        # High Energy + High Valence (Pop/Dance/Happy)
        if energy > 0.7 and valence >= 0.4:
            return random.choice(["karaoke", "slide_left", "zoom"])
            
        # Default mix for medium energy
        return random.choice(["fade", "karaoke", "typewriter", "slide_up"])
