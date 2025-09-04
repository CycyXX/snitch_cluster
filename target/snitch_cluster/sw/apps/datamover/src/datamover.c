// Copyright 2023 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#include <stdint.h>

#include "snrt.h"
#include "hal_datamover.h"
#include "datamover_utils.h"

#ifndef SIZE
#define SIZE 64
#endif

uint8_t *local_in;
uint8_t *local_out;
uint8_t *local_out2;
uint8_t *local_out3bis;
uint8_t *local_out3;
uint8_t *local_gold;
uint8_t *local_gold2;
uint8_t *local_gold3;

int main() {

  if (snrt_cluster_idx() > 0) return 0;

  uint32_t errors = 0;
  int offload_id_tmp;

  uint32_t core_idx = snrt_global_core_idx();

  uint16_t in_size  = SIZE * SIZE * sizeof(uint8_t);
  uint16_t out_size = SIZE * SIZE * sizeof(uint8_t);

  // Allocate space in TCDM and copy inputs to TCDM
  if (snrt_is_dm_core()) {
    local_in    = (uint8_t *) snrt_l1_alloc_cluster_local(in_size, 64);
    local_out   = (uint8_t *) snrt_l1_alloc_cluster_local(out_size, 64);
    local_gold  = (uint8_t *) snrt_l1_alloc_cluster_local(out_size, 64);
    local_out2  = (uint8_t *) snrt_l1_alloc_cluster_local(in_size/2, 64);
    local_gold2 = (uint8_t *) snrt_l1_alloc_cluster_local(in_size/2, 64);
    local_out3  = (uint8_t *) snrt_l1_alloc_cluster_local(in_size/4, 64);
    local_out3bis = (uint8_t *) snrt_l1_alloc_cluster_local(in_size/4, 64);
    local_gold3 = (uint8_t *) snrt_l1_alloc_cluster_local(in_size/4, 64);
    // Initialize input with a simple pattern and precompute expected outputs
    for (int i = 0; i < SIZE; ++i) {
      for (int j = 0; j < SIZE; ++j) {
        local_in[i*SIZE + j] = (uint8_t)((i*SIZE + j) & 0xFF);
      }
    }
    // Compute golden 8-bit transpose (64x64)
    for (int i = 0; i < SIZE; ++i) {
      for (int j = 0; j < SIZE; ++j) {
        local_gold[j*SIZE + i] = local_in[i*SIZE + j];
      }
    }
    // Compute golden 16-bit transpose (32x32) from local_out of first pass (i.e., from local_gold)
    for (int i = 0; i < SIZE/2; ++i) {         // rows in 16b elements
      for (int j = 0; j < SIZE/2; ++j) {       // cols in 16b elements
        // indices in bytes with 64B stride
        int in_idx  = (i*64) + (j*2);
        int out_idx = (j*64) + (i*2);
        local_gold2[out_idx + 0] = local_gold[in_idx + 0];
        local_gold2[out_idx + 1] = local_gold[in_idx + 1];
      }
    }
    // Compute golden 32-bit transpose (16x16) from the result of second pass (i.e., from local_gold2)
    for (int i = 0; i < SIZE/4; ++i) {         // rows in 32b elements
      for (int j = 0; j < SIZE/4; ++j) {       // cols in 32b elements
        int in_idx  = (i*64) + (j*4);
        int out_idx = (j*64) + (i*4);
        local_gold3[out_idx + 0] = local_gold2[in_idx + 0];
        local_gold3[out_idx + 1] = local_gold2[in_idx + 1];
        local_gold3[out_idx + 2] = local_gold2[in_idx + 2];
        local_gold3[out_idx + 3] = local_gold2[in_idx + 3];
      }
    }
  }

  snrt_cluster_hw_barrier();

  if (core_idx == 0) {
    // Enable Datamover
    datamover_cg_enable();
    datamover_mux_enable();

    datamover_soft_clear();

    // First job: 8b transpose, 64x64 matrix
    while( ( offload_id_tmp = datamover_acquire_job() ) < 0);

    datamover_in_set((unsigned int) local_in);
    datamover_out_set((unsigned int) local_out);
    datamover_len0_set(
      ((64 & 0x00000fff) << 12) | // in_d0_len
      (64 & 0x00000fff)           // tot_len
    );
    datamover_len1_set(
      (64 & 0x00000fff)           // out_d0_len
    );
    datamover_in_d0_stride_set(64);
    datamover_out_d0_stride_set(64);
    datamover_transp_mode_set(DATAMOVER_TRANSP_8B);

    // Start Datamover operation
    datamover_trigger_job();

    // Second job: 16b transpose, 32x32 matrix
    while( ( offload_id_tmp = datamover_acquire_job() ) < 0);

    datamover_in_set((unsigned int) local_out);
    datamover_out_set((unsigned int) local_out2);
    datamover_len0_set(
      ((32 & 0x00000fff) << 12) | // in_d0_len
      (32 & 0x00000fff)           // tot_len
    );
    datamover_len1_set(
      (32 & 0x00000fff)           // out_d0_len
    );
    datamover_in_d0_stride_set(64);
    datamover_out_d0_stride_set(64);
    datamover_transp_mode_set(DATAMOVER_TRANSP_16B);

    // Start Datamover operation
    datamover_trigger_job();

    // Third job: 32b transpose, 16x16 matrix
    while( ( offload_id_tmp = datamover_acquire_job() ) < 0);

    datamover_in_set((unsigned int) local_out2);
    datamover_out_set((unsigned int) local_out3bis);
    datamover_len0_set(
      ((16 & 0x00000fff) << 12) | // in_d0_len
      (16 & 0x00000fff)           // tot_len
    );
    datamover_len1_set(
      (16 & 0x00000fff)           // out_d0_len
    );
    datamover_in_d0_stride_set(64);
    datamover_out_d0_stride_set(64);
    datamover_transp_mode_set(DATAMOVER_TRANSP_32B);

    // Start Datamover operation
    datamover_trigger_job();

    // Fourth job: no transpose, 16x16 matrix
    while( ( offload_id_tmp = datamover_acquire_job() ) < 0);

    datamover_in_set((unsigned int) local_out3bis);
    datamover_out_set((unsigned int) local_out3);
    datamover_len0_set(
      ((16 & 0x00000fff) << 12) | // in_d0_len
      (16 & 0x00000fff)           // tot_len
    );
    datamover_len1_set(
      (16 & 0x00000fff)           // out_d0_len
    );
    datamover_in_d0_stride_set(64);
    datamover_out_d0_stride_set(64);
    datamover_transp_mode_set(DATAMOVER_TRANSP_NONE);

    // Start Datamover operation
    datamover_trigger_job();

  }

  snrt_cluster_hw_barrier();

  if (core_idx == 0) {

    int status;
    snrt_interrupt_enable(IRQ_M_ACC);
    while ((status = datamover_get_status()) != 0) snrt_wfi();
    datamover_evt_clear(1 << core_idx);
    snrt_interrupt_disable(IRQ_M_ACC);

    // Disable Datamover
    datamover_cg_disable();

    // Check computation is correct
    errors  = datamover_compare_int((uint64_t*)local_out,  (uint64_t*) local_gold,  SIZE*SIZE/8);
    errors += datamover_compare_int((uint64_t*)local_out2, (uint64_t*) local_gold2, SIZE*SIZE/16);
    errors += datamover_compare_int((uint64_t*)local_out3, (uint64_t*) local_gold3, SIZE*SIZE/32);
  }

  return errors;
}
