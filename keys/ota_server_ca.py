#要先把server.crt和server.key放在当前目录下
#把firmware.bin複製過來

import http.server
import ssl
import functools
from http.server import HTTPServer, SimpleHTTPRequestHandler

ip = '192.168.3.153'
port = 443
server_address = (ip, port)
local_path = 0

if local_path:
    FIRMWARE_DIR = "C:/Users/fanyu/Documents/Arduino/httpUpdate/build/esp32.esp32.node32s"
    handler = functools.partial(SimpleHTTPRequestHandler, directory=FIRMWARE_DIR)
    httpd = HTTPServer(server_address, handler)
else:
    httpd = http.server.HTTPServer(server_address, http.server.SimpleHTTPRequestHandler)

context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
context.load_cert_chain(certfile="server.crt", keyfile="server.key")

httpd.socket = context.wrap_socket(httpd.socket, server_side=True)

print("HTTPS Server running ip=" + ip + ",port=" + port.__str__())
httpd.serve_forever()