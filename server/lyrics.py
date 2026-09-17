import re
import aiohttp
import asyncio
from anyascii import anyascii

class LyricsManager:
    def __init__(self):
        self.base_url = "https://lrclib.net/api"
        self.headers = {
            "User-Agent": "CineLyric/1.0 (https://github.com/dooti2325/CineLyrics)"
        }

    async def fetch_lyrics(self, track_name, artist_name, album_name=None, duration_ms=None):
        """Asynchronously fetches synced lyrics from LRCLIB without blocking the event loop."""
        params = {
            "track_name": track_name,
            "artist_name": artist_name
        }
        if album_name:
            params["album_name"] = album_name
        if duration_ms:
            params["duration"] = duration_ms // 1000

        timeout = aiohttp.ClientTimeout(total=4)
        try:
            async with aiohttp.ClientSession(timeout=timeout, headers=self.headers) as session:
                async with session.get(f"{self.base_url}/get", params=params) as response:
                    if response.status == 200:
                        data = await response.json()
                        synced_lyrics = data.get("syncedLyrics")
                        if synced_lyrics:
                            return self.parse_lrc(synced_lyrics)

                # Fallback 1: Try with just track and artist if album/duration caused a mismatch
                if album_name or duration_ms:
                    fallback_params = {
                        "track_name": track_name,
                        "artist_name": artist_name
                    }
                    async with session.get(f"{self.base_url}/get", params=fallback_params) as response:
                        if response.status == 200:
                            data = await response.json()
                            synced_lyrics = data.get("syncedLyrics")
                            if synced_lyrics:
                                return self.parse_lrc(synced_lyrics)

                # Fallback 2: Search endpoint
                search_params = {"q": f"{track_name} {artist_name}"}
                async with session.get(f"{self.base_url}/search", params=search_params) as response:
                    if response.status == 200:
                        results = await response.json()
                        if results and isinstance(results, list):
                            for item in results:
                                if item.get("syncedLyrics"):
                                    return self.parse_lrc(item["syncedLyrics"])

        except Exception as e:
            print(f"[LyricsManager] Error or timeout fetching lyrics from LRCLIB: {e}")

        return []

    def parse_lrc(self, lrc_text):
        """Parses LRC formatted string into a list of dictionaries with time and text."""
        lyrics = []
        if not lrc_text:
            return lyrics

        lines = lrc_text.split('\n')
        # Regex to match [mm:ss.xx]
        time_pattern = re.compile(r'\[(\d{2}):(\d{2}\.\d{2})\](.*)')

        for line in lines:
            match = time_pattern.match(line)
            if match:
                minutes = int(match.group(1))
                seconds = float(match.group(2))
                text = match.group(3).strip()
                time_in_seconds = (minutes * 60) + seconds

                # Only add lines that actually have text
                if text:
                    ascii_text = anyascii(text)
                    lyrics.append({
                        "time": time_in_seconds,
                        "text": ascii_text
                    })
        return lyrics

    def get_current_and_next_line(self, parsed_lyrics, progress_seconds):
        """Returns the current lyric and the next lyric based on playback progress."""
        if not parsed_lyrics:
            return None, None

        current_line = None
        next_line = None

        for i in range(len(parsed_lyrics)):
            if parsed_lyrics[i]["time"] <= progress_seconds:
                current_line = parsed_lyrics[i]
                if i + 1 < len(parsed_lyrics):
                    next_line = parsed_lyrics[i + 1]
                else:
                    next_line = None
            else:
                break

        return current_line, next_line
