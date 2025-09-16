// Copyright 2023 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#include <stdint.h>
#include <stdio.h>

#include "snrt.h"
#include "snitch_cluster_addrmap.h"
#include "hal_datamover.h"
#include "datamover_utils.h"
#include "data.h"

static inline uint32_t tcdm_offset(void *ptr) {
  return (uint32_t)((uintptr_t)ptr - (uintptr_t)&snitch_cluster_addrmap.cluster);
}

int main() {
  if (snrt_cluster_idx() > 0) return 0;

  const uint32_t in_size_bytes  = SIZE * SIZE * sizeof(uint8_t);
  const uint32_t out_size_bytes = SIZE * SIZE * sizeof(uint8_t);

  printf("[DM-INFO] cluster=%u core(cluster)=%u core(global)=%u\n",
         snrt_cluster_idx(), snrt_cluster_core_idx(), snrt_global_core_idx());

  // Allocate and load buffers on DM core
  static uint8_t *local_in;
  static uint8_t *local_out;
  static uint8_t *local_out2;
  static uint8_t *local_out3bis;
  static uint8_t *local_out3;
  static uint8_t *local_gold;
  static uint8_t *local_gold2;
  static uint8_t *local_gold3;
  if (snrt_is_dm_core()) {
    local_in      = (uint8_t *) snrt_l1_alloc_cluster_local(in_size_bytes, 64);
    local_out     = (uint8_t *) snrt_l1_alloc_cluster_local(out_size_bytes, 64);
    local_gold    = (uint8_t *) snrt_l1_alloc_cluster_local(out_size_bytes, 64);
    local_out2    = (uint8_t *) snrt_l1_alloc_cluster_local(in_size_bytes/2, 64);
    local_gold2   = (uint8_t *) snrt_l1_alloc_cluster_local(in_size_bytes/2, 64);
    local_out3    = (uint8_t *) snrt_l1_alloc_cluster_local(in_size_bytes/4, 64);
    local_out3bis = (uint8_t *) snrt_l1_alloc_cluster_local(in_size_bytes/4, 64);
    local_gold3   = (uint8_t *) snrt_l1_alloc_cluster_local(in_size_bytes/4, 64);

    // DMA input and expected goldens into TCDM
    snrt_dma_start_1d(local_in,    golden_in,   in_size_bytes);
    snrt_dma_start_1d(local_gold,  golden_out,  out_size_bytes);
    snrt_dma_start_1d(local_gold2, golden_out2, in_size_bytes/2);
    snrt_dma_start_1d(local_gold3, golden_out3, in_size_bytes/4);
    snrt_dma_wait_all();
  }

  snrt_cluster_hw_barrier();

  if (snrt_cluster_core_idx() == 0) {
    // Enable Datamover and select mux
    datamover_cg_enable();
    datamover_mux_enable();
    datamover_soft_clear();

    // Variables for programming
    int acq_to = 1000000; int job_id = -1;
    uint32_t in_off = 0u;
    uint32_t out_off = 0u;
    int st = 0;
    int to = 0;

    // Job 1: 8b transpose, 64x64, stride 64
    while ((job_id = datamover_acquire_job()) < 0 && --acq_to) {}
    if (acq_to == 0) { printf("[DM-ERR] acquire timeout (job1)\n"); goto done; }
    in_off  = tcdm_offset(local_in);
    out_off = tcdm_offset(local_out);
    datamover_in_set(in_off);
    datamover_out_set(out_off);
    datamover_len0_set(((64 & 0xFFF) << 12) | (64 & 0xFFF)); // in_d0_len | tot_len
    datamover_len1_set((64 & 0xFFF));                        // out_d0_len
    datamover_in_d0_stride_set(64);
    datamover_out_d0_stride_set(64);
    datamover_in_d1_stride_set(0);
    datamover_out_d1_stride_set(0);
    datamover_in_d2_stride_set(0);
    datamover_out_d2_stride_set(0);
    datamover_transp_mode_set(DATAMOVER_TRANSP_8B);
    printf("[DM-CFG] (job1 8bT) in_off=0x%08x out_off=0x%08x 64x64 stride=64\n", in_off, out_off);
    datamover_trigger_job();

    // Job 2: 16b transpose, 32x32, stride 64
    acq_to = 1000000; job_id = -1;
    while ((job_id = datamover_acquire_job()) < 0 && --acq_to) {}
    if (acq_to == 0) { printf("[DM-ERR] acquire timeout (job2)\n"); goto done; }
    in_off  = tcdm_offset(local_out);
    out_off = tcdm_offset(local_out2);
    datamover_in_set(in_off);
    datamover_out_set(out_off);
    datamover_len0_set(((32 & 0xFFF) << 12) | (32 & 0xFFF));
    datamover_len1_set((32 & 0xFFF));
    datamover_in_d0_stride_set(64);
    datamover_out_d0_stride_set(64);
    datamover_in_d1_stride_set(0);
    datamover_out_d1_stride_set(0);
    datamover_in_d2_stride_set(0);
    datamover_out_d2_stride_set(0);
    datamover_transp_mode_set(DATAMOVER_TRANSP_16B);
    printf("[DM-CFG] (job2 16bT) in_off=0x%08x out_off=0x%08x 32x32 stride=64\n", in_off, out_off);
    datamover_trigger_job();

    // Job 3: 32b transpose, 16x16, stride 64
    acq_to = 1000000; job_id = -1;
    while ((job_id = datamover_acquire_job()) < 0 && --acq_to) {}
    if (acq_to == 0) { printf("[DM-ERR] acquire timeout (job3)\n"); goto done; }
    in_off  = tcdm_offset(local_out2);
    out_off = tcdm_offset(local_out3bis);
    datamover_in_set(in_off);
    datamover_out_set(out_off);
    datamover_len0_set(((16 & 0xFFF) << 12) | (16 & 0xFFF));
    datamover_len1_set((16 & 0xFFF));
    datamover_in_d0_stride_set(64);
    datamover_out_d0_stride_set(64);
    datamover_in_d1_stride_set(0);
    datamover_out_d1_stride_set(0);
    datamover_in_d2_stride_set(0);
    datamover_out_d2_stride_set(0);
    datamover_transp_mode_set(DATAMOVER_TRANSP_32B);
    printf("[DM-CFG] (job3 32bT) in_off=0x%08x out_off=0x%08x 16x16 stride=64\n", in_off, out_off);
    datamover_trigger_job();

    // Job 4: no transpose, 16x16, stride 64
    acq_to = 1000000; job_id = -1;
    while ((job_id = datamover_acquire_job()) < 0 && --acq_to) {}
    if (acq_to == 0) { printf("[DM-ERR] acquire timeout (job4)\n"); goto done; }
    in_off  = tcdm_offset(local_out3bis);
    out_off = tcdm_offset(local_out3);
    datamover_in_set(in_off);
    datamover_out_set(out_off);
    datamover_len0_set(((16 & 0xFFF) << 12) | (16 & 0xFFF));
    datamover_len1_set((16 & 0xFFF));
    datamover_in_d0_stride_set(64);
    datamover_out_d0_stride_set(64);
    datamover_in_d1_stride_set(0);
    datamover_out_d1_stride_set(0);
    datamover_in_d2_stride_set(0);
    datamover_out_d2_stride_set(0);
    datamover_transp_mode_set(DATAMOVER_TRANSP_NONE);
    printf("[DM-CFG] (job4 copy) in_off=0x%08x out_off=0x%08x 16x16 stride=64\n", in_off, out_off);
    datamover_trigger_job();

    // Wait for all jobs to complete
    to = 5000000; do { st = datamover_get_status(); } while (st != 0 && --to);
    if (to == 0) { printf("[DM-ERR] jobs stuck, status=0x%08x\n", st); }

  done:
    // Disable Datamover
    datamover_cg_disable();
  }

  snrt_cluster_hw_barrier();

  // Verify against goldens on core 0
  int errors = 0;
  if (snrt_cluster_core_idx() == 0) {
    errors  = datamover_compare_int((uint64_t*)local_out,  (uint64_t*)local_gold,  SIZE*SIZE/8);
    errors += datamover_compare_int((uint64_t*)local_out2, (uint64_t*)local_gold2, SIZE*SIZE/16);
    errors += datamover_compare_int((uint64_t*)local_out3, (uint64_t*)local_gold3, SIZE*SIZE/32);
    if (errors == 0) {
      printf("[DM-OK] 8b/16b/32b transpose + copy sequence passed.\n");
    } else {
      printf("[DM-ERR] mismatches: %d\n", errors);
    }
    *(volatile uint32_t *)0x80000000 = (uint32_t)errors;
    *(volatile uint32_t *)0x80000004 = 1u; // done
  }

  return errors;
}
