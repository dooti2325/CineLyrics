import asyncio
import json
from fastapi import WebSocket, WebSocketDisconnect, status

class WebSocketConnectionManager:
    def __init__(self, max_connections: int = 10):
        self.active_connections: list[WebSocket] = []
        self.max_connections = max_connections

    async def connect(self, websocket: WebSocket) -> bool:
        if len(self.active_connections) >= self.max_connections:
            print(f"[WS] Connection rejected: limit of {self.max_connections} reached.")
            await websocket.close(code=status.WS_1008_POLICY_VIOLATION)
            return False

        await websocket.accept()
        self.active_connections.append(websocket)
        print(f"[WS] Client connected. Total clients: {len(self.active_connections)}")
        return True

    def disconnect(self, websocket: WebSocket):
        if websocket in self.active_connections:
            self.active_connections.remove(websocket)
            print(f"[WS] Client disconnected. Total clients: {len(self.active_connections)}")

    async def broadcast(self, message: dict):
        if not self.active_connections:
            return

        json_message = json.dumps(message)
        # Create a copy to avoid modification during iteration
        for connection in list(self.active_connections):
            try:
                await connection.send_text(json_message)
            except (WebSocketDisconnect, RuntimeError):
                self.disconnect(connection)
            except Exception as e:
                print(f"[WS] Error sending message to client: {e}")
                self.disconnect(connection)
