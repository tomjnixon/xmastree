import numpy as np
import cv2

def get_gray(cap):
    ret, frame = cap.read()
    return cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)

def get_brightness(cap):
    gray = get_gray(cap)
    return np.mean(gray)

def skip_to_brigheness(cap, brighness):
    while get_brightness(cap) <= brighness:
        pass

def get_led_times(n):
    flash_time = 5.0*(1.0/25.0)
    # laging by half a period at the end. hax.
    flash_time += (flash_time / 2.0) / 50
    
    # there are 2 flash_time delays before the start of flash 0.
    return [flash_time * (i + 2.5) for i in range(n)]

def get_led_frames(n):
    frame_rate = 24.0
    led_times = get_led_times(n)
    return [int(round(t * frame_rate)) for t in led_times]

def get_frames(cap, frames):
    frameno = 1
    frame = get_gray(cap)
    for req_frameno in frames:
        assert req_frameno >= frameno
        while frameno < req_frameno:
            frame = get_gray(cap)
            frameno += 1
        yield frame

cap = cv2.VideoCapture('input_videos/0.mov')

if cap.isOpened():
    get_gray(cap) # skip the first
    
    start_brightness = get_brightness(cap)
    print "start_brightness: {}".format(start_brightness)
    
    skip_to_brigheness(cap, start_brightness * 2.0)
    
    led_frames = get_led_frames(50)
    for led_no, frame in enumerate(get_frames(cap, led_frames)):
        cv2.imwrite('tmp/{}.png'.format(led_no), frame)

cap.release()
cv2.destroyAllWindows()
