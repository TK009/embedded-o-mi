#!/usr/bin/python3

import http.server, cgi, sys, os, subprocess

core = subprocess.Popen(["./bin/core"], stdin=subprocess.PIPE, stdout=subprocess.PIPE, text=True)

class CallbackHandler(http.server.BaseHTTPRequestHandler):
    def do_POST(self):
        # Read all content
        
        ctype = self.headers.get_content_type()
        if ctype == "application/x-www-form-urlencoded":
            form = cgi.FieldStorage(
                fp=self.rfile, 
                headers=self.headers,
                environ={'REQUEST_METHOD':'POST'})
            msgxml = form.getfirst("msg", "")
            #msgxml = msg.decode("UTF-8")
        #elif ctype == "text/xml" or ctype == "application/xml": 
        else:
            content_len = int(self.headers['content-length'])
            post_body = self.rfile.read(content_len)
            msgxml = post_body.decode("UTF-8")
        #else:
        #    self.send_response(400)
        #    self.end_headers()
        #    print(f"Invalid content type {ctype}", file=self.wfile)



        self.send_response(200)
        self.end_headers()

        print("\nReceived Request:\n" + msgxml, file=sys.stderr)
        #print(msgxml)
        #sys.stdout.flush()
        #resp = input()
        core.stdin.write(msgxml)
        core.stdin.flush()
        resp = core.stdout.readline()
        print("\nSending Response:\n" + resp, file=sys.stderr)
        self.wfile.write(bytes(resp,"utf-8"))

    def do_OPTIONS(self):
        self.send_response(200)
        self.end_headers()

    def end_headers (self): # CORS
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Access-Control-Allow-Headers', '*')
        super().end_headers()

server_address = ('0.0.0.0', int(os.environ.get("PORT", 8081)))
httpd = http.server.HTTPServer(server_address, CallbackHandler)
print("Starting server")
httpd.serve_forever()
