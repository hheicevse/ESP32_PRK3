#要CA憑證,要有myCA.pem,要有openssl-san.cnf
# 這個檔案是用來產生 OTA 簽名的憑證
#把myCA.pem放在data資料夾裡
#透過platformio介面按下Upload Filesystem Image的時候,會自動去data資料夾裡找myCA.pem,並且燒入進去
#或者打指令上傳也可以 pio run --target uploadfs

# 驗證# openssl x509 -in server.crt -noout -text
# 驗證# openssl x509 -req -in server.csr -CA myCA.pem -CAkey myCA.key -CAcreateserial -out server.crt -days 500 -sha256
import os
# 建立 Root CA
os.system("openssl genrsa -out myCA.key 2048")
# os.system("openssl req -x509 -new -nodes -key myCA.key -sha256 -days 3650 -out myCA.pem -config openssl-san.cnf")
ca_subject = "/C=TW/ST=Taipei/L=Taipei/O=MyIOT Inc/OU=Dev/CN=MyRootCA"
os.system(f"openssl req -x509 -new -nodes -key myCA.key -sha256 -days 3650 -out myCA.pem -subj \"{ca_subject}\"")

# 建立 Server Key & CSR
os.system("openssl genrsa -out server.key 2048")
os.system("openssl req -new -key server.key -out server.csr -config openssl-san.cnf")

# 用 CA 簽發 Server 憑證，包含 subjectAltName
os.system("openssl x509 -req -in server.csr -CA myCA.pem -CAkey myCA.key -CAcreateserial -out server.crt -days 3650 -sha256")

# 複製 CA 憑證到 ESP32 的 SPIFFS (data 資料夾)
os.system("copy .\\myCA.pem ..\\data\\")
# 以下是OK的,但是要打CN的IP
# # 建立 Root CA
# openssl genrsa -out myCA.key 2048
# openssl req -x509 -new -nodes -key myCA.key -sha256 -days 1024 -out myCA.pem

# # 建立 server 憑證
# openssl genrsa -out server.key 2048
# openssl req -new -key server.key -out server.csr

# # 用 CA 簽署 server 憑證
# openssl x509 -req -in server.csr -CA myCA.pem -CAkey myCA.key -CAcreateserial -out server.crt -days 500 -sha256
