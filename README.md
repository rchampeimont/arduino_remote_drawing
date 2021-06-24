# arduino_remote_drawing
An Arduino-based system to synchronize drawings on two remote touchscreen through a Redis server.

Each station is made of the following hardware (so you need all this twice):
* Adafruit RA8875 https://www.adafruit.com/product/1590
* Adafruit 7.0" 40-pin TFT Display - 800x480 with Touchscreen https://www.adafruit.com/product/2354
* Arduino Uno Rev3
* Arduino Uno Wifi Rev2

Here is the complete schematic:
![Complete schematic](/schematics/complete_schematic.png?raw=true)

How to connect the Arduino Uno R3 (the "UX" Arduino) to the RA8875 circuit:
![RA8875 connections](/schematics/RA8875.jpg?raw=true)

The two Arduinos need to be connected to each other using this circuit in both directions (which is why this circuit is present twice):
![Reset circuit](/schematics/reset_circuit.jpg?raw=true)
It allows each Arduino (Arduino 1 above) to reset the other Arduino (Arduino 2 above). The optional part with an LED is if you want to see when a reset is triggered (LED will be on for 1/2 second).

This is useful for instance for crash detection and automatic reboot: Each Arduino pings the other every second. This allows each Arduino to detect when the other one is crashed, and reboot it.

Why use this complex circuit and not simply connect an output pin of one Arduino to the RESET pin of the other? If we did that, we would have the following issues:
* When Arduino 1 above is resetting, it would temporarily pull pow the RESET pin of Arduino A2 low, triggering a reset on the other side. Since they are both connected to each other like this, this would result in a deadlock situation where every RESET pin is pulled low and no one "releases" it. To avoid that, we invert the logic: Arduino 1 pulls Pin 7 high to make Arduino 2 RESET pin go low. This inversion is accomplished with the NPN transistor.
* When an Arduino is not connected to power or is being connected, there are transitory artefactul voltage variations on the pins (here pin 7), which can trigger a reboot. To get rid of these artefactual reboots, we charge a capacitor, and the RESET is triggered when the capacitor is sufficiently charged.
* A last issue is that pulling high the RESET pin (ie. when we don't want to trigger a reset from code) prevents resets from the computer via USB or the reset button, which is really annoying for development. The diode prevents this issue by making our circuit able to pull low but not high the reset pin.

Here is what the circuit behavior look like when a reset on one Arduino is triggered by the other:
![Pin 7 and RESET pin measures on an oscilloscope](/benchmarks/reboot%20system/20210622_190719.jpg?raw=true)
