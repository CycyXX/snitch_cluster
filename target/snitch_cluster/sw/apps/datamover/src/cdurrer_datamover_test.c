/*
 * Copyright (C) 2019-2020 ETH Zurich and University of Bologna
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
 */

/*
 * Authors:  Francesco Conti <f.conti@unibo.it>
 */


// Author (Konark version): Cyrill Durrer <cdurrer@iis.ee.ethz.ch>

#include "snrt.h"
#include "printf.h"

#include "pmsis.h"
#include "stdio.h"
#include <stdint.h>
#include "hal_datamover.h"
#include "snitch_hwpe_subsystem_addrmap.h"
#include "konark_addrmap.h"
#include "lfsr32.h"
#include "data.h"

#define DATAMOVER_BW (256 / 8)

static uint8_t *local_x;
static uint8_t *local_y;
static int ret_value;

static void dma_copy_to_tcdm(void *dest_addr, void *src_addr, size_t size) {
  if (snrt_is_dm_core()) {
    printf("<DMA> Copying data to TCDM...\n");
    snrt_dma_start_1d(dest_addr, src_addr, size);
    snrt_dma_wait_all();
  }
}


int main()
{
  printf("Entered cluster on cluster %d core %d (global core idx: %d)\n", snrt_cluster_idx(), snrt_cluster_core_idx(), snrt_global_core_idx());
  snrt_cluster_hw_barrier();

  int errors = 0;

  printf("Running data mover test...\n");

  local_x = (uint8_t *)snrt_l1_next();
  local_y = (uint8_t *)(local_x + DATA_SIZE);

  dma_copy_to_tcdm(local_x, &x, sizeof(uint8_t) * DATA_SIZE);
  dma_copy_to_tcdm(local_y, &y, sizeof(uint8_t) * DATA_SIZE);

  if (snrt_is_dm_core()) {
    printf("<DMA> x     [0x%p]: 0x%x\n", x, (unsigned int)x[0]);
    printf("<DMA> local_x     [0x%p]: 0x%x (signed dec: %d)\n", local_x, local_x[0], local_x[0]);
    printf("<DMA> local_y     [0x%p]: 0x%x (signed dec: %d)\n", local_y, local_y[0], local_y[0]);
  }

  // wait for DMA transfer to finish
  snrt_cluster_hw_barrier();

  if (snrt_cluster_core_idx() == 0) {
    // Enable clock only for Datamover (gate Neureka): clk_en = 2'b10
    volatile uint32_t *hwpe_clk_en = (uint32_t *)(KONARK_HWPE_SUBSYS_BASE_ADDR + SNITCH_HWPE_SUBSYSTEM_CLK_EN_REG_OFFSET);
    *hwpe_clk_en = 2; // bit0=Neureka, bit1=Datamover
    printf("[HWPE] clk_en set to 0x%x (expect 0x2)\n", *hwpe_clk_en);
    // Route HCI periph port to Datamover
    volatile uint32_t *hwpe_mux_sel = (uint32_t *)(KONARK_HWPE_SUBSYS_BASE_ADDR + SNITCH_HWPE_SUBSYSTEM_MUX_SEL_REG_OFFSET);
    *hwpe_mux_sel = 1; // 0=Neureka, 1=Datamover
    printf("[HWPE] mux_sel set to 0x%x (1=Datamover)\n", *hwpe_mux_sel);

    // Soft-clear Datamover and acquire a job
    DATAMOVER_WRITE_CMD(DATAMOVER_SOFT_CLEAR, DATAMOVER_SOFT_CLEAR_ALL);
    for (volatile int kk = 0; kk < 10; kk++) ;

    int job_id = -1;
    do {
      DATAMOVER_READ_CMD(job_id, DATAMOVER_ACQUIRE);
    } while (job_id < 0);

    // Program a flat 1D move (copy DATA_SIZE bytes from local_x to local_y)
    DATAMOVER_WRITE_REG(DATAMOVER_REG_IN_PTR,  (uint32_t)(uintptr_t)local_x);
    DATAMOVER_WRITE_REG(DATAMOVER_REG_OUT_PTR, (uint32_t)(uintptr_t)local_y);
    DATAMOVER_WRITE_REG(DATAMOVER_REG_TOT_LEN,       DATA_SIZE / DATAMOVER_BW);
    DATAMOVER_WRITE_REG(DATAMOVER_REG_IN_D0_LEN,     DATA_SIZE / DATAMOVER_BW);
    DATAMOVER_WRITE_REG(DATAMOVER_REG_IN_D0_STRIDE,  DATAMOVER_BW);
    DATAMOVER_WRITE_REG(DATAMOVER_REG_IN_D1_LEN,     1);
    DATAMOVER_WRITE_REG(DATAMOVER_REG_IN_D1_STRIDE,  0);
    DATAMOVER_WRITE_REG(DATAMOVER_REG_IN_D2_STRIDE,  0);
    DATAMOVER_WRITE_REG(DATAMOVER_REG_OUT_D0_LEN,    DATA_SIZE / DATAMOVER_BW);
    DATAMOVER_WRITE_REG(DATAMOVER_REG_OUT_D0_STRIDE, DATAMOVER_BW);
    DATAMOVER_WRITE_REG(DATAMOVER_REG_OUT_D1_LEN,    1);
    DATAMOVER_WRITE_REG(DATAMOVER_REG_OUT_D1_STRIDE, 0);
    DATAMOVER_WRITE_REG(DATAMOVER_REG_OUT_D2_STRIDE, 0);

    // Commit and trigger
    DATAMOVER_WRITE_CMD(DATAMOVER_COMMIT_AND_TRIGGER, DATAMOVER_TRIGGER_CMD);

    // Busy-wait for completion (poll STATUS)
    int status = 0;
    do {
      DATAMOVER_READ_CMD(status, DATAMOVER_STATUS);
    } while (status != 0);
  }

  snrt_cluster_hw_barrier();

  // Verify: local_y must match local_x (DATA_SIZE bytes)
  if (snrt_cluster_core_idx() == 0) {
    int mismatches = 0;
    for (int i = 0; i < DATA_SIZE; ++i) {
      if (local_y[i] != local_x[i]) {
        if (mismatches < 8) {
          printf("Mismatch @ %d: exp=0x%02x got=0x%02x\n", i, local_x[i], local_y[i]);
        }
        mismatches++;
      }
    }
    if (mismatches == 0) {
      printf("> Datamover copy OK (%d bytes).\n", DATA_SIZE);
    } else {
      printf("> Datamover copy FAILED: %d mismatches.\n", mismatches);
      errors += mismatches;
    }
  }

  return errors;
//   return pmsis_kickoff((void *)test_kickoff);
}

