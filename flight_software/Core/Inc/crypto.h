/**
 * @file crypto.h
 * @brief Lightweight Cryptographic Command Authentication for VELA
 */

#ifndef CRYPTO_H
#define CRYPTO_H

#include <stdint.h>
#include <string.h>
#include <stdbool.h>

/* The Pre-Shared Secret Key (MUST MATCH PYTHON GDS) */
#define SECRET_KEY "VELA_AEROSPACE_2026"

/**
 * @brief Calculates a 32-bit FNV-1a Hash-MAC.
 * 
 * @param key The pre-shared secret key string.
 * @param message The incoming command string.
 * @return uint32_t The generated cryptographic signature.
 */
static inline uint32_t Generate_MAC(const char* key, const char* message)
{
    uint32_t hash = 0x811c9dc5; /* FNV offset basis */
    uint32_t prime = 0x01000193; /* FNV prime */
    
    /* Hash the Key */
    while (*key) {
        hash ^= (uint8_t)*key++;
        hash *= prime;
    }
    
    /* Hash the Message */
    while (*message) {
        hash ^= (uint8_t)*message++;
        hash *= prime;
    }
    
    return hash;
}

/**
 * @brief Verifies if an incoming command's signature is valid.
 * 
 * @param incoming_msg The command string received over UART.
 * @param provided_sig The 32-bit signature received alongside the command.
 * @return true if the signature matches (Authentic).
 * @return false if the signature fails (Spoofed/Corrupted).
 */
static inline bool Verify_Command(const char* incoming_msg, uint32_t provided_sig)
{
    uint32_t expected_sig = Generate_MAC(SECRET_KEY, incoming_msg);
    return (expected_sig == provided_sig);
}

#endif /* CRYPTO_H */