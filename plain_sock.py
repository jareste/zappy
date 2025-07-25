import asyncio
import websockets
import random
import string
import json

async def client_task(client_id):
    uri = "ws://localhost:8674"

    try:
        async with websockets.connect(uri) as websocket:
            # print(f"Client {client_id}: Connected to WS server")

            response = await websocket.recv()
            # print(f"Client {client_id}: Received: {response}")

            while True:
                message = ''.join(random.choices(string.ascii_letters + string.digits, k=20))
                
                json_message = json.dumps({"client_id": client_id, "message": message})
                
                await websocket.send(json_message)
                # print(f"Client {client_id}: Sent (as JSON): {json_message}")
                
                response = await websocket.recv()
                try:
                    parsed_response = json.loads(response)
                    # print(f"Client {client_id}: Received (parsed as JSON): {parsed_response}")
                except json.JSONDecodeError:
                    print(f"Client {client_id}: Received (not a valid JSON): {response}")
                
                await asyncio.sleep(0.05)
    except Exception as e:
        print(f"Client {client_id}: Error - {e}")

async def main():
    tasks = [client_task(client_id) for client_id in range(1, 51)]
    await asyncio.gather(*tasks)

asyncio.run(main())
