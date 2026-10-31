#!/usr/bin/env python3
"""Summarise a benchmark run from the ESP32 (the H, and Q, lines printed by bench.c).

usage: tests/bench_stats.py results/17-benchmark-warm-reused.txt [--from-round N]
"""
import argparse
import math
import statistics
from collections import defaultdict


def p95(values):
    # nearest rank: the value at position ceil(0.95 * n) of the sorted list
    v = sorted(values)
    return v[math.ceil(0.95 * len(v)) - 1]


def row(label, ms):
    return "%-26s %5d %9.1f %9.1f %9.1f %9.1f %9.1f" % (
        label, len(ms), statistics.mean(ms), statistics.median(ms), p95(ms), min(ms), max(ms))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("file")
    ap.add_argument("--from-round", type=int, default=1, help="skip the rounds before this one")
    args = ap.parse_args()

    per_name, handshakes, heap, errors = defaultdict(list), [], [], 0
    for line in open(args.file):
        f = line.strip().split(",")
        if f[0] == "Q" and int(f[1]) >= args.from_round:
            per_name[f[2]].append(int(f[6]) / 1000)      # latency_us -> ms
            heap.append(int(f[7]))
        elif f[0] == "H":
            handshakes.append(int(f[2]) / 1000)          # handshake_us -> ms
        elif f[0] == "E":
            errors += 1

    print("query latency in ms, rounds from %d" % args.from_round)
    print("%-26s %5s %9s %9s %9s %9s %9s" % ("name", "n", "mean", "median", "p95", "min", "max"))
    for name in sorted(per_name):
        print(row(name, per_name[name]))
    print(row("all queries", [x for v in per_name.values() for x in v]))
    if handshakes:
        print("\nTLS handshakes: " + ", ".join("%.0f ms" % h for h in handshakes))
    print("free heap: min %d, max %d bytes. errors: %d" % (min(heap), max(heap), errors))


main()
