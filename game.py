import pygame, json, ssl
import asyncio
import websockets

# 1) Start your Pygame window…
pygame.init()
screen = pygame.display.set_mode((800,600))
clock = pygame.time.Clock()

# 2) Create an SSL context to allow self-signed certificates
ssl_context = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
ssl_context.check_hostname = False
ssl_context.verify_mode = ssl.CERT_NONE  # Accept self-signed cert

uri = "wss://localhost:8675"

async def websocket_task():
    try:
        async with websockets.connect(uri, ssl=ssl_context, ping_interval=None) as websocket:
            print("Connected to WSS server")
            await websocket.send(json.dumps({"type": "login", "key": "SOME_KEY", "role": "observer"}))  # Example action, replace with your own

            running = True
            while running:
                # 4) Handle Pygame events
                for evt in pygame.event.get():
                    if evt.type == pygame.QUIT:
                        running = False

                # 5) Receive one JSON‐encoded game-state packet
                try:
                    raw = await websocket.recv()  # Await the WebSocket response
                    state = json.loads(raw)
                    print(f"Received state:\n{state}")  # Debugging output
                except Exception as e:
                    print(f"Error receiving data: {e}")
                    running = False

                # 6) Draw your map / players / inventory based on `state`
                screen.fill((0,0,0))
                # … your blitting code here …

                pygame.display.flip()
                clock.tick(60)  # cap at 60 FPS
    except Exception as e:
        print(f"WebSocket Error: {e}")

# Run the asyncio event loop
asyncio.run(websocket_task())
