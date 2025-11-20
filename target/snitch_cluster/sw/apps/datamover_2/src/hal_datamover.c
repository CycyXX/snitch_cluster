/*
 * Cyrill Durrer <cdurrer@iis.ee.ethz.ch>
 *
 * Copyright 2025 ETH Zurich and University of Bologna
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

 #include "hal_datamover.h"
 #include "archi_datamover.h"
//  #include "snrt.h"
//  #include <stdio.h>
 #include "printf.h"

void datamover_in_set(unsigned int value) {
  DATAMOVER_WRITE(value, DATAMOVER_REG_OFFS + DATAMOVER_REG_IN_PTR);
}

void datamover_out_set(unsigned int value) {
  DATAMOVER_WRITE(value, DATAMOVER_REG_OFFS + DATAMOVER_REG_OUT_PTR);
}

void datamover_len0_set(unsigned int value) {
  DATAMOVER_WRITE(value, DATAMOVER_REG_OFFS + DATAMOVER_REG_LEN0);
}

void datamover_len1_set(unsigned int value) {
  DATAMOVER_WRITE(value, DATAMOVER_REG_OFFS + DATAMOVER_REG_LEN1);
}

void datamover_in_d0_stride_set(unsigned int value) {
  DATAMOVER_WRITE(value, DATAMOVER_REG_OFFS + DATAMOVER_REG_IN_D0_STRIDE);
}

void datamover_in_d1_stride_set(unsigned int value) {
  DATAMOVER_WRITE(value, DATAMOVER_REG_OFFS + DATAMOVER_REG_IN_D1_STRIDE);
}

void datamover_in_d2_stride_set(unsigned int value) {
  DATAMOVER_WRITE(value, DATAMOVER_REG_OFFS + DATAMOVER_REG_IN_D2_STRIDE);
}

void datamover_out_d0_stride_set(unsigned int value) {
  DATAMOVER_WRITE(value, DATAMOVER_REG_OFFS + DATAMOVER_REG_OUT_D0_STRIDE);
}

void datamover_out_d1_stride_set(unsigned int value) {
  DATAMOVER_WRITE(value, DATAMOVER_REG_OFFS + DATAMOVER_REG_OUT_D1_STRIDE);
}

void datamover_out_d2_stride_set(unsigned int value) {
  DATAMOVER_WRITE(value, DATAMOVER_REG_OFFS + DATAMOVER_REG_OUT_D2_STRIDE);
}

void datamover_transp_mode_set(unsigned int value) {
  DATAMOVER_WRITE(value, DATAMOVER_REG_OFFS + DATAMOVER_REG_TRANSP_MODE);
}

void datamover_trigger_job() { DATAMOVER_WRITE(0, DATAMOVER_TRIGGER); }

int datamover_acquire_job() { return DATAMOVER_READ(DATAMOVER_ACQUIRE); }

unsigned int datamover_get_status() { return DATAMOVER_READ(DATAMOVER_STATUS); }
unsigned int datamover_get_running_job() { return DATAMOVER_READ(DATAMOVER_RUNNING_JOB); }

void datamover_soft_clear() {
  volatile int i;
  DATAMOVER_WRITE(0, DATAMOVER_SOFT_CLEAR);
}

void datamover_evt_clear(int value) {
  DATAMOVER_WRITE(value, DATAMOVER_EVT_OFFS);
}

void datamover_cg_enable() { DATAMOVER_WRITE(2, DATAMOVER_CK_GATE_OFFS); }

void datamover_cg_disable() { DATAMOVER_WRITE(0, DATAMOVER_CK_GATE_OFFS); }

void datamover_mux_enable() { DATAMOVER_WRITE(1, DATAMOVER_MUX_SEL_OFFS); }

// ToDo(cdurrer): pass cluster address map to decouple HAL from system
static inline uint32_t tcdm_offset(void *ptr) {
  return (uint32_t)((uintptr_t)ptr - (uintptr_t)&snitch_cluster_addrmap.cluster);
}

void datamover_init() {
	printf("[DM-INFO] Initializing datamover\n");
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

datamover_status_t datamover_transpose(uint8_t *matrix_in, uint8_t *matrix_out, uint32_t size_m, uint32_t size_n, datamover_transp_mode_t transp_mode) {
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
