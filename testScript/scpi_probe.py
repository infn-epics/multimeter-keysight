#!/usr/bin/env python3
"""Send SCPI commands to the 34465A raw socket and print replies.

usage: scpi_probe.py [--host 192.168.190.18] [--port 5025] "*IDN?" "SYST:ERR?" ...
Commands containing '?' wait for a reply. Stop the IOC first, or expect
interleaving with its polling.
"""
import argparse
import socket

ap = argparse.ArgumentParser()
ap.add_argument("--host", default="192.168.190.18")
ap.add_argument("--port", type=int, default=5025)
ap.add_argument("cmds", nargs="+")
args = ap.parse_args()

with socket.create_connection((args.host, args.port), timeout=5) as s:
    f = s.makefile("rwb")
    for c in args.cmds:
        f.write((c + "\n").encode())
        f.flush()
        if "?" in c:
            print(f"{c:30s} -> {f.readline().decode().rstrip()}")
        else:
            print(f"{c:30s}    (sent)")
