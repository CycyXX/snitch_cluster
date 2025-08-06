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

// #include "snrt.h"

// #include "printf.h"

// #include <stdint.h>
// #include <stdio.h>

// #include "layer_util.h"
#include "nnx_layer.h"

// #include "bias.h"
// #include "input.h"
// #include "layer_conf.h"
// #include "scale.h"
// #include "weight.h"
#include "output.h"



int main() {
  execute_nnx_layer(NULL);
  return 0;
}
