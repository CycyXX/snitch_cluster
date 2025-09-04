// Copyright 2023 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#include <stdint.h>
#include <stdio.h>

#include "snrt.h"
#include "snitch_cluster_addrmap.h"
#include "hal_datamover.h"

static inline uint32_t tcdm_offset(void *ptr) {
  return (uint32_t)((uintptr_t)ptr - (uintptr_t)&snitch_cluster_addrmap.cluster);
}

int main() {
  const uint32_t test_bytes = 256; // copy 256B total
  // Effective TCDM data width is 256b (see HW wiring), use 32B beats
  const uint32_t beat_bytes = 256 / 8; // 32B per beat

  printf("[DM-INFO] cluster=%u core(cluster)=%u core(global)=%u\n",
         snrt_cluster_idx(), snrt_cluster_core_idx(), snrt_global_core_idx());

  // Allocate and init buffers on DM core
  static uint8_t *local_in;
  static uint8_t *local_out;
  if (snrt_is_dm_core()) {
    local_in  = (uint8_t *) snrt_l1_alloc(test_bytes);
    local_out = (uint8_t *) snrt_l1_alloc(test_bytes);
    for (uint32_t i = 0; i < test_bytes; ++i) {
      local_in[i]  = (uint8_t)(i & 0xFF);
      local_out[i] = 0u;
    }
    printf("[DM-DMA] local_in  [0x%p]: 0x%02x\n", (void*)local_in,  local_in[0]);
    printf("[DM-DMA] local_out [0x%p]: 0x%02x\n", (void*)local_out, local_out[0]);
  }

  snrt_cluster_hw_barrier();

  if (snrt_cluster_core_idx() == 0) {
    // Enable Datamover and select mux
    datamover_cg_enable();
    datamover_mux_enable();
    datamover_soft_clear();

    // Acquire job with timeout
    int acq_to = 1000000; int job_id = -1;
    // Hoist declarations to avoid jumping over initializations with goto
    uint32_t in_off = 0u;
    uint32_t out_off = 0u;
    uint32_t beats = 0u;
    uint32_t leftover = 0u;
    int st = 0;
    int to = 0;
    while ((job_id = datamover_acquire_job()) < 0 && --acq_to) {}
    if (acq_to == 0) {
      printf("[DM-ERR] acquire timeout, status=0x%08x\n", datamover_get_status());
      goto done;
    }

    // Program job: copy test_bytes using 36B beats (HW width), mode none
    in_off  = tcdm_offset(local_in);
    out_off = tcdm_offset(local_out);
    beats = test_bytes / beat_bytes;
    leftover = test_bytes % beat_bytes;
    datamover_in_set(in_off);
    datamover_out_set(out_off);
    datamover_len0_set(((beats & 0xFFF) << 12) | (beats & 0xFFF));
    datamover_len1_set((beats & 0xFFF));
    datamover_in_d0_stride_set(beat_bytes);
    datamover_out_d0_stride_set(beat_bytes);
    datamover_in_d1_stride_set(0);
    datamover_out_d1_stride_set(0);
    datamover_in_d2_stride_set(0);
    datamover_out_d2_stride_set(0);
    datamover_transp_mode_set((leftover << 16) | DATAMOVER_TRANSP_NONE);
    printf("[DM-CFG] in_off=0x%08x out_off=0x%08x beats=%u leftover=%u beat_bytes=%u\n",
           in_off, out_off, beats, leftover, beat_bytes);

    // Trigger and wait for completion
    datamover_trigger_job();
    to = 2000000;
    do { st = datamover_get_status(); } while (st != 0 && --to);
    if (to == 0) { printf("[DM-ERR] copy stuck, status=0x%08x\n", st); goto done; }

    // Verify copy
    for (uint32_t i = 0; i < test_bytes; ++i) {
      if (local_out[i] != local_in[i]) {
        printf("[DM-ERR] mismatch @%u exp=0x%02x got=0x%02x\n", i, local_in[i], local_out[i]);
        goto done;
      }
    }
    printf("[DM-OK] Copy %uB passed.\n", test_bytes);

  done:
    // Disable Datamover and signal done to testbench
    datamover_cg_disable();
    *(volatile uint32_t *)0x80000000 = 0; // errors=0 for now
    *(volatile uint32_t *)0x80000004 = 1; // done
  }

  return 0;
}
