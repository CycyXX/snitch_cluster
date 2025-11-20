// Copyright 2023 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0
//
// Authors: Francesco Conti <f.conti@unibo.it>
//          Cyrill Durrer <cdurrer@iis.ee.ethz.ch>

#pragma once

#include "archi_datamover.h"
// #include "snitch_cluster_addrmap.h"   // ToDo(cdurrer): remove dependency to address map by passing it to init function

#define DATAMOVER_ADDR_BASE DATAMOVER_BASE_ADD

#define DATAMOVER_WRITE(value, offset) *(volatile int *)(DATAMOVER_ADDR_BASE + offset) = value
#define DATAMOVER_READ(offset) *(volatile int *)(DATAMOVER_ADDR_BASE + offset)

typedef enum {
    DM_OK = 0,
    DM_ERR
} datamover_status_t;

// Function declarations
void datamover_in_set(unsigned int value);
void datamover_out_set(unsigned int value);
void datamover_len0_set(unsigned int value);
void datamover_len1_set(unsigned int value);
void datamover_in_d0_stride_set(unsigned int value);
void datamover_in_d1_stride_set(unsigned int value);
void datamover_in_d2_stride_set(unsigned int value);
void datamover_out_d0_stride_set(unsigned int value);
void datamover_out_d1_stride_set(unsigned int value);
void datamover_out_d2_stride_set(unsigned int value);
void datamover_transp_mode_set(unsigned int value);
void datamover_trigger_job();
int datamover_acquire_job();
unsigned int datamover_get_status();
unsigned int datamover_get_running_job();
void datamover_soft_clear();
void datamover_evt_clear(int value);
void datamover_cg_enable();
void datamover_cg_disable();
void datamover_mux_enable();

void datamover_init();    // ToDo(cdurrer): pass cluster address map?
datamover_status_t datamover_copy(uint8_t *src, uint8_t *dst, uint32_t nof_elements);
datamover_status_t datamover_transpose(uint8_t *matrix_in, uint8_t *matrix_out, uint32_t size_m, uint32_t size_n, datamover_transp_mode_t transp_mode);
