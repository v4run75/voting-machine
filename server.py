#!/usr/bin/env python3

import os
import posixpath
from urllib.parse import unquote
from http.server import HTTPServer, SimpleHTTPRequestHandler

# modify this to add additional routes
ROUTES = (
    # [url_prefix ,  directory_path]
    ['/cgi','/home/seed/Desktop/etovucca/cgi-bin/'],
    ['','/home/seed/Desktop/etovucca/']  # empty string for the 'default' match
) 

class RequestHandler(SimpleHTTPRequestHandler):

    def translate_path(self, path):
        """translate path given routes"""

        # set default root to cwd
        root = os.getcwd()

        # look up routes and set root directory accordingly
        for pattern, rootdir in ROUTES:
            if path.startswith(pattern):
                # found match!
                path = path[len(pattern):]  # consume path up to pattern len
                root = rootdir
                break

        # normalize path and prepend root directory
        path = path.split('?',1)[0]
        path = path.split('#',1)[0]
        path = posixpath.normpath(unquote(path))
        words = path.split('/')
        words = filter(None, words)

        path = root
        for word in words:
            drive, word = os.path.splitdrive(word)
            head, word = os.path.split(word)
            if word in (os.curdir, os.pardir):
                continue
            path = os.path.join(path, word)

        return path

if __name__ == '__main__':
    myServer = HTTPServer(('0.0.0.0', 8000), RequestHandler)
    print("Ready to begin serving files.")
    try:
        myServer.serve_forever()
    except KeyboardInterrupt:
        pass

    myServer.server_close()
    print("Exiting.")
