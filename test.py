import asyncio
import ssl
import websockets
import random
import string

async def test_wss():
    ssl_context = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
    ssl_context.check_hostname = False
    ssl_context.verify_mode = ssl.CERT_NONE  # Accept self-signed cert

    uri = "wss://localhost:8674"

    async with websockets.connect(uri, ssl=ssl_context) as websocket:
        print("Connected to WSS server")

        while True:
            # Wait for user input
            message = input("Enter message to send (or 'exit' to quit): ")
            if message.lower() == 'exit':
                print("Exiting...")
                break

            if message.lower() == 'rand':
                # Generate a random string with more than 10,000 characters
                message = ''.join(random.choices(string.ascii_letters + string.digits, k=10001))
                print("Generated random string with more than 10,000 characters")

            # Send the message
            await websocket.send(message)
            print("Sent:", message)

            # Receive response
            response = await websocket.recv()
            print("Received:", response)
            response = await websocket.recv()
            print("Received:", response)

asyncio.run(test_wss())
