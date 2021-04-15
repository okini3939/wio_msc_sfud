# wio_msc_sfud

- Wio Terminal
  - Internal QSPI Flash (W25Q32JVZPIM)
  - USB Type-C - MSC Device

- for Arduino IDE
  - Board: Seeeduino Wio Terminal
  - USB Stack: TinyUSB
  - Library: https://github.com/Seeed-Studio/Seeed_Arduino_FS
  - Library: https://github.com/Seeed-Studio/Seeed_Arduino_SFUD
  - Library: https://github.com/adafruit/Adafruit_TinyUSB_Arduino

## Memo

- Button A: List directory
- Button B: Read file
- Button C: Write file

FAT: Sector Size 4096 bytes

1st sector address: 0x01f8 (Flash)
