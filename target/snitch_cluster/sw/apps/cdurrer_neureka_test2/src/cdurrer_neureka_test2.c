/*
 * Copyright (C) 2020-2024 ETH Zurich and University of Bologna
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
 * Authors:  Francesco Conti <fconti@iis.ee.ethz.ch>
 *           Gianna Paulin <pauling@iis.ee.ethz.ch>
 *           Renzo Andri <andrire@iis.ee.ethz.ch>
 *           Arpan Suravi Prasad <prasadar@iis.ee.ethz.ch>
 *           Luka Macan <luka.macan@unibo.it>
 *
 * Adapted for Snitch cluster (Konark): Cyrill Durrer <cdurrer@iis.ee.ethz.ch>
 */

#include "snrt.h"

#include "printf.h"

// #include <stdint.h>
// #include <stdio.h>

#include "layer_util.h"
#include "nnx_layer.h"

#include "bias.h"
#include "input.h"
#include "layer_conf.h"
#include "scale.h"
#include "weight.h"
#include "output.h"

void dma_copy_to_tcdm(int32_t *local_bias) {
  if (snrt_is_dm_core()) {
    printf("<DMA> Copying data to TCDM...\n");
    size_t size = sizeof(int32_t) * BIAS_SIZE;
    snrt_dma_start_1d(local_bias, bias, size);
    snrt_dma_wait_all();
  }
}

int main() {
  int32_t *local_bias;

  local_bias = (int32_t *)snrt_l1_next();
  dma_copy_to_tcdm(local_bias);

  if (snrt_is_dm_core()) {
    printf("<DMA> local_bias [0x%p]: %d\n", local_bias, local_bias[0]);
  }

  // wait for DMA transfer to finish
  snrt_cluster_hw_barrier();

  if(snrt_is_compute_core()) {
    // execute NNX layer
    printf("<COMPUTE> Executing NNX layer...\n");
    execute_nnx_layer(NULL);

    // output checking
    int err = check_output();

    *(volatile int *) (0x80000000) = err;
    *(volatile int *) (0x80000004) = 1;
  }
  return 0;
}
