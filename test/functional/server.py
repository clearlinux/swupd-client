#!/usr/bin/env python3

import argparse
import http.server as server
import ssl
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


if __name__ == '__main__':

    parser = argparse.ArgumentParser()

    parser.add_argument('--client_cert', help='client public key')

    parser.add_argument('--port_file',
                        help='File path to write port used by web server')

    parser.add_argument('--server_cert', help='server public key')

    parser.add_argument('--server_key', help='server private key')

    parser.add_argument('--slow_server', action='store_true',
                        default=False, help='Use slow server')

    args = parser.parse_args()

    # host web server on localhost with available port
    addr = ('localhost', 0)

    # server with delay or normal
    if args.slow_server:
        request_handler = SlowResponse
    else:
        request_handler = server.SimpleHTTPRequestHandler

    httpd = server.HTTPServer(addr, request_handler)

    # write port to file which can be read by other processes
    if args.port_file:
        port = httpd.socket.getsockname()[1]
        with open (args.port_file, "w") as f:
            f.write(str(port))

    # configure ssl certificates
    if args.server_cert and args.server_key:
        wrap_socket_args = { 'certfile': args.server_cert, 'keyfile': args.server_key, 'server_side': True }

        # add client certificate
        if args.client_cert:
            wrap_socket_args.update({ 'ca_certs': args.client_cert, 'cert_reqs': ssl.CERT_REQUIRED })

        httpd.socket = ssl.wrap_socket(httpd.socket, **wrap_socket_args)

    # invalid certificate combination
    elif args.server_cert or args.server_key or args.client_cert:
        sys.exit("server.py: Invalid certificate combination")

    httpd.serve_forever()
