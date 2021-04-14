/*
 * Board: Seeeduino Wio Terminal
 * USB Stack: TinyUSB
 * 
 * Library: https://github.com/Seeed-Studio/Seeed_Arduino_FS
 * Library: https://github.com/Seeed-Studio/Seeed_Arduino_SFUD
 * Library: https://github.com/adafruit/Adafruit_TinyUSB_Arduino
 */

#include "Wio_io.h"

#include "SPI.h"
#include "Seeed_FS.h"
#include "SFUD/Seeed_SFUD.h"

#include "sfud.h"
#include "Adafruit_TinyUSB.h"

#define RAMDISK  0
#if RAMDISK
#define DISK_BLOCK_NUM  16
#define DISK_BLOCK_SIZE 512
#include "ramdisk.h"
#else
#define DISK_BLOCK_NUM 1024
#define DISK_BLOCK_SIZE SECTORSIZE
#endif
Adafruit_USBD_MSC usb_msc;

void setup() {
  Serial.begin(115200);
  ioInitPin();

  if (! SPIFLASH.begin(50000000UL)) {
    Serial.println("W25Q32JV not detected");
    for(;;);
  }

  usb_msc.setID("Seeed", "MSC", "0.1");
#if RAMDISK
  usb_msc.setCapacity(DISK_BLOCK_NUM, DISK_BLOCK_SIZE);
#else
  int block = SPIFLASH.flashSize() / DISK_BLOCK_SIZE;
  usb_msc.setCapacity(block, DISK_BLOCK_SIZE);
#endif
  usb_msc.setReadWriteCallback(msc_read_cb, msc_write_cb, msc_flush_cb);
  usb_msc.setUnitReady(true);
  usb_msc.begin();

  while( !USBDevice.mounted() ) delay(1);

}

void loop() {

  if (digitalRead(WIO_5S_PRESS) == LOW) {
    const sfud_flash *flash = sfud_get_device_table() + 0;
    uint8_t buf[16];
    for (int n = 0; n < 0x800; n ++) {
      if (sfud_read(flash, n * 512, sizeof(buf), buf) == SFUD_SUCCESS) {
        if (buf[3] != 'M' || buf[4] != 'S') continue; // search FAT header
        Serial.print(n, HEX);
        Serial.print(" : ");
        for (int i = 0; i < sizeof(buf); i ++) {
          if (buf[i] >= 0x20 && buf[i] < 0x7f) {
            Serial.print((char)buf[i]);
          } else {
            Serial.print(buf[i], HEX);
          }
          Serial.print(" ");
        }
      }
      Serial.println();
    }
    while (digitalRead(WIO_5S_PRESS) == LOW);
  }

  if (digitalRead(WIO_KEY_A) == LOW) {
    listDir(SPIFLASH, "/", 0);
    while (digitalRead(WIO_KEY_A) == LOW);
  }

  if (digitalRead(WIO_KEY_B) == LOW) {
    Serial.println("-----");
    File fs = SPIFLASH.open("test.txt", "r");
    if (fs) {
      while (fs.available()) {
        Serial.write(fs.read());
      }
      fs.close();
    }
    Serial.println("-----");
    while (digitalRead(WIO_KEY_B) == LOW);
  }

  if (digitalRead(WIO_KEY_C) == LOW) {
    Serial.println("write");
    File fs = SPIFLASH.open("test.txt", "w");
    if (fs) {
      for (int i = 0; i < 4096; i ++) {
//        fs.write((i >> 4) & 0xff);
        fs.print(i, HEX);
        if ((i & 0x0f) == 0x0f) {
          fs.println();
        } else {
          fs.print(" ");
        }
      }
      fs.close();
    }
    while (digitalRead(WIO_KEY_C) == LOW);
  }

  delay(100);
}

int32_t msc_read_cb (uint32_t lba, void* buffer, uint32_t bufsize) {
#if RAMDISK
  uint8_t const* addr = msc_disk[lba];
  memcpy(buffer, addr, bufsize);
#else
  static int read_lba = -1, read_size = 0;
  const sfud_flash *flash = sfud_get_device_table();
  int addr = DISK_BLOCK_SIZE * lba + (512 * 0x1f8);
  if (read_lba != lba) {
    read_lba = lba;
    read_size = bufsize;
  } else {
    addr += read_size;
    read_size += bufsize;
    if (read_size >= DISK_BLOCK_SIZE) read_size = 0;
  }
  if (sfud_read(flash, addr, bufsize, (uint8_t*)buffer) != SFUD_SUCCESS) {
    return -1;
  }
#endif
  return bufsize;
}

int32_t msc_write_cb (uint32_t lba, uint8_t* buffer, uint32_t bufsize) {
#if RAMDISK
  uint8_t* addr = msc_disk[lba];
  memcpy(addr, buffer, bufsize);
#else
  static int write_lba = -1, write_size = 0;
  const sfud_flash *flash = sfud_get_device_table();
  int addr = DISK_BLOCK_SIZE * lba + (512 * 0x1f8);
  if (write_lba != lba) {
    write_lba = lba;
    write_size = bufsize;
  } else {
    addr += write_size;
    write_size += bufsize;
    if (write_size >= DISK_BLOCK_SIZE) write_size = 0;
  }
  if (sfud_erase_write(flash, addr, bufsize, buffer) != SFUD_SUCCESS) {
    return -1;
  }
#endif
  return bufsize;
}

void msc_flush_cb (void) {
  // nothing to do
}


void listDir(fs::FS& fs, const char* dirname, uint8_t levels) {
    Serial.print("Listing directory: ");
    Serial.println(dirname);

    File root = fs.open(dirname);
    if (!root) {
        Serial.println("Failed to open directory");
        return;
    }
    if (!root.isDirectory()) {
        Serial.println("Not a directory");
        return;
    }

    File file = root.openNextFile();
    while (file) {
        if (file.isDirectory()) {
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if (levels) {
                listDir(fs, file.name(), levels - 1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("  SIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
    root.close();

    Serial.print("Total: ");
    Serial.println((int)SPIFLASH.totalBytes(), DEC);
    Serial.print("Used : ");
    Serial.println((int)SPIFLASH.usedBytes(), DEC);
}
