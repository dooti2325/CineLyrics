from vaderSentiment.vaderSentiment import SentimentIntensityAnalyzer

class EmotionAI:
    def __init__(self):
        self.analyzer = SentimentIntensityAnalyzer()
        self.emotion_map = {
            "Happy": ["smile", "laugh", "joy", "happy", "glad", "sunshine"],
            "Sad": ["cry", "tears", "sad", "sorrow", "pain", "hurt", "die", "alone"],
            "Powerful": ["power", "strong", "survive", "win", "fight", "king", "alive", "unbreakable"],
            "Love": ["love", "heart", "kiss", "romance", "forever", "together"],
            "Angry": ["hate", "anger", "fury", "burn", "destroy", "rage"],
            "Fear": ["scared", "fear", "dark", "afraid", "run", "hide"],
            "Hope": ["hope", "light", "dream", "believe", "rise"]
        }

    def analyze(self, text: str) -> str:
        if not text or len(text.strip()) == 0:
            return "Calm"
            
        text_lower = text.lower()
        
        # 1. Keyword matching for specific music emotions
        for emotion, keywords in self.emotion_map.items():
            if any(kw in text_lower for kw in keywords):
                return emotion
                
        # 2. VADER sentiment analysis for general feeling
        scores = self.analyzer.polarity_scores(text)
        compound = scores['compound']
        
        if compound >= 0.5:
            return "Happy"
        elif compound <= -0.5:
            return "Sad"
        elif compound >= 0.1:
            return "Excited"
        elif compound <= -0.1:
            return "Angry"
        else:
            return "Calm"
