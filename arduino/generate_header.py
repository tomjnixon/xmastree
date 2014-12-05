#!/usr/bin/env python2
import csv
import numpy as np

def read_positions(in_f):
    positions = [
        map(float, [row["x"], row["y"], row["z"]])
        for row
        in csv.DictReader(open(in_f))]
    return np.array(positions)

def normalise(p):
    max_range = np.max(p.max(0) - p.min(0)) / 2.0
    center = (p.max(0) + p.min(0)) / 2
    norm = (p - center) / max_range
    as_char = (norm * 125.0).astype(np.int8)
    
    return as_char

def write_header(out_f, leds):
    with open(out_f, "w") as out:
        out.write(
"""#ifndef LEDS_H
#define LEDS_H
const uint8_t led_positions[][3] = {
""");
        for pos in leds:
            out.write("    {{{0[0]}, {0[1]}, {0[2]}}},\n".format(pos))

        out.write(
"""
};
#endif
""");

def run(in_f, out_f):
    positions = read_positions(in_f)
    norm = normalise(positions)
    write_header(out_f, norm)

if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("input", help="Input csv file.")
    parser.add_argument("output", help="Output header file.")
    args = parser.parse_args()
    run(args.input, args.output)

