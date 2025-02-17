# Sonos volume controller

This is the code for a standalone Sonos volume controller. It utilizes a [rotary encoder](https://www.aliexpress.com/item/1005005973850924.html) and a [Wemos D1 Mini](https://www.wemos.cc/en/latest/d1/d1_mini.html) arduino clone ([available at Kjell.com](http://kjell.com/no/produkter/elektro-og-verktoy/elektronikk/utviklerkit/arduino/utviklingskort/luxorparts-wemos-d1-mini-utviklingskort-p87294)). It is created with the objective to be as responsive as possible, as a volume knob should be. The rotary encoder also has a button function, which is used to play/pause what's currently playing.

The only configuration needed is found as definitions on the top of the .ino file, and are as follows:

```
ROOM_NAME: Name of the Sonos room you want to control
SSID: Wifi network name
PASSWORD: Wifi password
```

Feel free to paste this code into any GPT for help with debugging and implementation.
