"""
Mock HTTP Server for Integration Testing.

Handles common REST methods (GET, POST, PUT, PATCH, DELETE) and
returns JSON showing the method, URL, and request body (if any).

- GET/DELETE: returns {"method": "...", "url": "..."}
- POST/PUT/PATCH: returns {"method": "...", "body": "..."}

Usage: python server.py (listens on 0.0.0.0:8080)

Intended for local testing of REST clients; always responds with JSON.
"""

from http.server import BaseHTTPRequestHandler, HTTPServer
import json

class RequestHandler(BaseHTTPRequestHandler):
    def _send_json(self, data, status=200):
        self.send_response(status)
        self.send_header("Content-Type", "application/json")
        self.end_headers()
        self.wfile.write(json.dumps(data).encode())

    def do_GET(self):
        self._send_json({"method": "GET", "url": self.path})

    def do_POST(self):
        length = int(self.headers.get('Content-Length', 0))
        body = self.rfile.read(length).decode()
        self._send_json({"method": "POST", "body": body})

    def do_PUT(self):
        length = int(self.headers.get('Content-Length', 0))
        body = self.rfile.read(length).decode()
        self._send_json({"method": "PUT", "body": body})

    def do_PATCH(self):
        length = int(self.headers.get('Content-Length', 0))
        body = self.rfile.read(length).decode()
        self._send_json({"method": "PATCH", "body": body})

    def do_DELETE(self):
        self._send_json({"method": "DELETE", "url": self.path})

if __name__ == "__main__":
    server = HTTPServer(("0.0.0.0", 8080), RequestHandler)
    print("Server running on port 8080")
    server.serve_forever()
