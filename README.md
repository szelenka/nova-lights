# nova-lights

PlatformIO/Arduino firmware for the Nova light clock, targeting the
[Adafruit QT Py SAMD21](https://www.adafruit.com/product/4600).

This project is based on
[Charlyn Gonda's AC Nova light clock](https://charlyn.codes/ac-nova-light-clock/).
It supports multiple animations in one cycle, allowing the two larger lights
to flicker while the smaller light breathes.

## Hardware

- Adafruit QT Py SAMD21
- PCF8523 real-time clock connected to the STEMMA QT port
- 16-pixel RGB NeoPixel top light
- 8-pixel RGBW NeoPixel middle light
- 16-pixel RGB NeoPixel bottom light

The default NeoPixel data pins are:

| Light | QT Py pin |
| --- | --- |
| Top | A0 |
| Middle | A1 |
| Bottom | A2 |

Update [`include/config.h`](include/config.h) if the physical wiring differs.
The QT Py uses 3.3 V logic; ensure the NeoPixel power and logic wiring is
appropriate for the installed pixels.

## Color and schedule configuration

The shared color palette, hour schedule, and weekday colors are defined in
[`lib/AnimationLogic/AnimationLogic.h`](lib/AnimationLogic/AnimationLogic.h):

- [`Color` constants](lib/AnimationLogic/AnimationLogic.h#L21-L32) define the
  RGB and RGBW values used by every animation.
- [`HOUR_PERIODS`](lib/AnimationLogic/AnimationLogic.h#L42-L52) maps the start
  of each daily period to its primary and secondary colors.
- [`DAY_COLORS`](lib/AnimationLogic/AnimationLogic.h#L54-L56) controls the
  bottom light by weekday.

`DAY_COLORS` uses the RTClib weekday order: Sunday (`0`), Monday (`1`),
Tuesday (`2`), Wednesday (`3`), Thursday (`4`), Friday (`5`), and Saturday
(`6`).

## Build and upload

Install [PlatformIO](https://platformio.org/), connect the QT Py over USB, then
run:

```sh
pio run
pio run --target upload
pio device monitor
```

The serial monitor runs at 115200 baud. PlatformIO installs the Arduino
NeoPixel and RTClib dependencies declared in `platformio.ini`.

## Test

The middle RGBW light uses a deterministic two-second eased pulse: it fades
from dark to full white in one second, then back to dark in one second. It has
no pause or queued catch-up transitions.

Run the host-side animation tests without connecting hardware:

```sh
pio test -e native
```

The middle-light tests cover key brightness frames, monotonic fade direction,
irregular loop timing, long scheduling delays, and the `millis()` counter
rolling over. Top- and bottom-light tests cover chase section initialization,
movement in both directions, index wraparound, weekday colors, hour-period
blending, and animation-mode priority.

## Hardware validation firmware

The validation environment lets you flash once and inspect representative
clock states without changing the PCF8523 or rebuilding for every timestamp:

```sh
pio run -e validation --target upload
pio device monitor --baud 115200
```

The validation firmware starts on the normal Monday work-period scenario and
freezes its simulated clock there. Enter commands in the serial monitor:

| Command | Action |
| --- | --- |
| `next` or `n` | Select the next scenario and hold it |
| `previous` or `p` | Select the previous scenario and hold it |
| `auto` | Advance every 10 seconds |
| `hold` | Stop automatic advancement |
| `list` | Show all scenarios |
| `status` | Print the current scenario |
| `help` | Show available commands |
| Scenario name | Jump directly to a scenario, such as `blend` or `friday` |

The scenarios cover normal operation, quarter-hour accents, next-period
blending, hourly rainbow transitions, Friday colors, sleep/off, morning, and
Sunday/Saturday bottom-light colors. Scenario times remain frozen so short
time windows cannot expire while the LEDs are being inspected. Flash the normal
`adafruit_qt_py_m0` environment after validation to restore RTC-driven
operation.

Color-to-color ring transitions use a spatially distributed pixel order instead
of walking through adjacent LEDs. Each transition rotates and reverses that
order, avoiding a visible clockwise or counter-clockwise sweep around the
physical ring.

If the RTC reports that its time is unset, set `SET_RTC_TO_BUILD_TIME` to
`true` in `include/config.h` for one build and upload. Then restore it to
`false` and upload again. The setting is off by default so normal firmware
uploads do not overwrite RTC time.
