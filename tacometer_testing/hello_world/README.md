# Stamp-S3 ignition tachometer test

This ESP-IDF project reads the conditioned induction-coil output on **Stamp-S3 G1
(GPIO1, ADC1 channel 0)**. Connect the circuit output to G1 and its reference
ground to the Stamp-S3 ground. The program uses the ESP32-S3 ADC continuous
driver at **10,000 samples/s**, matching the 100 µs spacing in `../scope_7.csv`.

The detector keeps a slowly moving average of normal ADC samples as its
baseline. It measures absolute deviation, so either positive or negative
spikes can trigger. The trigger level is the larger of 12 ADC counts and five
times the measured normal deviation. It waits for the signal to return near
the baseline, then ignores additional triggers for 15 ms so ignition ringing
is counted once. RPM uses the mean of the most recent four spark intervals and
goes to zero after 500 ms without a spark.

The scope file spans 200 ms and has prominent disturbances about 33–35 ms
apart. With this engine's **two sparks per revolution**, that suggests roughly
**850–900 RPM**. This is an estimate from the capture, not a calibration of the ADC
input. The recorded voltage is only millivolts, and actual ADC counts depend
on the gain and bias of your circuit. The firmware starts with a 12-count
minimum threshold; tune it using the live `ADC`, `baseline`, and `threshold`
log values.

## Build and run

From this directory in an ESP-IDF 5.5 shell:

```text
idf.py set-target esp32s3
idf.py build
idf.py -p COM_PORT flash monitor
```

The project already has an `esp32s3` sdkconfig, so `set-target` is only needed
if that changes. Replace `COM_PORT` with the board's serial port.

The serial log prints each accepted spark and a summary every 0.5 s:

```text
I (...) tachometer: spark #3      ADC=... baseline=... threshold=...
I (...) tachometer: RPM=... sparks=... baseline=... threshold=... clipped=...
```

`clipped` counts samples at ADC code 0 or 4095 during that report interval.
Frequent clipping means one polarity may be invisible or the peak may be
truncated. An ADC buffer overflow resets the detector because the sample
interval is no longer trustworthy.

## Tuning

- `ADC_GPIO`, `SAMPLE_RATE_HZ`, and `SPARKS_PER_REV` are at the top of
  `main/hello_world_main.c`. `SPARKS_PER_REV` is set to 2 for this engine.
- `MIN_THRESHOLD_COUNTS` and `NOISE_MULTIPLIER` are in
  `main/spark_detector.c`. Increase the minimum if ordinary noise causes false
  sparks; lower it if real transients stay below `threshold`. The reported
  `ADC` value at a spark should differ clearly from `baseline`.
- `MIN_SPARK_SAMPLES` is the 15 ms ringing holdoff at 10 kHz. Reduce it if
  true spark intervals are shorter than 15 ms. `STOP_SAMPLES` controls the
  no-spark timeout.
- ADC attenuation is set to `ADC_ATTEN_DB_12` in
  `main/hello_world_main.c`. Change it if your conditioned voltage range
  calls for a different ADC range.

Pin availability is documented in the [M5Stack Stamp-S3 pin map](https://docs.m5stack.com/en/core/Stamp-S3).
The sampling API is the [Espressif ADC continuous driver](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/peripherals/adc/adc_continuous.html).
