#要先把server.crt和server.key放在当前目录下
#把firmware.bin複製過來
import os
import http.server
import ssl
import functools
from http.server import HTTPServer, SimpleHTTPRequestHandler

def read_cn_from_openssl_cnf(file_path):
    cn_value = None
    inside_dn_section = False
    with open(file_path, 'r') as f:
        for line in f:
            line = line.strip()
            if line.startswith('['):
                inside_dn_section = (line == '[ dn ]')
                continue
            if inside_dn_section and '=' in line:
                key, value = line.split('=', 1)
                if key.strip() == 'CN':
                    cn_value = value.strip()
                    break
    return cn_value

# ip = '192.168.3.152'
ip = read_cn_from_openssl_cnf('openssl-san.cnf')
port = 443
server_address = (ip, port)
local_path = 0

if local_path:
    FIRMWARE_DIR = "C:/Users/fanyu/Documents/Arduino/httpUpdate/build/esp32.esp32.node32s"
    # handler = functools.partial(SimpleHTTPRequestHandler, directory=FIRMWARE_DIR)
    # httpd = HTTPServer(server_address, handler)
else:
    httpd = http.server.HTTPServer(server_address, http.server.SimpleHTTPRequestHandler)

context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
context.load_cert_chain(certfile="server.crt", keyfile="server.key")

httpd.socket = context.wrap_socket(httpd.socket, server_side=True)

print("HTTPS Server running ip=" + ip + ",port=" + port.__str__())
httpd.serve_forever()


