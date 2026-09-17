import webbrowser
import urllib.parse
from spotify import SpotifyManager

if __name__ == "__main__":
    print("=== CineLyric Spotify Auth Setup ===")
    manager = SpotifyManager()
    url = manager.get_auth_url()
    if not url:
        print("ERROR: SPOTIFY_CLIENT_ID or SPOTIFY_CLIENT_SECRET not set in .env")
        exit(1)

    print("\n1. Opening Spotify Authorization page in your default browser...")
    print(f"URL: {url}\n")
    try:
        webbrowser.open(url)
    except Exception:
        pass

    print("2. In your browser, log in and click 'Agree'.")
    print("3. You will be redirected to your configured Redirect URI (e.g. https://cinelyrics.onrender.com/?code=...)")
    redirect_url = input("\nPaste the FULL URL you were redirected to here:\n> ").strip()

    if "code=" in redirect_url:
        parsed = urllib.parse.urlparse(redirect_url)
        params = urllib.parse.parse_qs(parsed.query)
        code = params.get("code", [None])[0]
        if code:
            success, msg, token_json = manager.exchange_code(code)
            if success:
                print("\n[SUCCESS] Spotify authentication completed! Token cached in .cache")
                print("\nFor Render Cloud Deployment, set SPOTIFY_CACHE_INFO in your Render Environment Variables to:")
                print(token_json)
            else:
                print(f"\n[ERROR] Failed to exchange code: {msg}")
        else:
            print("ERROR: No code parameter found in the URL.")
    else:
        print("ERROR: Invalid redirect URL entered (missing code= parameter).")
