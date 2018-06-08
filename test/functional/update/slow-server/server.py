#!/usr/bin/env python3

import http.server as server
import sys
import time


class SlowResponse(server.BaseHTTPRequestHandler):
    """Handler that returns data with a set delay between writes"""
    def do_GET(self):
        try:
            f = open('web-dir/' + self.path, 'rb')
        except:
            self.send_response(404)
            self.end_headers()
            return

        self.send_response(200)
        self.send_header("Content-type", "text/html")
        self.end_headers()
        delay = 0.00001 # seconds

        while True:
            b = f.read(1000)
            if b == b'':
                break
            self.wfile.write(b)
            time.sleep(delay)
        self.wfile.flush()
        f.close()

if __name__ == '__main__':
    print(sys.argv)
    addr = ('', int(sys.argv[1]))
    httpd = server.HTTPServer(addr, SlowResponse)
    httpd.serve_forever()
