# How to change my Wifi password?
If your Wifi password has changed, the remote drawer will not work any more and will display an error saying it failed to connect to Wifi
(FATAL: Connection to Wifi network "[you Wifi name]" failed).

To upload the new Wifi password, first disconnect your remote drawer from the power supply,
and connect the USB port which is on the Wifi Arduino to the computer you used to upload it initially.

![How to connect the Remote Drawer to the computer to change the Wifi password](change_wifi_password.jpg?raw=true)

Open the Arduino application and open the `secrets.h` file which you created during the initial setup.

If you don't have it any more, you need to do again the initial setup as explained in
[the general README](../README.md#wifi-arduino-for-arduino-uno-wifi-rev2)
with both the Wifi credentials and the Redis server credentials.

If you still have the `secrets.h` file, you just need to change the `WIFI_PASSWORD` variable
(and `WIFI_NAME` if your Wifi does not have the same name any more).

Set the port and Arduino hardware correctly:
* Tools -> Board -> Arduino megaAVR Board -> Arduino Uno Wifi Rev2
* Tools -> Registers emulation -> None (ATMEGA4809)
* Port -> Select the port which shown "Arduino Uno Wifi Rev2" in parenthesis after the port name

Click "upload". The remote drawer should restart and then work properly.

You can then disconnect the USB cable from the remote drawer and connect back the power supply.
