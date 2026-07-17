from spotify import SpotifyManager

if __name__ == "__main__":
    print("Starting Spotify Auth Flow...")
    manager = SpotifyManager()
    # This will trigger the browser opening and wait for the redirect
    playback = manager.get_current_playback()
    print("Authentication successful! You can now start the uvicorn server.")
