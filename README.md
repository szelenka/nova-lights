# nova-lights

This is a modification to the blog post here:
- https://charlyn.codes/ac-nova-light-clock/

The main changes center around allowing for multiple animations within a single cycle. This allows the two 
larger lights to "flicker", while the smaller one still "breaths".

## Installing on Feather M4 Express

Download the respective `.mpy` files to place in the `lib` folder on the device:

### For PCF8523 RTC
- https://github.com/adafruit/Adafruit_CircuitPython_Register/releases
- https://github.com/adafruit/Adafruit_CircuitPython_BusDevice/releases
- https://github.com/adafruit/Adafruit_CircuitPython_PCF8523/releases

### For NeoPixels
- https://github.com/adafruit/Adafruit_Blinka/releases
- https://github.com/adafruit/Adafruit_CircuitPython_Pypixelbuf/releases
- https://github.com/adafruit/Adafruit_CircuitPython_NeoPixel/releases
