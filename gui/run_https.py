#!/usr/bin/env python3
import http.server
import ssl
import socketserver
import os
import subprocess
import sys

def main():
    PORT = 8674
    DIRECTORY = "."
    
    cert_file = "../server/certs/localhost+2.pem"
    key_file = "../server/certs/localhost+2-key.pem"

    os.chdir(DIRECTORY)
    
    Handler = http.server.SimpleHTTPRequestHandler
    
    with socketserver.TCPServer(("", PORT), Handler) as httpd:
        context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        context.load_cert_chain(cert_file, key_file)
        
        httpd.socket = context.wrap_socket(httpd.socket, server_side=True)
        
        print(f"HTTPS Server running on https://localhost:{PORT}")
        print(f"Serving files from: {os.getcwd()}")
        print(f"Access your HTML at: https://localhost:{PORT}/https_accept.html")
        print("Press Ctrl+C to stop the server")
        
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\nServer stopped.")

if __name__ == "__main__":
    main()