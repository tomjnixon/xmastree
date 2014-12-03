#!/usr/bin/env python2
import numpy as np
import cv2
import os.path as path

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

def get_brightest(frame):
    # blur the frame, then find the brightest pixel
    # halved to avoid lots of possible maximum values; we want the center
    blurred = cv2.GaussianBlur(frame * 0.5,(5,5),0)
    brightest = np.unravel_index(blurred.argmax(), blurred.shape)
    brightness = blurred[brightest] * 2.0
    brightest = (brightest[1], brightest[0]) # x then y pos
    return brightest, brightness

def run(input_file, output_file, debug_dir=None):
    cap = cv2.VideoCapture(input_file)
    assert cap.isOpened()

    if cap.isOpened():
        get_gray(cap) # skip the first
        
        start_brightness = get_brightness(cap)
        print "start_brightness: {}".format(start_brightness)
        ret, bright_ref = cap.read()
        
        skip_to_brigheness(cap, start_brightness * 2.0)
        
        led_frames = get_led_frames(50)
        for led_no, frame in enumerate(get_frames(cap, led_frames)):
            brightest, brightness = get_brightest(frame)
            
            # draw and write
            if debug_dir is not None:
                colored = cv2.cvtColor(frame, cv2.COLOR_GRAY2BGR)
                cv2.circle(colored, brightest, 5, (0, 255, 0), -1)
                out_path = path.join(debug_dir, '{}.png'.format(led_no))
                cv2.imwrite(out_path, colored)

    cap.release()

if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("input", help="Input video file.")
    parser.add_argument("output", help="Output csv file.")
    parser.add_argument("-d", "--debug_dir",
            help="Dir to write images to with their detected points.")
    args = parser.parse_args()
    run(args.input, args.output, args.debug_dir)
