/*
 * This file is part of the Trezor project, https://trezor.io/
 *
 * Copyright (c) SatoshiLabs
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include STM32_HAL_H

#include <stdbool.h>

#include "rdi.h"
#include "rand.h"

#define MAX(x,y) (((x)>(y))?(x):(y))

void chacha_drbg_init(CHACHA_DRBG_CTX *ctx, const uint8_t entropy[CHACHA_DRBG_SEED_LENGTH]) {
  uint8_t buffer[MAX(CHACHA_DRBG_KEY_LENGTH, CHACHA_DRBG_IV_LENGTH)] = {0};
  ECRYPT_keysetup(&ctx->chacha_ctx, buffer, CHACHA_DRBG_KEY_LENGTH * 8, CHACHA_DRBG_IV_LENGTH * 8);
  ECRYPT_ivsetup(&ctx->chacha_ctx, buffer);

  chacha_drbg_reseed(ctx, entropy);
}

static void chacha_drbg_update(CHACHA_DRBG_CTX *ctx, const uint8_t data[CHACHA_DRBG_SEED_LENGTH]) {
  uint8_t buffer[CHACHA_DRBG_SEED_LENGTH] = {0};

  if (data)
    ECRYPT_encrypt_bytes(&ctx->chacha_ctx, data, buffer, CHACHA_DRBG_SEED_LENGTH);
  else
    ECRYPT_keystream_bytes(&ctx->chacha_ctx, buffer, CHACHA_DRBG_SEED_LENGTH);

  ECRYPT_keysetup(&ctx->chacha_ctx, buffer, CHACHA_DRBG_KEY_LENGTH * 8, CHACHA_DRBG_IV_LENGTH * 8);
  ECRYPT_ivsetup(&ctx->chacha_ctx, buffer + CHACHA_DRBG_KEY_LENGTH);
}

void chacha_drbg_generate(CHACHA_DRBG_CTX *ctx, uint8_t *output, uint8_t output_length) {
  ECRYPT_keystream_bytes(&ctx->chacha_ctx, output, output_length);
  chacha_drbg_update(ctx, NULL);
  ctx->reseed_counter++;
}

void chacha_drbg_reseed(CHACHA_DRBG_CTX *ctx, const uint8_t entropy[CHACHA_DRBG_SEED_LENGTH]) {
  chacha_drbg_update(ctx, entropy);
  ctx->reseed_counter = 1;
}

#define BUFFER_LENGTH 64
#define RESEED_INTERVAL 0

static CHACHA_DRBG_CTX drbg_ctx;
static uint8_t buffer[BUFFER_LENGTH];
static uint8_t buffer_index;
static bool initialized = false;
static uint8_t session_delay;

static void rdi_reseed(void) {
  uint8_t entropy[CHACHA_DRBG_SEED_LENGTH];
  random_buffer(entropy, CHACHA_DRBG_SEED_LENGTH);
  chacha_drbg_reseed(&drbg_ctx, entropy);
}

void buffer_refill(void) {
  chacha_drbg_generate(&drbg_ctx, buffer, BUFFER_LENGTH);
}

static uint32_t random8(void) {
  buffer_index += 1;
  if (buffer_index >= BUFFER_LENGTH) {
    buffer_refill();
    if (drbg_ctx.reseed_counter > RESEED_INTERVAL)
      rdi_reseed();
    buffer_index = 0;
  }
  return buffer[buffer_index];
}

void rdi_regenerate_session_delay(void) { session_delay = random8(); }

void rdi_start(void) {
  uint8_t entropy[CHACHA_DRBG_SEED_LENGTH];
  random_buffer(entropy, CHACHA_DRBG_SEED_LENGTH);
  chacha_drbg_init(&drbg_ctx, entropy);
  buffer_refill();
  buffer_index = 0;
  initialized = true;
  rdi_regenerate_session_delay();
}

void rdi_stop(void) {
  initialized = false;
  memzero(&drbg_ctx, sizeof(drbg_ctx));
}

void rdi_handler(void) {
  if (initialized) {
    uint32_t delay = random8() + session_delay;

    asm volatile(
        "ldr r0, %0;"  // r0 = delay
        "add r0, $3;"  // r0 += 3
        "loop:"
        "subs r0, $3;"  // r0 -= 3
        "bhs loop;"     // if (r0 >= 3): goto loop
        // loop ((delay // 3) + 1) times
        // one extra loop ensures that branch predictor learns the loop
        // every loop takes 3 ticks
        // r0 == (delay % 3) - 3
        "lsl r0, $1;"      // r0 *= 2
        "add r0, $4;"      // r0 += 4
        "rsb r0, r0, $0;"  // r0 = -r0
        // r0 = 2 if (delay % 3 == 0) else 0 if (delay % 3 == 1) else -2 if
        // (delay % 3 == 2)
        "add pc, r0;"  // jump (r0 + 2)/2 instructions ahead
        // jump here if (delay % 3 == 2)
        "nop;"  // wait one tick
        // jump here if (delay % 3 == 1)
        "nop;"  // wait one tick
        // jump here if (delay % 3 == 0)
        :
        : "m"(delay)
        : "r0");  // wait (18 + delay) ticks
  }
}
