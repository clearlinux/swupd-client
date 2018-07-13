#!/usr/bin/env python3

import http.server as server
import sys
import time


class SlowResponse(server.BaseHTTPRequestHandler):
    """Handler that returns data with a set delay between writes"""
    def do_GET(self):
        self.send_response(200)
        self.send_header("Content-type", "text/html")
        self.end_headers()
        response = "99990"
        delay = 0.00001 # seconds
        for c in response:
            self.wfile.write(str.encode(c))
            time.sleep(delay)


if __name__ == '__main__':
    print(sys.argv)
    addr = ('', int(sys.argv[1]))
    httpd = server.HTTPServer(addr, SlowResponse)
    httpd.serve_forever()
