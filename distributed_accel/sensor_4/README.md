# ADXL345 SPI test for ESP32-S3

This is a small ESP-IDF project for checking one ADXL345 module. It configures
the sensor for an 800 Hz output-data rate and prints every 50th sample, which
is approximately 16 lines per second.

## Pinout

| ESP32-S3 | ADXL345 |
| --- | --- |
| GPIO 6 | SCL / SCLK |
| GPIO 5 | SDA / MOSI |
| GPIO 4 | SDO / MISO |
| GPIO 2 | INT1 (configured as an input, not required for polling) |
| GPIO 1 | CS |

The SPI mode is 3. The program checks that the sensor returns the ADXL345
device ID (`0xE5`) before starting the sample loop.


Build and flash with ESP-IDF v5.5.3:

```text
idf.py set-target esp32s3
idf.py build
idf.py -p PORT flash monitor
```
