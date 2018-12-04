#!/usr/bin/env python3

import argparse
import http.server as server
import os
import ssl
import sys
import time


partial_download_file = ""
slow_server = False


class TestServer(server.BaseHTTPRequestHandler):
    first_time = True

    """Handler that returns data with a set delay between writes and/or
       simulates a partial download on the first download attempt for the
       file specified by partial_download_file."""
    def do_GET(self):
        try:
            f = open(os.getcwd() + self.path, 'rb')
        except Exception:
            self.send_response(404)
            self.end_headers()
            return

        # Simulate partial download
        partial_download = False
        split_paths = os.path.split(self.path)

        if split_paths[1] == partial_download_file and TestServer.first_time:
            partial_download = True
            TestServer.first_time = False
            self.send_response(206)
        else:
            self.send_response(200)
        self.send_header("Content-type", "text/html")
        self.end_headers()

        while True:
            b = f.read(1)
            if b == b'':
                break

            self.wfile.write(b)
            if partial_download:
                break

            # Insert delay for slow server
            if slow_server:
                delay = 0.00001  # seconds
                time.sleep(delay)
        self.wfile.flush()
        f.close()


if __name__ == '__main__':

    parser = argparse.ArgumentParser()

    parser.add_argument('--client_cert', help='client public key')

    parser.add_argument('--partial_download_file', default="",
                        help='file name to download partially. On first '
                        'download fail with partial download error and succeed'
                        ' on 2nd download.')

    parser.add_argument('--pid_file',
                        help='File path to write pid used by web server')

    parser.add_argument('--port_file',
                        help='File path to write port used by web server')

    parser.add_argument('--server_cert', help='server public key')

    parser.add_argument('--server_key', help='server private key')

    parser.add_argument('--slow_server', action='store_true',
                        default=False, help='Use slow server')

    args = parser.parse_args()

    # host web server on localhost with available port
    addr = ('localhost', 0)

    partial_download_file = args.partial_download_file
    slow_server = args.slow_server

    # The TestServer is used when simulating a slow server and/or a
    # partial file download. Otherwise use the simple HTTP server.
    if slow_server or partial_download_file:
        request_handler = TestServer
    else:
        request_handler = server.SimpleHTTPRequestHandler

    httpd = server.HTTPServer(addr, request_handler)

    # write pid to file to be read by other processes
    if args.pid_file:
        pid = os.getpid()
        with open(args.pid_file, "w") as f:
            f.write(str(pid))

    # write port to file which can be read by other processes
    if args.port_file:
        port = httpd.socket.getsockname()[1]
        with open(args.port_file, "w") as f:
            f.write(str(port))

    # configure ssl certificates
    if args.server_cert and args.server_key:
        wrap_socket_args = {'certfile': args.server_cert,
                            'keyfile': args.server_key,
                            'server_side': True}

        # add client certificate
        if args.client_cert:
            wrap_socket_args.update({'ca_certs': args.client_cert,
                                     'cert_reqs': ssl.CERT_REQUIRED})

        httpd.socket = ssl.wrap_socket(httpd.socket, **wrap_socket_args)

    # invalid certificate combination
    elif args.server_cert or args.server_key or args.client_cert:
        sys.exit("server.py: Invalid certificate combination")

    httpd.serve_forever()