// static void pe_entry(void *arg) {

//   printf("Entered cluster on cluster %d core %d (global core idx: %d)\n", snrt_cluster_idx(), snrt_cluster_core_idx(), snrt_global_core_idx());

//   snrt_cluster_hw_barrier();

//   int errors = 0;

// //   if (snrt_cluster_core_idx() == 0) {

// //     uint8_t volatile *x = (uint8_t volatile *) pi_cl_l1_malloc(NULL, DATA_SIZE);
// //     uint8_t volatile *y = (uint8_t volatile *) pi_cl_l1_malloc(NULL, DATA_SIZE);
// //     generate_random_buffer((int) x, (int) x + DATA_SIZE, DEFAULT_SEED);

// //     // enable clock
// //     DATAMOVER_CG_ENABLE();

// //     // setup HCI
// //     DATAMOVER_SETPRIORITY_DATAMOVER(); // priority to DATAMOVER w.r.t. cores, DMA
// //     DATAMOVER_RESET_MAXSTALL();   // reset maximum stall
// //     DATAMOVER_SET_MAXSTALL(8);    // set maximum consecutive stall to 8 cycles for cores, DMA side

// //     // soft-clear DATAMOVER
// //     DATAMOVER_WRITE_CMD(DATAMOVER_SOFT_CLEAR, DATAMOVER_SOFT_CLEAR_ALL);
// //     for(volatile int kk=0; kk<10; kk++);

// //     // acquire job
// //     int job_id = -1;
// //     DATAMOVER_BARRIER_ACQUIRE(job_id);

// //     // set up datamover
// //     DATAMOVER_WRITE_REG(DATAMOVER_REG_IN_PTR,  x);
// //     DATAMOVER_WRITE_REG(DATAMOVER_REG_OUT_PTR, y);
// //     DATAMOVER_WRITE_REG(DATAMOVER_REG_TOT_LEN,       DATA_SIZE / DATAMOVER_BW);
// //     DATAMOVER_WRITE_REG(DATAMOVER_REG_IN_D0_LEN,     DATA_SIZE / DATAMOVER_BW);
// //     DATAMOVER_WRITE_REG(DATAMOVER_REG_IN_D0_STRIDE,  DATAMOVER_BW);
// //     DATAMOVER_WRITE_REG(DATAMOVER_REG_IN_D1_LEN,     DATA_SIZE / DATAMOVER_BW);
// //     DATAMOVER_WRITE_REG(DATAMOVER_REG_IN_D1_STRIDE,  0);
// //     DATAMOVER_WRITE_REG(DATAMOVER_REG_IN_D2_STRIDE,  0);
// //     DATAMOVER_WRITE_REG(DATAMOVER_REG_OUT_D0_LEN,    DATA_SIZE / DATAMOVER_BW);
// //     DATAMOVER_WRITE_REG(DATAMOVER_REG_OUT_D0_STRIDE, DATAMOVER_BW);
// //     DATAMOVER_WRITE_REG(DATAMOVER_REG_OUT_D1_LEN,    DATA_SIZE / DATAMOVER_BW);
// //     DATAMOVER_WRITE_REG(DATAMOVER_REG_OUT_D1_STRIDE, 0);
// //     DATAMOVER_WRITE_REG(DATAMOVER_REG_OUT_D2_STRIDE, 0);

// //     // commit and trigger datamover operation
// //     DATAMOVER_WRITE_CMD(DATAMOVER_COMMIT_AND_TRIGGER, DATAMOVER_TRIGGER_CMD);

// //     // wait for end of computation
// //     DATAMOVER_BARRIER();

// //     // disable clock
// //     DATAMOVER_CG_DISABLE();

// //     // set priority to core side
// //     DATAMOVER_SETPRIORITY_CORE();

// //     ret_value = check_random_buffer((int) y, (int) y + DATA_SIZE, DEFAULT_SEED);

// //   }
// //   pi_cl_team_barrier();
// // }

// // static void cluster_entry(void *arg) {
// //   pi_cl_team_fork(0, pe_entry, 0);
// // }

// // void test_kickoff(void *arg)
// // {
// //   struct pi_device cluster_dev;
// //   struct pi_cluster_conf conf;
// //   struct pi_cluster_task task;
// //   ret_value = 0;

// //   pi_cluster_conf_init(&conf);
// //   conf.id = 0;

// //   pi_open_from_conf(&cluster_dev, &conf);

// //   pi_cluster_open(&cluster_dev);

// //   pi_cluster_task(&task, cluster_entry, NULL);

// //   pi_cluster_send_task_to_cl(&cluster_dev, &task);

// //   pi_cluster_close(&cluster_dev);

// //   pmsis_exit(ret_value);
// }
