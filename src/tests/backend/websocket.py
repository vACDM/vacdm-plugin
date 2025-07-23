import asyncio
import websockets

async def echo(websocket, path):
    async for message in websocket:
        print(f"Received: {message}")
        await websocket.send(f"Echo: {message}")

async def main():
    async with websockets.serve(echo, "localhost", 8080):
        print("WebSocket server started at ws://localhost:8080")
        await asyncio.Future()  # run forever

asyncio.run(main())
