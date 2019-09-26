# ESP32-Examples
This repository contains example projects for ESP32 boards. I used a ESP32 DEVKIT V1 with 30 pins to create them.
Main reason for this repository is to keep  working examples for my private use. If you can re-use any of them, feel free
to use the examples as starting point.

Please note, most o these examples are based on official ESP32 examples provided by Espressif. Also, I use duff2013 ulptool 
to program ULP co-processor using Arduino IDE instead of esp-idf (https://github.com/duff2013/ulptool).

#Examples

ESP32_ULP_infinite_count: This project uses ULP to count as long as it is powered on. ESP core uses loop to print count values every few seconds

ESP32_ULP_simple_count: ULP count pulses of one GPIo while ESP sleeps. After a pre-defined count of pulses are reached, ULP wakes ESP to print value

ESP32_ULP_multi_count: ULP count pulses on two GPIOs independently. ESP32 sleeps and wakes up by timer. After wake up, it prints counted pulses and starts sleeping again.
