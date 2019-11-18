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

#include "chacha20poly1305/ecrypt-sync.h"
#include "hmac_drbg.h"
#include "rand.h"

#define BUFFER_LENGTH 128
#define RESEED_AFTER_BYTES 0

static HMAC_DRBG_CTX drbg_ctx;
static uint8_t buffer[BUFFER_LENGTH];
static uint8_t buffer_index;
static uint32_t bytes_since_reseed;
static bool initialized = false;
static uint8_t session_delay;

static void rdi_reseed(void) {
  uint8_t entropy[48];
  random_buffer(entropy, sizeof(entropy));
  hmac_drbg_reseed(&drbg_ctx, entropy, sizeof(entropy), NULL, 0);
}

void buffer_refill(void) {
  hmac_drbg_generate(&drbg_ctx, buffer, BUFFER_LENGTH);
}

static uint32_t random8(void) {
  buffer_index += 1;
  bytes_since_reseed += 1;
  if (buffer_index >= BUFFER_LENGTH) {
    if (RESEED_AFTER_BYTES != 0)
    {
      bytes_since_reseed += BUFFER_LENGTH;
      if (bytes_since_reseed > RESEED_AFTER_BYTES) {
      	rdi_reseed();
      	bytes_since_reseed = 0;
      }
    }
    buffer_refill();
    buffer_index = 0;
  }

  return buffer[buffer_index];
}

void rdi_regenerate_session_delay(void) { session_delay = random8(); }

void rdi_start(void) {
  rdi_reseed();
  bytes_since_reseed = 0;
  buffer_refill();
  buffer_index = 0;
  initialized = true;
  rdi_regenerate_session_delay();
}

void rdi_stop(void) { initialized = false; }

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
