import board
import neopixel
import busio
import adafruit_pcf8523
import time
import random
from collections import OrderedDict, namedtuple
from enum import Enum


class TimeOfDay(Enum):
    NIGHTTIME = 0
    HOUR_TRANSITION = 1
    FRIDAY = 2
    NORMAL = 3


HourPeriod = namedtuple("HourPeriod", ("start", "color", "secondary"))

is_debug = False


def debug(*args, **kwargs):
    if is_debug:
        print(*args, **kwargs)
   

# setup real-time clock
myI2C = busio.I2C(board.SCL, board.SDA)
rtc = adafruit_pcf8523.PCF8523(myI2C)

# configure the time on the RTC
if False:
    #                     year, mon, date, hour, min, sec, wday, yday, isdst
    t = time.struct_time((2021,  02,   01,   20,  09,   00,   1,   -1,    -1))
    # you must set year, mon, date, hour, min, sec and weekday
    # yearday is not supported, isdst can be set but we don't do anything with it at this time
    debug(f"Setting time to: {t}")
    rtc.datetime = t


# COLORS
OFF = (0, 0, 0, 0)
WHITE = (0, 0, 0, 255)

YELLOW = (200, 155, 0, 0)
GREEN = (50, 150, 50, 0)
BLUE = (0, 0, 100, 0)
PURPLE = (180, 50, 180, 0)
PINK = (231, 84, 128, 0)
ORANGE = (155, 50, 0, 0)
WHITE_RGB = (100, 100, 100, 0)

RED = (100, 5, 5, 0)
CYAN = (0, 100, 100, 0)
MINT = (62, 180, 137, 0)


hour_periods = OrderedDict([
    ("morning", HourPeriod(7, BLUE, OFF)),
    ("work", HourPeriod(9, MINT, OFF)),
    ("break", HourPeriod(11, PINK, OFF)),
    ("lunch", HourPeriod(12, GREEN, OFF)),
    ("afternoon", HourPeriod(13, YELLOW, OFF)),
    ("sunset", HourPeriod(16, RED, OFF)),
    ("dinner", HourPeriod(18, ORANGE, OFF)),
    ("bed", HourPeriod(19, PURPLE, OFF)),
    ("sleep", HourPeriod(21, OFF, OFF))
])

day_periods = [
    WHITE_RGB,  # Sunday
    BLUE,       # Monday
    CYAN,       # Tuesday
    YELLOW,
    PINK,
    CYAN,
    RED
]

timestamp = rtc.datetime

time_of_day = TimeOfDay.NIGHTTIME

lights = OrderedDict([
    ("top", neopixel.NeoPixel(board.D6, 16)),
    ("middle", neopixel.NeoPixel(board.D9, 8, pixel_order=neopixel.GRBW)),
    ("bottom", neopixel.NeoPixel(board.D5, 16))
])

random_color_cycles = random_color_sort()


async def get_time():
    global rtc, timestamp, time_of_day, hour_periods
    t = rtc.datetime
    print(f"{t.tm_wday} {t.tm_mon:02}/{t.tm_mday:02}/{t.tm_year} @ {t.tm_hour:02}:{t.tm_min:02}:{t.tm_sec:02}")
    if t.tm_hour >= hour_periods['sleep'].start and t.tm_hour < hour_periods['morning'].start:
        time_of_day = TimeOfDay.NIGHTTIME
        await tasko.sleep(60)
    elif (
        t.tm_hour == hour_periods['morning'].start - 1 and t.tm_min >= 59 and t.tm_sec >= 45
    ) or (
        t.tm_min >= 59 and t.tm_sec >= 45
    ):
        time_of_day = TimeOfDay.HOUR_TRANSITION
        random_color_cycles = random_color_sort()
    elif t.tm_wday == 5:
        time_of_day = TimeOfDay.FRIDAY
    else:
        time_of_day = TimeOfDay.NORMAL

    # TODO: move 'prep_hour' logic into here
    timestamp = t


async def turn_off(pixel):
    pixel.fill(OFF)
    pixel.show()
    await tasko.sleep(60)


async def cycle_between(pixel, colors=None, cycle_duration=None, pause_duration=None):
    num_leds = len(pixel)
    duration = cycle_duration // num_leds
    for c in colors:
        for i in range(num_leds):
            pixel[i] = colors[c]
            pixel.show()
            await tasko.sleep(duration)

        await tasko.sleep(pause_duration)


