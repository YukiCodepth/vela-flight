#!/usr/bin/env python3
"""
VELA FLIGHT SOFTWARE - SECURE UPLINK COMMANDER
----------------------------------------------
Generates cryptographically signed commands using a lightweight 
FNV-1a Hash-MAC to prevent command spoofing over open radio links.
"""

import sys
import struct

# The Pre-Shared Secret Key (MUST MATCH THE STM32 C CODE)
SECRET_KEY = "VELA_AEROSPACE_2026"

def generate_fnv1a_mac(key: str, message: str) -> int:
    """Calculates a 32-bit FNV-1a hash of the key + message."""
    data = key.encode('utf-8') + message.encode('utf-8')
    
    hash_val = 0x811c9dc5  # FNV offset basis
    for b in data:
        hash_val ^= b
        hash_val = (hash_val * 0x01000193) & 0xFFFFFFFF # FNV prime
        
    return hash_val

def main():
    print("==================================================")
    print("   VELA FLIGHT - SECURE COMMAND UPLINK TERMINAL   ")
    print("==================================================")
    
    if len(sys.argv) < 2:
        print("Usage: py tools/uplink_commander.py <COMMAND_STRING>")
        print("Example: py tools/uplink_commander.py DEPLOY_SOLAR_PANELS")
        sys.exit(1)
        
    command = sys.argv[1]
    signature = generate_fnv1a_mac(SECRET_KEY, command)
    
    print(f"[TARGET] STM32 Flight Computer")
    print(f"[CMD]    {command}")
    print(f"[KEY]    {SECRET_KEY}")
    print(f"[SIG]    0x{signature:08X}")
    print("==================================================")
    print("-> Ready for RF Transmission.")

if __name__ == "__main__":
    main()