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
const union {
    struct {
        int8_t x, y, z, rz, ry, rx;
    };
    int8_t axes[3];
} led_positions[] = {
""");
        for pos in leds:
            axes  = list(pos) + [
                    int(127 * np.arctan2(pos[0], pos[1]) / np.pi),
                    int(127 * np.arctan2(pos[0], pos[2]) / np.pi),
                    int(127 * np.arctan2(pos[1], pos[2]) / np.pi)
            ]
            out.write("    {" + ", ".join(map(str, axes)) + "},\n")

        out.write("};\n")
        
        out.write("const int8_t axis_min[3] = {" + ", ".join(map(str, leds.min(0))) + "};\n")
        out.write("const int8_t axis_max[3] = {" + ", ".join(map(str, leds.max(0))) + "};\n")
        out.write("#endif\n")

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

