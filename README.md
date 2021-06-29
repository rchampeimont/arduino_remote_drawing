# arduino_remote_drawing
An Arduino-based system to synchronize drawings on two remote touchscreens through a Redis server.

Here is the general overview of the whole system:
![General overview](/schematics/overview.jpg?raw=true)

Each station is made of the following hardware (so you need all this twice):
* Arduino Uno Wifi Rev2
* Arduino Uno Rev3
* Adafruit RA8875 https://www.adafruit.com/product/1590
* Adafruit 7.0" 40-pin TFT Display - 800x480 with Touchscreen https://www.adafruit.com/product/2354
* A phototransistor: I used the one in the Arduino starter kit (https://www.arduino.cc/documents/datasheets/HW5P-1.pdf) but you can probably use another one. This is used to adapt the brightness of the screen according to ambiant light, but this is not stricly necessary (replace the phototransistor with a wire if you don't want this feature).
* Some standard components: resistors, diodes, capacitors and NPN transistors. See schematic below for details.

As you can see in the diagram above, you also need a Redis server that can be accessed by the two stations. Here we use Redis to store the drawing and to create a pub/sub channel so that each station gets changes in real-time (a few seconds). So you need to have a Linux server running around the clock for the project to work. But if you already have one, adding a Redis server is very simple (you just need to set port number and password) and will require very little resources (Redis uses only a few megabytes of RAM). Learn more about Redis here: https://redis.io/

Here is the complete schematic for one station:
![Complete schematic](/schematics/complete_schematic.png?raw=true)

## "Wifi" Arduino (for Arduino Uno Wifi Rev2)
To compile the code for the "Wifi" Arduino (remote_drawing_wifi directory), you need to rename the secrets.txt file and you put your own secrets in it (Wifi password and Redis host and password).

## "UX" Arduino (for Arduino Uno Rev3)
The code for the UX Arduino (remote_drawing_ux directory) does not need any secrets and can be compiled and uploaded to an Arduino Uno directly.

The first time you run the program, it is going to ask you to calibrate the touch screen. It then stores the calibration data in EEPROM to skip calibration on future restarts. If you want to force recalibration, you can tie pin 5 to ground and restart the Arduino.

## Powering
To power the system, you can of course connect a USB cable to each Arduino.

In my case, I connect a 9V adapter to the jack input on the Arduino Uno Wifi R2, then I connect the 5V of both Arduinos together, which means that all voltage conversion from 9V to 5V is done by the Arduino Uno Wifi R2 (with an MPM3610 chip), which does that more efficiently (it heats less) than the Arduino Uno (which does it with an NCP1117 chip).

Here are some benchmarks showing that (intensity is measured on 9V adapter output):
* 9V to VIN of Arduino Uno Wifi R2 and 5V tied together: 0.17 A
* 9V to VIN of Arduino Uno R3 and 5V tied together: 0.25 A
* 9V to both VINs tied together (5V are not tied): 0.24 A

## More details on the "reset circuit"
As you can see in the schematic above, this circuit is present twice:
![Reset circuit](/schematics/reset_circuit.jpg?raw=true)
It allows each Arduino (Arduino 1 above) to reset the other Arduino (Arduino 2 above). The optional part with an LED is if you want to see when a reset is triggered (LED will be on for 1/2 second).

This is useful for instance for crash detection and automatic reboot: Each Arduino pings the other every second. This allows each Arduino to detect when the other one is crashed, and reboot it.

Why use this complex circuit and not simply connect an output pin of one Arduino to the RESET pin of the other? If we did that, we would have the following issues:
* When Arduino 1 above is resetting, it would temporarily pull pow the RESET pin of Arduino A2 low, triggering a reset on the other side. Since they are both connected to each other like this, this would result in a deadlock situation where every RESET pin is pulled low and no one "releases" it. To avoid that, we invert the logic: Arduino 1 pulls Pin 7 high to make Arduino 2 RESET pin go low. This inversion is accomplished with the NPN transistor.
* When an Arduino is not connected to power or is being connected, there are transitory artefactul voltage variations on the pins (here pin 7), which can trigger a reboot. To get rid of these artefactual reboots, we charge a capacitor, and the RESET is triggered when the capacitor is sufficiently charged.
* A last issue is that pulling high the RESET pin (ie. when we don't want to trigger a reset from code) prevents resets from the computer via USB or the reset button, which is really annoying for development. The diode prevents this issue by making our circuit able to pull low but not high the reset pin.

Here is what the circuit behavior look like when a reset on one Arduino is triggered by the other:
![Pin 7 and RESET pin measures on an oscilloscope](/benchmarks/reboot%20system/20210622_190719.jpg?raw=true)
