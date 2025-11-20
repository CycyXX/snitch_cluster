// Copyright 2023 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0
//
// Authors: Cyrill Durrer <cdurrer@iis.ee.ethz.ch>
//          Daniel Keller <dankeller@student.ethz.ch>


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

void datamover_init() {
  datamover_cg_enable();
  datamover_mux_enable();
  datamover_soft_clear();
}

datamover_status_t datamover_copy(uint8_t *src, uint8_t *dst, uint32_t nof_elements) {
  uint32_t src_off = tcdm_offset(src);
  uint32_t dst_off = tcdm_offset(dst);
  int acq_to = 1000000;
  int job_id = -1;

  while ((job_id = datamover_acquire_job()) < 0 && --acq_to) {}
  if (acq_to == 0) {
    printf("[DM-ERR] acquire timeout (job0)\n");
    return DM_ERR;
  }

  datamover_in_set(src_off);
  datamover_out_set(dst_off);
  datamover_len0_set((((nof_elements/DATAMOVER_BANDWIDTH_ELEMS) & 0xFFF) << 12) | (nof_elements & 0xFFF)); // tot_len
  datamover_len1_set((nof_elements/DATAMOVER_BANDWIDTH_ELEMS) & 0xFFF); // out_d0_len
  datamover_in_d0_stride_set(DATAMOVER_BANDWIDTH_ELEMS);
  datamover_out_d0_stride_set(DATAMOVER_BANDWIDTH_ELEMS);
  datamover_in_d1_stride_set(0);
  datamover_out_d1_stride_set(0);
  datamover_in_d2_stride_set(0);
  datamover_out_d2_stride_set(0);
  datamover_transp_mode_set(DATAMOVER_TRANSP_NONE);
  datamover_trigger_job();
  printf("[DM-CFG] Copying %i elements (src=0x%08x, dst=0x%08x)\n", nof_elements, src_off, dst_off);
  return DM_OK;
}

datamover_status_t datamover_transpose(uint8_t *matrix_in, uint8_t *matrix_out, uint32_t size_m, uint32_t size_n, uint8_t transp_mode) {
  uint32_t src_off = tcdm_offset(matrix_in);
  uint32_t dst_off = tcdm_offset(matrix_out);
  int acq_to = 1000000;
  int job_id = -1;

  while ((job_id = datamover_acquire_job()) < 0 && --acq_to) {}
  if (acq_to == 0) {
    printf("[DM-ERR] acquire timeout (job1)\n");
    return DM_ERR;
  }

  datamover_in_set(src_off);
  datamover_out_set(dst_off);
  datamover_len0_set((((size_n/DATAMOVER_BANDWIDTH_ELEMS) & 0x0FF) << 24) | ((size_m & 0xFFF) << 12) | (((size_m * size_n) / DATAMOVER_BANDWIDTH_ELEMS) & 0xFFF)); // in_d1_len[7:0] | in_d0_len | tot_len
  datamover_len1_set((((size_n/DATAMOVER_BANDWIDTH_ELEMS) & 0xF00) << (24-8)) | ((((size_m*transp_mode)/DATAMOVER_BANDWIDTH_ELEMS) & 0xFFF) << 12) | (DATAMOVER_BANDWIDTH_ELEMS & 0xFFF)); // in_d1_len[11:8] | out_d1_len | out_d0_len
  datamover_in_d0_stride_set(size_n);
  datamover_out_d0_stride_set(size_m * transp_mode);
  datamover_in_d1_stride_set(DATAMOVER_BANDWIDTH_ELEMS);
  datamover_out_d1_stride_set(DATAMOVER_BANDWIDTH_ELEMS);
  datamover_in_d2_stride_set(0);
  datamover_out_d2_stride_set(size_m * DATAMOVER_BANDWIDTH_ELEMS);
  datamover_transp_mode_set(transp_mode);
  datamover_trigger_job();
  printf("[DM-CFG] Transposing matrix: %ix%i, mode: %i elements (src=0x%08x, dst=0x%08x)\n", size_m, size_n, transp_mode, src_off, dst_off);
  return DM_OK;
}

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
