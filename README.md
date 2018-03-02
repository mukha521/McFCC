# McFCC

McFLY's Car Controller. An RC car controller with SBUS input, two PWM outputs and LED strip lights/effects

**This project is still in heavy development stage. I will update readme file and all the info as soon as it'll be a working one**


## The idea
My interest in RC car grew after I've started flying 5-inch race/freestyle quadcopters. So at the moment I decided to build 1/10 rc car, I already had FrSky Taranis radio and couple of frsky sbus-only receivers like r-xsr and xm+ which are cheaper and smaller than the ones with conventional pwm outputs. Also I wanted some LED lights on my rc car with an ability to control them by the radio.
After some web-searching, I found some projects like "sbus to 16pwm" and led controllers that takes pwm signals as an input. Well, I didn't want to make several devices with lots of wires to put into a car. As I have some small experience in avr controllers, I came up with the idea to make controller myself that will meet my needs. Having SBUS, we aren't limited by 3 or 4 channels, there are 16 of them, we even could use of telemetry (I think I'll implement it). Also, there is a room for some other things to turn on and off as we have a lot of arduino pins free...

## Key features
* SBUS input from receiver
* 2 PWM outputs for servo and ESC
* LED strips as lights and effects
* VBAT monitoring (2S or 3S supported)

## Hardware used
I had some leftovers from projects I did earlier...
* [Arduino Pro Mini (3.3v one with ATmega 328p 8MHz)](https://ru.aliexpress.com/item/ProMini-ATmega328P-3-3V-Compatible-for-Arduino-Pro-Mini/32525927539.html)
* [WS2812B RGB LED strip with IP65 water protection. 60 pixels per meter](https://www.aliexpress.com/item/1m-4m-5m-WS2812B-Smart-led-pixel-strip-Black-White-PCB-30-60-144-leds-m/2036819167.html?spm=a2g0v.10010108.1000016/B.1.5c563c72pkbJN9&isOrigTitle=true). 7 as headlights, 7 as rearlights and 5 as an effect strip. 19 pixels in total.
* Some other electronic components like resistors, transistors, etc... I'll post a full list after I'll finish the project.
