///////////////////////////////////Arduino  IDE////////////////////////////////////////////////////////////////////
1.
版本Arduino  IDE 2.3.6

2.
開啟Arduino IDE後，選擇功能表的檔案/偏好設定，開啟偏好設定視窗，在addition boards manager URL輸入以下文字後，按OK。
https://dl.espressif.com/dl/package_esp32_index.json
Arduino Library manager: Go to sketch -> Include Library -> Manage Libraries, search for NimBLE (2.2.3) and install.
或者
Alternatively: Download as .zip and extract to Arduino/libraries folder, or in Arduino IDE from Sketch menu -> Include library -> Add .Zip library.

3.
在 Arduino IDE 的「工具 → Partition Scheme」改為 "Minimal SPIFFS"

////////////////////////////////////////PlatformIO///////////////////////////////////////////////////////////////


[env:node32s]
platform = espressif32
board = node32s
framework = arduino
lib_deps = h2zero/NimBLE-Arduino@2.2.3
board_build.partitions = min_spiffs.csv

; 如果要用HTTP,他是原生的,不需要加库,不然會build fail
; lib_extra_dirs = ~/Documents/Arduino/libraries

/////////////////////////////////////////bin檔燒錄//////////////////////////////////////////////////////////////
bin檔燒錄
安裝 esptool
python -m pip install esptool

燒錄bin,改COM PORT就好
[esp32dev]
python -m esptool --chip esp32 --port COM22 --baud 921600 write_flash -z 0x1000 bootloader.bin 0x8000 partitions.bin 0x10000 firmware.bin
python -m esptool --chip esp32 --port COM3 --baud 115200 write-flash -z 0x1000 bootloader.bin 0x8000 partitions.bin 0xe000 C:\Users\fanyu\.platformio\packages\framework-arduinoespressif32\tools\partitions\boot_app0.bin 0x10000 firmware.bin

[esp32s3]
python -m esptool --chip esp32s3 --port COM11 --baud 460800 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 80m --flash_size 8MB 0x0000 bootloader.bin 0x8000 partitions.bin 0xe000 C:\Users\fanyu\.platformio\packages\framework-arduinoespressif32\tools\partitions\boot_app0.bin 0x10000 firmware.bin


PlatformIO 會依據你 platformio.ini 的設定（board = esp32dev）自動帶入 上傳配置。
pio run -t upload -v

  /////////////////////////////////////////min_spiffs.csv路徑//////////////////////////////////////////////////////////////
C:\Users\<你的使用者>\.platformio\packages\framework-arduinoespressif32\tools\partitions\
