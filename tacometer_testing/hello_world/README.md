# Stamp-S3 raw ADC capture

The `hello_world` project currently builds a raw ADC recorder for the ignition
pickup. It reads **Stamp-S3 G1 (GPIO1, ADC1 channel 0)** at **10,000 samples/s**
with **0 dB attenuation**. Each capture contains 2,000 consecutive samples,
covering 200 ms at the same nominal spacing as `../scope_7.csv`.

The firmware stores a capture in RAM, stops the ADC, then prints the raw
12-bit codes as CSV. Serial output therefore cannot slow the sampling or
create gaps within a capture. After printing, it waits 500 ms and starts the
next capture. No baseline tracking or spark detection is applied.

## Build and run

From this directory in an ESP-IDF 5.5 shell:

```text
idf.py build
idf.py -p COM_PORT flash monitor
```

Replace `COM_PORT` with the board's serial port. The project already targets
`esp32s3`. Output has a header followed by rows such as:

```csv
capture,time_us,raw
0,0,4
0,100,5
0,200,0
...
1,0,3
```

`capture` identifies each 200 ms burst; `time_us` is time from the start of
that burst; `raw` is the ADC code from 0 to 4095. Boot messages or ADC error
warnings may appear outside the CSV rows. A capture is discarded if the ADC
buffer overflows or a read fails. Sharing one capture at idle and one at high
throttle will show whether the ignition pulses are distinct from noise.

If most values are still 0, the input is at the ADC's lower rail. If many
values are 4095, the peaks exceed the 0 dB measurement range. The
[ESP32-S3 datasheet](https://documentation.espressif.com/esp32-s3_datasheet_en.pdf)
lists an effective 0 dB range of about 0–850 mV and a calibrated total error
of roughly ±5 mV. That total error concerns absolute voltage accuracy; the
capture will show how repeatable the code changes are on this board.

The earlier tachometer implementation remains in `main/hello_world_main.c`
and `main/spark_detector.c`. To build it again, change `main/CMakeLists.txt`
back to those two source files.
