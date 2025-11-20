// Copyright 2023 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0
//
// Authors: Francesco Conti <f.conti@unibo.it>
//          Cyrill Durrer <cdurrer@iis.ee.ethz.ch>

#pragma once

#define DATAMOVER_ARCHI_CL_EVT_ACC0 0
#define DATAMOVER_ARCHI_CL_EVT_ACC1 1

#define DATAMOVER_BANDWIDTH 512
#define DATAMOVER_WORD_WIDTH 64
#define DATAMOVER_ELEM_WIDTH 8

#define DATAMOVER_BANDWIDTH_ELEMS   (DATAMOVER_BANDWIDTH / DATAMOVER_ELEM_WIDTH)
#define DATAMOVER_WORD_ELEMS        (DATAMOVER_WORD_WIDTH / DATAMOVER_ELEM_WIDTH)

// Base address
#include <stdint.h>
#include "snitch_cluster_addrmap.h"
// Map Datamover control into the cluster-visible narrow address space:
#define DATAMOVER_BASE_ADD ((uintptr_t)(&snitch_cluster_addrmap.cluster.zeromem) + sizeof(snitch_cluster__zeromem_t) + 0x100)

// Commands
#define DATAMOVER_TRIGGER           0x00
#define DATAMOVER_ACQUIRE           0x04
#define DATAMOVER_FINISHED          0x08
#define DATAMOVER_STATUS            0x0C
#define DATAMOVER_RUNNING_JOB       0x10
#define DATAMOVER_SOFT_CLEAR        0x14
#define DATAMOVER_SWSYNC            0x18
#define DATAMOVER_URISCY_IMEM       0x1C

#define DATAMOVER_EVT_OFFS          0x94
#define DATAMOVER_MUX_SEL_OFFS      0x98
#define DATAMOVER_CK_GATE_OFFS      0x9C

// Registers
#define DATAMOVER_REG_OFFS          0x40
#define DATAMOVER_REG_CXT0_OFFS     0x80
#define DATAMOVER_REG_CXT1_OFFS     0x120

#define DATAMOVER_REG_IN_PTR        0x00
#define DATAMOVER_REG_OUT_PTR       0x04
#define DATAMOVER_REG_LEN0          0x08
#define DATAMOVER_REG_LEN1          0x0C
#define DATAMOVER_REG_IN_D0_STRIDE  0x10
#define DATAMOVER_REG_IN_D1_STRIDE  0x14
#define DATAMOVER_REG_IN_D2_STRIDE  0x18
#define DATAMOVER_REG_OUT_D0_STRIDE 0x1C
#define DATAMOVER_REG_OUT_D1_STRIDE 0x20
#define DATAMOVER_REG_OUT_D2_STRIDE 0x24
#define DATAMOVER_REG_TRANSP_MODE   0x28       // Transposition mode (LSB: 000=none, 001=8b, 010=16b, 100=32b) + Leftover (MSB 31:16)

// Transposition formats
#define DATAMOVER_TRANSP_NONE       0x0
#define DATAMOVER_TRANSP_1ELEM      0x1
#define DATAMOVER_TRANSP_2ELEM      0x2
#define DATAMOVER_TRANSP_4ELEM      0x4

// FP Formats encoding      ToDo(cdurrer): verify?
#define DATAMOVER_FP16              0x2
#define DATAMOVER_FP8               0x3
#define DATAMOVER_FP16ALT           0x4
#define DATAMOVER_FP8ALT            0x5
