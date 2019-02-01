#! /usr/bin/env python3

"""
print interfaces addresses
"""

import netifaces


def main():
    for interface in netifaces.interfaces():
        for link in netifaces.ifaddresses(interface).get(netifaces.AF_INET, []):
            print("IPv4", link['addr'])

    for interface in netifaces.interfaces():
        for link in netifaces.ifaddresses(interface).get(netifaces.AF_INET6, []):
            if link['addr'].find('%') == -1:
                print("IPv6", link['addr'])


if __name__ == "__main__":
    main()
