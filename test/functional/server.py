#!/usr/bin/env python3

import argparse
import http.server as server
import os
import ssl
import sys
import time


# global variable initialization
slow_server = False
hang_server = False
partial_download_file = ""
threshold = 0
time_delay = 0
length = 16*1024


class SimpleServer(server.SimpleHTTPRequestHandler):

    counter = 0

    def do_GET(self):
        """Serve a GET request."""
        self.hang_server()
        f = self.send_head()
        if f:
            try:
                while 1:
                    buf = f.read(length)
                    if not buf:
                        break
                    self.wfile.write(buf)
                    # only start delaying responses after the threshold has
                    # been passed
                    if SimpleServer.counter > threshold:
                        time.sleep(time_delay)
            finally:
                f.close()

    def do_HEAD(self):
        """Serve a HEAD request."""
        self.hang_server()
        f = self.send_head()
        if f:
            f.close()

    def hang_server(self):

        # to avoid hanging the server while testing for the connection to be
        # ready when the --hang-server option is set, the server provides the
        # special url "/test-connection"
        if self.path == "/test-connection" or self.path == "/test-connection/":
            self.path = "/"
        else:
            SimpleServer.counter += 1
            # only hang the server if we have reached the threshold
            if hang_server and (SimpleServer.counter > threshold):
                print("Hanging the server. Please use Ctrl-C to stop the "
                      "server.")
                while True:
                    pass


class TestServer(server.BaseHTTPRequestHandler):
    first_time = True

    """Handler that returns data with a set delay between writes and/or
       simulates a partial download on the first download attempt for the
       file specified by partial_download_file."""
    def do_GET(self):
        try:
            f = open(os.getcwd() + self.path, "rb")
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
            b = f.read(length)
            if b == b"":
                break

            self.wfile.write(b)
            if partial_download:
                break

            # Insert delay for slow server
            time.sleep(time_delay)
        self.wfile.flush()
        f.close()


def parse_arguments():

    parser = argparse.ArgumentParser()

    parser.add_argument("--client-cert", help="client public key")

    parser.add_argument("--server-cert", help="server public key")

    parser.add_argument("--server-key", help="server private key")

    parser.add_argument("--port", type=int, default=0,
                        help="If specified, the server will attempt to use "
                        "the selected port, it will use a random port "
                        "otherwise.")

    parser.add_argument("--pid-file",
                        help="File path to write pid used by web server")

    parser.add_argument("--port-file",
                        help="File path to write port used by web server")

    parser.add_argument("--reachable", action="store_true",
                        default=False, help="If set, the server will be "
                        "reachable from other machines in the network")

    parser.add_argument("--directory", default=None, help="The relative path "
                        "to the directory to serve. If not specified, the "
                        "directory to serve is the current working directory")

    parser.add_argument("--slow-server", action="store_true",
                        default=False, help="Use slow server")

    parser.add_argument("--time-delay", type=float,
                        help="Used to fine tune the slow server function. It "
                        "sets the delay value (in miliseconds) used to sleep "
                        "between reads. Default value is 1 milisecond.")

    parser.add_argument("--chunk-length", type=int,
                        help="Used to fine tune the slow server function. It "
                        "sets the length of the chunk of data read in each "
                        "cycle by the server. Default value is 10 bytes.")

    parser.add_argument("--hang-server", action="store_true",
                        default=False, help="Hangs the server")

    parser.add_argument("--after-requests", type=int, default=0,
                        help="The server will respond normally for the first "
                        " N number of requests, and will start responding "
                        "slowly / hang after that.")

    parser.add_argument("--partial-download-file",
                        help="file name to download partially. On first "
                        "download fail with partial download error and succeed"
                        " on 2nd download.")

    return parser.parse_args()


if __name__ == "__main__":

    args = parse_arguments()

    # configure the web server based on user options
    if args.reachable:
        addr = ("0.0.0.0", args.port)
    else:
        addr = ("localhost", args.port)
    partial_download_file = args.partial_download_file
    if args.slow_server:
        time_delay = (args.time_delay if args.time_delay else 1)/1000
        length = args.chunk_length if args.chunk_length else 10
    print("Time delay (seconds): ", time_delay)
    print("Chunk length: ", length)
    threshold = args.after_requests
    hang_server = args.hang_server

    # The TestServer is used when simulating partial file download
    # Otherwise use the SimpleServer
    if partial_download_file:
        request_handler = TestServer
    else:
        request_handler = SimpleServer
    httpd = server.HTTPServer(addr, request_handler)

    # write pid to file to be read by other processes
    pid = os.getpid()
    if args.pid_file:
        with open(args.pid_file, "w") as f:
            f.write(str(pid))
    print("Web server PID: ", pid)

    # write port to file which can be read by other processes
    port = httpd.socket.getsockname()[1]
    if args.port_file:
        with open(args.port_file, "w") as f:
            f.write(str(port))
    print("Web server port: ", port)

    # configure ssl certificates
    if args.server_cert and args.server_key:
        wrap_socket_args = {"certfile": args.server_cert,
                            "keyfile": args.server_key,
                            "server_side": True}

        # add client certificate
        if args.client_cert:
            wrap_socket_args.update({"ca_certs": args.client_cert,
                                     "cert_reqs": ssl.CERT_REQUIRED})

        httpd.socket = ssl.wrap_socket(httpd.socket, **wrap_socket_args)

    # invalid certificate combination
    elif args.server_cert or args.server_key or args.client_cert:
        sys.exit("server.py: Invalid certificate combination")

    # for the time-delay or the chunk-length to be used the slow server has
    # to be used
    if not args.slow_server and (args.time_delay or args.chunk_length):
        print("server.py (WARNING): --slow-server is not set, the "
              "--time-delay and --chunk-length flags will be are ignored.")

    if args.directory:
        # since this is going to change the working directory for python
        # it needs to be run in the end so it does not affect other variables
        # that were based on the original path
        directory = os.path.join(
            os.path.dirname(os.path.realpath(__file__)),
            args.directory)
        # change the current working dir to that one we want to serve
        os.chdir(directory)

    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        print("Server stopped by user")
    finally:
        httpd.server_close()
