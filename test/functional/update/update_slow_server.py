#!/usr/bin/env python3

import http.server as server
import sys
import time

class SlowResponse(server.BaseHTTPRequestHandler):
    first_time = True

    """Handler that returns data with a set delay between writes"""
    def do_GET(self):
        try:
            f = open('update-slow-server/web-dir/' + self.path, 'rb')
        except:
            self.send_response(404)
            self.end_headers()
            return

        #Simulate partial download
        partial_download = False
        if self.path == "//100/pack-test-bundle-from-10.tar" and SlowResponse.first_time:
            partial_download = True
            SlowResponse.first_time = False
            self.send_response(206)
        else:
            self.send_response(200)
        self.send_header("Content-type", "text/html")
        self.end_headers()
        delay = 0.00001 # seconds

        while True:
            b = f.read(100)
            if b == b'':
                break

            self.wfile.write(b)
            if partial_download:
                break;
            time.sleep(delay)
        self.wfile.flush()
        f.close()

if __name__ == '__main__':
    print(sys.argv)
    addr = ('', int(sys.argv[1]))
    httpd = server.HTTPServer(addr, SlowResponse)
    httpd.serve_forever()
