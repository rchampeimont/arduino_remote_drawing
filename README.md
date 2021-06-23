# arduino_remote_drawing
An Arduino-based system to synchronize drawings on two remote touchscreen through a Redis server

I used this hardware:
* Adafruit RA8875 https://www.adafruit.com/product/1590
* Adafruit 7.0" 40-pin TFT Display - 800x480 with Touchscreen https://www.adafruit.com/product/2354
* Arduino Uno Rev3
* Arduino Uno Wifi Rev2

How to connect the Arduino Uno R3 to the RA8875 circuit:
![RA8875 connections](/schematics/RA8875.jpg?raw=true)

The two Arduino need to be connected to each other using this circuit in both directions (so this circuit is present twice):
![Reset circuit](/schematics/reset_circuit.jpg?raw=true)
It allows each Arduino to reset the other Arduino.

This is useful for instance for crash detection and automatic reboot: Each Arduino pings the other every second. This allows each Arduino to detect when the other one is crashed, and reboot it.

Why use this complex circuit and not simply connect pin 7 of one Arduino with the RESET pin of the other?
