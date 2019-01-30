#!/usr/bin/env python3

import argparse
import datetime
import email.utils
import http.server as server
import os
import ssl
import sys
import time
import urllib.parse
from http import HTTPStatus


# global variable initialization
slow_server = False
hang_server = False
partial_download_file = None
threshold = 0
time_delay = 0
length = 16*1024
response = HTTPStatus.OK


class SimpleServer(server.SimpleHTTPRequestHandler):

    counter = 0
    first_time = True

    def do_GET(self):
        """Serve a GET request."""

        status_code = response

        # if the partial download option was set and the server is
        # currently serving the file that was requested as partial
        if self.path == "/{}".format(partial_download_file):
            if SimpleServer.first_time:
                SimpleServer.first_time = False
                status_code = HTTPStatus.PARTIAL_CONTENT

        self.hang_server()

        f = self.send_head(status_code)
        if f:
            try:
                # start serving the requested URL
                while True:
                    buf = f.read(length)
                    if not buf:
                        break
                    self.wfile.write(buf)

                    # if the partial download was set break
                    # after readng the fist chunk of data
                    if status_code == HTTPStatus.PARTIAL_CONTENT:
                        break

                    # only start delaying responses after the threshold has
                    # been passed
                    if SimpleServer.counter > threshold:
                        time.sleep(time_delay)
            finally:
                f.close()

    def hang_server(self):
        """Hangs the server if set and the threshold has been reached"""

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

    def do_HEAD(self):
        """Serve a HEAD request."""
        self.hang_server()
        f = self.send_head()
        if f:
            f.close()

    def send_head(self, status=HTTPStatus.OK):
        """Common code for GET and HEAD commands.

        This sends the response code and MIME headers.

        Return value is either a file object (which has to be copied
        to the outputfile by the caller unless the command was HEAD,
        and must be closed by the caller under all circumstances), or
        None, in which case the caller has nothing further to do.

        """
        path = self.translate_path(self.path)
        f = None
        if os.path.isdir(path):
            parts = urllib.parse.urlsplit(self.path)
            if not parts.path.endswith('/'):
                # redirect browser - doing basically what apache does
                self.send_response(HTTPStatus.MOVED_PERMANENTLY)
                new_parts = (parts[0], parts[1], parts[2] + '/',
                             parts[3], parts[4])
                new_url = urllib.parse.urlunsplit(new_parts)
                self.send_header("Location", new_url)
                self.end_headers()
                return None
            for index in "index.html", "index.htm":
                index = os.path.join(path, index)
                if os.path.exists(index):
                    path = index
                    break
            else:
                return self.list_directory(path)
        ctype = self.guess_type(path)
        try:
            f = open(path, 'rb')
        except OSError:
            self.send_error(HTTPStatus.NOT_FOUND, "File not found")
            return None

        try:
            fs = os.fstat(f.fileno())
            # Use browser cache if possible
            if ("If-Modified-Since" in self.headers
                    and "If-None-Match" not in self.headers):
                # compare If-Modified-Since and time of last file modification
                try:
                    ims = email.utils.parsedate_to_datetime(
                        self.headers["If-Modified-Since"])
                except (TypeError, IndexError, OverflowError, ValueError):
                    # ignore ill-formed values
                    pass
                else:
                    if ims.tzinfo is None:
                        # obsolete format with no timezone, cf.
                        # https://tools.ietf.org/html/rfc7231#section-7.1.1.1
                        ims = ims.replace(tzinfo=datetime.timezone.utc)
                    if ims.tzinfo is datetime.timezone.utc:
                        # compare to UTC datetime of last modification
                        last_modif = datetime.datetime.fromtimestamp(
                            fs.st_mtime, datetime.timezone.utc)
                        # remove microseconds, like in If-Modified-Since
                        last_modif = last_modif.replace(microsecond=0)

                        if last_modif <= ims:
                            self.send_response(HTTPStatus.NOT_MODIFIED)
                            self.end_headers()
                            f.close()
                            return None

            self.send_response(status)
            self.send_header("Content-type", ctype)
            self.send_header("Content-Length", str(fs[6]))
            self.send_header("Last-Modified",
                             self.date_time_string(fs.st_mtime))
            self.end_headers()
            return f
        except Exception:
            f.close()
            raise


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

    parser.add_argument("--force-response", type=int, default=HTTPStatus.OK,
                        help="forces the web server to respond with a specific"
                        "code to any request")

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
    response = args.force_response

    httpd = server.HTTPServer(addr, SimpleServer)

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
        # change the current working dir to that one we want to serve
        os.chdir(args.directory)

    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        print("Server stopped by user")
    finally:
        httpd.server_close()
