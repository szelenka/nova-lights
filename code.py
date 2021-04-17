import board
import neopixel
import busio
import adafruit_pcf8523
import time
import random
from collections import OrderedDict, namedtuple


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

ColorDuration = namedtuple("ColorDuration", ("color", "duration"))
HourPeriod = namedtuple("HourPeriod", ("start", "color", "secondary"))

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
    BLUE,   # Monday
    CYAN,   # Tuesday
    YELLOW,
    PINK,
    CYAN,
    RED
]

class NovaStar(neopixel.NeoPixel):
    def __init__(self, pin, n, *, fade_duration=0.5, bpp=3, brightness=0.7, auto_write=False, pixel_order=neopixel.GRB):
        super(NovaStar, self).__init__(pin, n, bpp=bpp, brightness=brightness, auto_write=auto_write, pixel_order=pixel_order)
        self._bpp = len(pixel_order)
        self._duration = [1] * n
        self._last_tick = time.monotonic()
        self.cycle_index = 0
        self.cycle_colors = list()
        self.num_pixels = len(self)
        self.pause_duration = fade_duration * 2
        self.cycle_duration = fade_duration / self.num_pixels
        
    def turn_off(self):
        for _ in range(0, self.num_pixels):
            self[_] = OFF
        
        self.show()
        
    def count_pixels_colored(self, color):
        num_on = 0
        for _ in self:
            if _[:self._bpp] == color[:self._bpp]:
                num_on += 1
                
        return num_on
        
    def cycle_between(self, colors=None, cycle_duration=None, pause_duration=None):
        cycle_duration = cycle_duration or self.cycle_duration 
        pause_duration = pause_duration or self.pause_duration
        colors = colors or self.cycle_colors
        self.cycle_colors = colors
        
        try:
            decrement_color = colors[self.cycle_index]
        except IndexError:
            self.cycle_index = 0
            decrement_color = colors[0]
        try:
            increment_color = colors[self.cycle_index + 1]
        except IndexError:
            increment_color = colors[0]
          
        num_on = self.count_pixels_colored(increment_color)
        
        if num_on < self.num_pixels - 1:
            new_pixels = [ColorDuration(increment_color, cycle_duration)] * (num_on + 1) + [ColorDuration(decrement_color, cycle_duration)] * (self.num_pixels - num_on - 1)
        else:
            new_pixels = [ColorDuration(increment_color, pause_duration)] * self.num_pixels             
        
        return self.tick(new_pixels)
        
    def tick(self, next_colors):
        active_colors = set()
        refresh = False
        delta = time.monotonic() - self._last_tick
        current_colors = [_ for _ in self]
        for i, (pixel, cd) in enumerate(zip(current_colors, next_colors)):
            self._duration[i] -= delta
            if self._duration[i] <= 0:
                # change color & update duration
                self._duration[i] = cd.duration
                active_colors.add(cd.color)
                if pixel != cd.color:
                    self[i] = cd.color
                    refresh = True
                
        if refresh is True and len(active_colors) == 1:
            self.cycle_index = (self.cycle_index + 1) % (len(self.cycle_colors) or 1)
                    
        if refresh is True:
            self.show()
            
        self._last_tick = time.monotonic() 
        return
        
    def chase(self, primary_color, secondary_color, duration=None, percentage=0.25, randomness=0.5, movement=1, sections=2):
        duration = duration or self.cycle_duration
        new_pixels = list()
        secondary_size = round(self.num_pixels * percentage)
        secondary_positions = list()
        for i, _ in enumerate(self):
            if _[:self._bpp] == secondary_color[:self._bpp]:
                secondary_positions.append(i)
        
        if len(secondary_positions) == 0 or len(secondary_positions) == self.num_pixels:
            # init sections across neopixels
            secondary_positions = [range(_, _ + secondary_size - 1) for _ in range(0, self.num_pixels, self.num_pixels // sections)]
            secondary_positions = [y for x in secondary_positions for y in x]
            debug(f"init secondary_positions: {secondary_positions}")
                
        new_idx = list()
        if random.uniform(0.0, 1.0) <= randomness:
            # counter clock-wise
            new_idx = [(_ + movement) % self.num_pixels for _ in secondary_positions]
            debug(f"Counter-Clockwise: {new_idx}")
        else:
            # clock-wise
            new_idx = [(_ - movement) % self.num_pixels for _ in secondary_positions]
            debug(f"Clockwise: {new_idx}")
            
        for _ in range(0, self.num_pixels):
            if _ in new_idx:
                cd = ColorDuration(secondary_color, duration)
            else:
                cd = ColorDuration(primary_color, duration)
            new_pixels.append(cd)
            
        return self.tick(new_pixels)
        
    

lights = OrderedDict([
    ("top", NovaStar(board.D6, 16)),
    ("middle", NovaStar(board.D9, 8, pixel_order=neopixel.GRBW)),
    ("bottom", NovaStar(board.D5, 16))
])

# turn off everything
for light in lights.values():
    light.turn_off()
    
    
def random_duration():
    # TODO: find flicker pattern for neopixel
    if random.uniform(0.0, 1.0) > 0.25:
        return random.uniform(0.1, 0.3)
    else:
        return random.uniform(1.0, 2.0)


def prep_hour(hour, minute, second):
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

    
def random_color_sort(neopixels):
    colors = [RED, ORANGE, YELLOW, GREEN, MINT, BLUE, PURPLE, PINK]
    # the neopixel jewel wants WHITE while the 16 wants WHITE_RBG
    return [
        sorted(colors + [WHITE if _.num_pixels == 8 else WHITE_RGB], key=lambda _: random.random())
        for _ in neopixels
    ]
    
    
random_color_cycles = list()
previous_second = None
while True:
    t = rtc.datetime
    if t.tm_sec != previous_second:
        previous_second = t.tm_sec
        print(f"{t.tm_wday} {t.tm_mon:02}/{t.tm_mday:02}/{t.tm_year} @ {t.tm_hour:02}:{t.tm_min:02}:{t.tm_sec:02}")
    
    # execute plan
    primary, secondary, duration, perc = prep_hour(t.tm_hour, t.tm_min, t.tm_sec)
    is_it_friday = t.tm_wday == 5
    is_it_transition = (
        t.tm_hour == hour_periods['morning'].start - 1 and t.tm_min >= 59 and t.tm_sec >= 45
    ) or (
        t.tm_min >= 59 and t.tm_sec >= 45
    )
    if is_it_transition or (is_it_friday and len(random_color_cycles) == 0):
        random_color_cycles = random_color_sort(lights.values())
    elif not is_it_friday and not is_it_transition:
        random_color_cycles = list()
        
    for i, (k, v) in enumerate(lights.items()):
        if primary == OFF and secondary == OFF:
            v.turn_off()
            continue
            
        if is_it_transition:
            v.cycle_between(
                colors=random_color_cycles[i], pause_duration=random.uniform(0.1, 0.25), cycle_duration=random.uniform(0.01, 0.07)
            )
            continue
            
        if is_it_friday or is_it_transition:
            v.cycle_between(
                colors=random_color_cycles[i], pause_duration=random.uniform(0.5, 1.5), cycle_duration=random.uniform(0.01, 0.07)
            )
            continue
            
        if k == "top":
            v.chase(primary, secondary, duration, perc)
        elif k == "middle":
            v.cycle_between(colors=[OFF, WHITE])
        elif k == "bottom":
            v.chase(day_periods[t.tm_wday], OFF, random_duration(), 0.25)
    
    

