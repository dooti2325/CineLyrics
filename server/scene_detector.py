class SceneDetector:
    def __init__(self):
        pass

    def detect_scene(self, progress_ms: int, duration_ms: int, energy: float) -> str:
        """
        Uses heuristics based on track progress and energy to estimate the current scene.
        """
        if duration_ms == 0:
            return "Verse"
            
        progress_pct = progress_ms / duration_ms
        
        if progress_pct < 0.1:
            return "Intro"
        elif progress_pct > 0.9:
            return "Outro"
            
        # High energy sections in the middle are usually choruses
        if energy > 0.75:
            return "Chorus"
            
        # Middle-late sections that are lower energy are often bridges
        if 0.65 < progress_pct < 0.8:
            return "Bridge"
            
        # Otherwise default to Verse
        return "Verse"
