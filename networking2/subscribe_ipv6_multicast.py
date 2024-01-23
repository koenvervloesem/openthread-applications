"""Subscribe to an IPv6 multicast group.

Copyright (c) 2024 Koen Vervloesem

SPDX-License-Identifier: MIT
"""
import socket
import struct
import sys

MCAST_GRP = sys.argv[1]
sock = socket.socket(socket.AF_INET6, socket.SOCK_DGRAM)
group = socket.inet_pton(socket.AF_INET6, MCAST_GRP)
mreq = group + struct.pack("@I", 0)
sock.setsockopt(socket.IPPROTO_IPV6, socket.IPV6_JOIN_GROUP, mreq)

print(f"Successfully subscribed to multicast group {MCAST_GRP}")
input("Press any key to end the subscription.")
