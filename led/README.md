Little sketch to control 12v LED strip using an Arduino with support for infrared remote and real time clock time based control.

Hardware:
1. [Arduino UNO](https://store.arduino.cc/usa/arduino-uno-rev3)
2. [RGB LED Strip Driver Shield v1.0](https://www.dfrobot.com/product-1032.html)
3. [DS3231 Real Time Clock module](https://www.amazon.com/Holdding-AT24C32-Precision-Temperature-compensated-Oscillator/dp/B00LZCTMJM/ref=as_li_ss_tl?s=electronics&ie=UTF8&qid=1533397234&sr=1-5&keywords=DS3231&linkCode=sl1&tag=howto045-20&linkId=293efb3abb86d2477055e0614cb63207)
4. [12v RGB LED Strip](https://www.bunnings.com.au/lytworx-rgb-colour-changing-led-strip-light-with-sound-sync_p0054466)
5. [An NEC compatible IR remote, similar to the one in this pack](https://www.bunnings.com.au/lytworx-rgb-colour-changing-led-strip-light-with-sound-sync_p0054466)

The sketch provides the following functionality:
* Set the real time clock
  * Send 'd' to the Serial Monitor to set the clock.
* Monitor the real time clock
  * Send 't' to the Serial Monitor to toggle time display
* Set and display colors from IR remote
  * "Red/Green/Blue" buttons sets full red/green/blue
  * Buttons below each color component increase color by 1
  * Buttons two below each color component decrease color by 1  
* "Mode" button sets random color cycling mode
* "Brightness" buttons increase/decrease brightness

* At 8pm set the brightness to zero
* At 7am set the brightness to the default
* "Music" button resets the arduino

Requires the following Arduino libraries:
* PinChangeInterrupt
* IRLremote
* DS3231
