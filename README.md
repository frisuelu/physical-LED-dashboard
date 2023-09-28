# IoT project for LED physical dashboard

Code for sensor data streaming to a remote server/computer using Raspberry Pi Pico and ESP8266 WIFI modules.

## Structure

- The `d1mini` folder contains the actual code that runs on the D1 Mini chip and retrieves sensor data to stream to the MQTT open broker.
- The `examples` folder contains some previous code for testing and learning.
- The `touchdesigner` folder contains the `.toe` files that Touch Designer can run. They are designed to be run on the specific setup shown, in case
  some adaptation is required both the local network IP and some static files may require to be changed.

## ⚠️ ATTENTION ⚠️

Currently we're not using TinyGo due to many issues with additional drivers. We will work with Arduino from now on, and once the project is complete
we will migrate to TinyGo. Ignore most of the messages below as they imply TinyGo use.

---

## Summary

The aim of this project is to create a program used for a physical dashboard of LEDs. For the first iteration, only an external sound sensor (AKA, a microphone) will be
used to record audio levels and stream them to an external server; at first, this will be the Windows computer connected to the LED dashboard.

The project uses TinyGo as the programming language for small connected devices and the following hardware:

- Raspberry Pi Pico as the microcontroller
- ESP8266 as the WIFI module
- _Electret_ microphone MAX 9814 with automatic gain to record audio levels


## Requirements

In order to run this code in the Raspberry Pis, you will need the following programs:

1. Install Go from the [official webpage](https://go.dev/)
2. Install TinyGo [here](https://tinygo.org/getting-started/). For Linux it's easier, in Windows you will have to download binaries and add TinyGo to your PATH variable ->
in order to do this, follow [this guide](https://www.ntweekly.com/2020/10/01/add-windows-permanent-path-using-powershell/) and remember to run Powershell as admin!
3. If you're using VSCode, installing the Go + TinyGo extensions is very helpful for Intellisense code completion

## How to run the code

...

To debug, you should either use the TinyGo debugger as stated [here](https://tinygo.org/docs/guides/debugging/) or use something like `picocom` like in
[this video](https://www.youtube.com/watch?v=bkRySPeIXeU). **Bear in mind this only works on Linux/MacOS!!!** Windows is really bad and trying to stream to
UART or the USB bus is meh and doesn't work half the time :(

## Helpful documentation

Some helpful tutorials are recorded here:

- [Interactive Go tour](https://go.dev/tour/welcome/1) to learn basic Go
- [Go by Example](https://gobyexample.com/) has nice code examples to learn basic Go
- [TinyGo docs](https://tinygo.org/docs/) are a helpful way to get started as well, but keep in mind that it's more like a wrapper with a different compiler so everything
you learn in plain Go will work the same way
- [This repo](https://github.com/soypat/tinygo-arduino-examples) has good examples, specially the `monitor` Go package to print statements using the UART serial USB port
- [This video](https://www.youtube.com/watch?v=Fl5eFIYU1Xg) has a nice example of running TinyGo on a Raspberry Pi Pico
- [How to solder](https://www.youtube.com/watch?v=QKbJxytERvg), at least it seems easy 🙄
- [How to connect a 16-pin LCD display](https://pimylifeup.com/raspberry-pi-lcd-16x2/), be **very careful** to not let the 5V of the display go to the RPi that uses 3.3V
-> **IT WILL BRICK THE BOARD!!**
- [Another LCD example](https://www.circuitschools.com/interfacing-16x2-lcd-module-with-raspberry-pi-pico-with-and-without-i2c/), this one is easier to set up with the
diagram below (if we had an I2C component to set up it would be much cleaner...):

![16x2 LCD diasplay connection to Raspberry Pi Pico](https://www.circuitschools.com/wp-content/uploads/2021/12/connecting-raspberry-pi-pico-with-lcd-module-without-i2c-adapter.webp)

For the microphone it's a lot easier (the output should go to one of the three available ADC pins):

![Connection diagram](https://hackster.imgix.net/uploads/attachments/1274764/pico_wake_word_bb_qRshYnstCF.png?auto=compress%2Cformat&w=740&h=555&fit=max)

---

## TASK LIST

- [x] Flash code in Pi
- [x] Connect LCD display correctly
- [x] Add LED display code
- [x] Add microphone code (analog output, ADC)
- [x] Add ESP8266 module
- [x] Stream output through MQTT
