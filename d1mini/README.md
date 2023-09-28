# WEMOS D1 MINI - ESP8266 WIFI CHIP

This folder contains code developed on the ESP8266-based D1 Mini to retrieve sensor data and publish to an MQTT topic.
Three chips are built and connected at the same time, providing three different client connectionss to the MQTT broker,
and publishing data simultaneously.

## Structure

- `fft_LEDstripe`: code running on the D1 Mini that includes the LED stripe. It's a combination of the `fft_wifi` and `fft_simple` codes,
  both publishing the raw signal and plotting a local calculation of the _FFT_
- `fft_simple`: manual (i.e, inside the actual chip) calculation of the _Fast Fourier Tranform_ of the signal and plot using an LED strip (model _MAX7219_)
- `fft_wifi`: obtain the raw sensor signal from the microphone (model _MAX9814_ with automatic gain at 40, 50 or 60 dB) and publish to MQTT topic
  (from a public broker, using unsecured connection for the time being). For the three different chips we have now, some values MUST be changed before the upload:
  - The MQTT _client name_ must be different for each one; for the time being, using `ESPClient1`, `ESPClient2` and `ESPClient3` suffices
  - Right now, **TouchDesigner** expects the following topics:
    - `ESPClient/micro1`
    - `ESPClient/micro2`
    - `ESPClient/micro3`
