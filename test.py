import asyncio
import ssl
import websockets
import random
import string
import json

async def test_wss():
    ssl_context = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
    ssl_context.check_hostname = False
    ssl_context.verify_mode = ssl.CERT_NONE  # Accept self-signed cert

    uri = "wss://localhost:8674"

    async with websockets.connect(uri, ssl=ssl_context) as websocket:
        print("Connected to WSS server")

        response = await websocket.recv()
        print("Received:", response)
        while True:        
            message = input("Enter message to send (or 'exit' to quit): ")
            if message.lower() == 'exit':
                print("Exiting...")
                break
            
            if message.lower() == 'rand':
                message = ''.join(random.choices(string.ascii_letters + string.digits, k=10001))
                print("Generated random string with more than 10,000 characters")
            
            # Wrap the message in a JSON object
            json_message = json.dumps({"message": message})
            
            await websocket.send(json_message)
            print("Sent (as JSON):", json_message)
            response = await websocket.recv()
            try:
                parsed_response = json.loads(response)
                print("Received (parsed as JSON):", parsed_response)
            except json.JSONDecodeError:
                print("Received (not a valid JSON):", response)


asyncio.run(test_wss())