async def chase(pixel, primary_color, secondary_color, duration=0.1, percentage=0.25, randomness=0.5, movement=1, sections=2):
    bpp = None # TODO: discover bpp from pixel
    num_leds = len(pixel)
    secondary_size = round(num_leds * percentage)
    secondary_positions = list()
    for i, _ in enumerate(pixel):
        if _[:bpp] == secondary_color[:bpp]:
            secondary_positions.append(i)

    if len(secondary_positions) == 0 or len(secondary_positions) == num_leds:
        # init sections across neopixels
        secondary_positions = [range(_, _ + secondary_size - 1) for _ in range(0, num_leds, num_leds // sections)]
        secondary_positions = [y for x in secondary_positions for y in x]
        debug(f"init secondary_positions: {secondary_positions}")
            
    new_idx = list()
    if random.uniform(0.0, 1.0) <= randomness:
        # counter clock-wise
        new_idx = [(_ + movement) % num_leds for _ in secondary_positions]
        debug(f"Counter-Clockwise: {new_idx}")
    else:
        # clock-wise
        new_idx = [(_ - movement) % num_leds for _ in secondary_positions]
        debug(f"Clockwise: {new_idx}")
    
    for i in range(num_leds):
        pixel[i] = secondary_color if _ in new_idx else primary_color

    pixel.show()
    await tasko.sleep(duration)

    
def random_duration():
    # TODO: find flicker pattern for neopixel
    if random.uniform(0.0, 1.0) > 0.25:
        return random.uniform(0.1, 0.3)
    else:
        return random.uniform(1.0, 2.0)


def prep_hour(hour, minute, second):
    global timestamp, hour_periods
    hour, minute, second = timestamp.tm_hour, timestamp.tm_min, timestamp.tm_sec
    values = [_ for _ in hour_periods.values()]
    for _old, _new in zip(values, values[1:]+[values[0]]):
        if _old.start <= hour < _new.start:
            if minute in [15, 30, 45] and 0 <= second < 3:
                return _old.color, _old.secondary, random_duration(), 0.75
            elif _new.start - 1 == hour:
                if minute == 59 and 50 <= second <= 59:
                    return _new.color, _old.color, 0.01, 0.5
                elif minute >= 55:
                    return _old.color, _new.color, random_duration(), (minute - 55) / 5
                
            return _old.color, _old.secondary, random_duration(), 0.25
            
            
    debug(f"Unable to locate period matching hour: {hour}")
    return OFF, OFF, 1.0, 1.0 

    
def random_color_sort():
    global lights
    colors = [RED, ORANGE, YELLOW, GREEN, MINT, BLUE, PURPLE, PINK]
    # the neopixel jewel wants WHITE while the 16 wants WHITE_RBG
    return [
        sorted(colors + [WHITE if _.num_pixels == 8 else WHITE_RGB], key=lambda _: random.random())
        for _ in lights
    ]
    


async def action_plan(idx, pixel):
    global random_color_cycles, time_of_day, day_periods
    if time_of_day == TimeOfDay.NIGHTTIME:
        return turn_off(
            pixel=pixel
        )
    elif time_of_day == TimeOfDay.HOUR_TRANSITION:
        await cycle_between(
            pixel=pixel,
            colors=random_color_cycles[idx],
            pause_duration=random.uniform(0.1, 0.25), 
            cycle_duration=random.uniform(0.01, 0.07)
        )
    elif time_of_day == TimeOfDay.FRIDAY or time_of_day == TimeOfDay.HOUR_TRANSITION:
        await cycle_between(
            pixel=pixel,
            colors=random_color_cycles[idx], 
            pause_duration=random.uniform(0.5, 1.5), 
            cycle_duration=random.uniform(0.01, 0.07)
        )
    elif idx == 0:
        primary, secondary, duration, perc = prep_hour()
        await chase(pixel, primary, secondary, duration, perc)
    elif idx == 1:
        await cycle_between(pixel, colors=[OFF, WHITE])
    elif idx == 2:
        await chase(pixel, day_periods[t.tm_wday], OFF, random_duration(), 0.25)


# ---------- Tasko wiring begins here ---------- #
# Schedule the workflows at whatever frequency makes sense
# get time every second
tasko.schedule(hz=48,  coroutine_function=get_time)

for i, v in enumerate(lights.values()):
    tasko.schedule(hz=10,  coroutine_function=action_plan, idx=i, pixel=v)

# And let tasko do while True
tasko.run()
# ----------  Tasko wiring ends here  ---------- #