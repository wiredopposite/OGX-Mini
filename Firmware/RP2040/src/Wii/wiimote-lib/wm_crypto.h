#ifndef WM_CRYPTO_H
#define WM_CRYPTO_H

#include <stdint.h>

struct ext_crypto_state
{
  uint8_t ft[8];
  uint8_t sb[8];
};

void ext_generate_tables(struct ext_crypto_state * state, const uint8_t key[16]);
void ext_encrypt_bytes(const struct ext_crypto_state * state, uint8_t * buffer,
  int addr_offset, int length);

#endif
