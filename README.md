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

[`include/config.h`](include/config.h) is the source of truth for NeoPixel data
pins, pixel counts, brightness, and the middle-light pulse period. Update that
file if the physical wiring or animation timing differs. The QT Py uses 3.3 V
logic; ensure the NeoPixel power and logic wiring is appropriate for the
installed pixels.

## Color and schedule configuration

The shared color palette, hour schedule, and weekday colors are defined in
[`lib/AnimationLogic/AnimationLogic.h`](lib/AnimationLogic/AnimationLogic.h):

- `Color` constants define the RGB and RGBW values used by every animation.
- `HOUR_PERIODS` maps each daily period to its primary and secondary colors.
- `DAY_COLORS` controls the bottom light by weekday.

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

The middle RGBW light uses the deterministic eased pulse period configured by
`MIDDLE_PULSE_PERIOD_MS` in `include/config.h`: half of each period fades from
dark to full white and the other half fades back to dark. It has no pause or
queued catch-up transitions.

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

If the RTC reports invalid time, upload the dedicated clock-setting firmware:

```sh
pio run -e set_rtc --target upload
pio run -e adafruit_qt_py_m0 --target upload
```

The first image injects the computer's current local date and time into a fresh
build and sets the PCF8523. The second restores normal production behavior so
future restarts do not reset the clock.

Run the PlatformIO static analysis, host tests, and firmware builds with:

```sh
pio check -e adafruit_qt_py_m0
pio test -e native
pio run
```
