// Copyright 2023 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0
//
// Authors: Cyrill Durrer <cdurrer@iis.ee.ethz.ch>
//          Daniel Keller <dankeller@student.ethz.ch>

#include "hal_datamover.h"
#include "datamover_utils.h"
#include "snitch_cluster_addrmap.h"
#include "snrt.h"
#include "data.h"

#include <stdint.h>
#include <stdio.h>


int main() {
  if (snrt_cluster_idx() > 0) return 0;

  const uint32_t size_m = SIZE;
  const uint32_t size_n = SIZE;
  const uint32_t tot_size = size_m * size_n;
  datamover_status_t datamover_status;

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
    local_in      = (uint8_t *) snrt_l1_alloc_cluster_local(tot_size, 64);
    local_out     = (uint8_t *) snrt_l1_alloc_cluster_local(tot_size, 64);
    local_gold    = (uint8_t *) snrt_l1_alloc_cluster_local(tot_size, 64);
    local_out2    = (uint8_t *) snrt_l1_alloc_cluster_local(tot_size/2, 64);
    local_gold2   = (uint8_t *) snrt_l1_alloc_cluster_local(tot_size/2, 64);
    local_out3    = (uint8_t *) snrt_l1_alloc_cluster_local(tot_size/4, 64);
    local_out3bis = (uint8_t *) snrt_l1_alloc_cluster_local(tot_size/4, 64);
    local_gold3   = (uint8_t *) snrt_l1_alloc_cluster_local(tot_size/4, 64);

    // DMA input and expected goldens into TCDM
    snrt_dma_start_1d(local_in,    golden_in,   tot_size);
    snrt_dma_start_1d(local_gold,  golden_out,  tot_size);
    snrt_dma_start_1d(local_gold2, golden_out2, tot_size/2);
    snrt_dma_start_1d(local_gold3, golden_out3, tot_size/4);
    snrt_dma_wait_all();
  }
  snrt_cluster_hw_barrier();

  if (snrt_cluster_core_idx() == 0) {
    datamover_init();

    int st = 0;
    int to = 0;

    // Job 1: 8b transpose, 64x64, stride 64
    datamover_status = datamover_transpose(local_in, local_out, size_m, size_n, DATAMOVER_TRANSP_1ELEM);

    // Job 2: 16b transpose, 32x32, stride 64
    datamover_status = datamover_transpose(local_out, local_out2, size_m/2, size_n, DATAMOVER_TRANSP_2ELEM);

    // Job 3: 32b transpose, 16x16, stride 64
    datamover_status = datamover_transpose(local_out2, local_out3bis, size_m/4, size_n, DATAMOVER_TRANSP_4ELEM);

    // Job 4: no transpose, 16x16, stride 64
    datamover_status = datamover_copy(local_out3bis, local_out3, (size_m/2) * size_n);

    // Wait for all jobs to complete
    to = 5000000; do { st = datamover_get_status(); } while (st != 0 && --to);
    if (to == 0) { printf("[DM-ERR] jobs stuck, status=0x%08x\n", st); }

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
      printf("[DM-OK] ======= DATAMOVER TEST PASSED =======\n");
    } else {
      printf("[DM-ERR] !!!!!!! DATAMOVER TEST FAILED !!!!!!!\n");
      printf("[DM-ERR] mismatches: %d\n", errors);
    }
    *(volatile uint32_t *)0x80000000 = (uint32_t)errors;
    *(volatile uint32_t *)0x80000004 = 1u; // done
  }

  return errors;
}
