// Copyright 2023 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0
//
// Francesco Conti <f.conti@unibo.it>
//

#include <inttypes.h>

#pragma once

#define ERR 0x0011

static inline int datamover_compare_int(
  uint64_t *actual,
  uint64_t *golden,
  int len
) {
  int errors = 0;
  for (int i=0; i<len; i++) {
    uint64_t actual_ = *(actual+i);
    uint64_t golden_ = *(golden+i);
    // printf("  0x%" PRIx64 " <- 0x%" PRIx64 " @ %p (%d)\n", golden_, actual_, (void *)(actual+i), i);
    // printf("  0x%016x <- 0x%016x @ 0x%08x (%d)\n", golden_, actual_, (actual+i), i);
    if (actual_ != golden_) {
      // printf("ERROR! 0x%" PRIx64 " != 0x%" PRIx64 " @ %p (%d)\n", actual_, golden_, (void *)(actual+i), i);
      errors ++;
    }
    #ifdef VERBOSE
    printf("  0x%" PRIx64 " <- 0x%" PRIx64 " @ %p (%d)\n", golden_, actual_, (void *)(actual+i), i);
    #endif
  }
  return errors;
}
