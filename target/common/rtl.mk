# Copyright 2025 ETH Zurich and University of Bologna.
# Solderpad Hardware License, Version 0.51, see LICENSE for details.
# SPDX-License-Identifier: SHL-0.51

# Directories
SN_BOOTROM_DIR ?= $(SN_TARGET_DIR)/test

# Templates
SN_CLUSTER_WRAPPER_TPL = $(SN_HW_DIR)/snitch_cluster/src/snitch_cluster_wrapper.sv.tpl
SN_CLUSTER_PKG_TPL     = $(SN_HW_DIR)/snitch_cluster/src/snitch_cluster_pkg.sv.tpl
SN_CLUSTER_RDL_TPL	   = $(SN_HW_DIR)/snitch_cluster/src/snitch_cluster.rdl.tpl

# Generated RTL sources
SN_CLUSTER_WRAPPER     = $(SN_GEN_DIR)/snitch_cluster_wrapper.sv
SN_CLUSTER_PKG         = $(SN_GEN_DIR)/snitch_cluster_pkg.sv
SN_CLUSTER_ADDRMAP_SVH = $(SN_GEN_DIR)/snitch_cluster_addrmap.svh
SN_HWPE_SUBSYS_ADDRMAP_SVH = $(SN_GEN_DIR)/snitch_hwpe_subsystem_addrmap.svh
SN_KONARK_ADDRMAP_SVH      = $(SN_GEN_DIR)/konark_addrmap.svh
SN_CLUSTER_PERIPH      = $(SN_PERIPH_DIR)/snitch_cluster_peripheral_reg.sv
SN_CLUSTER_PERIPH_PKG  = $(SN_PERIPH_DIR)/snitch_cluster_peripheral_reg_pkg.sv
SN_BOOTROM             = $(SN_BOOTROM_DIR)/snitch_bootrom.sv
SN_CLUSTER_RDL         = $(SN_GEN_DIR)/snitch_cluster.rdl

# All generated RTL sources
SN_GEN_RTL_SRCS = $(SN_CLUSTER_WRAPPER) $(SN_CLUSTER_PKG) $(SN_CLUSTER_ADDRMAP_SVH) $(SN_HWPE_SUBSYS_ADDRMAP_SVH) $(SN_KONARK_ADDRMAP_SVH) $(SN_CLUSTER_PERIPH) $(SN_CLUSTER_PERIPH_PKG) $(SN_BOOTROM)

# CLUSTERGEN rules
$(eval $(call sn_cluster_gen_rule,$(SN_CLUSTER_WRAPPER),$(SN_CLUSTER_WRAPPER_TPL)))
$(eval $(call sn_cluster_gen_rule,$(SN_CLUSTER_PKG),$(SN_CLUSTER_PKG_TPL)))
$(eval $(call sn_cluster_gen_rule,$(SN_CLUSTER_RDL),$(SN_CLUSTER_RDL_TPL)))

# peakRDL rules
$(SN_CLUSTER_PERIPH_PKG): $(SN_CLUSTER_PERIPH)
$(SN_CLUSTER_PERIPH): $(SN_PERIPH_DIR)/snitch_cluster_peripheral_reg.rdl
	@echo "[peakrdl] Generating $@"
	$(PEAKRDL) regblock $< -o $(SN_PERIPH_DIR) --cpuif apb4-flat --default-reset arst_n
$(SN_CLUSTER_ADDRMAP_SVH): $(SN_CLUSTER_RDL)
	@echo "[peakrdl] Generating $@"
	$(PEAKRDL) raw-header $< -o $@ --format svh -I $(SN_PERIPH_DIR)

# HWPE subsystem (Konark) SV header from SystemRDL (offset constants)
SN_HWPE_SUBSYS_RDL = $(SN_HW_DIR)/snitch_cluster/src/hwpe_subsystem/snitch_hwpe_subsystem.rdl
$(SN_HWPE_SUBSYS_ADDRMAP_SVH): $(SN_HWPE_SUBSYS_RDL)
	@echo "[peakrdl] Generating $@"
	$(PEAKRDL) raw-header $< -o $@ --format svh

# Konark top-level addrmap SV header
SN_KONARK_RDL = $(SN_HW_DIR)/snitch_cluster/src/konark.rdl
$(SN_KONARK_ADDRMAP_SVH): $(SN_KONARK_RDL) $(SN_CLUSTER_RDL) $(SN_HWPE_SUBSYS_RDL)
	@echo "[peakrdl] Generating $@"
	$(PEAKRDL) raw-header $< -o $@ --format svh -I $(SN_GEN_DIR) -I $(SN_HW_DIR)/snitch_cluster/src -I $(SN_PERIPH_DIR) -I $(SN_HW_DIR)/snitch_cluster/src/hwpe_subsystem

# Bootrom rules
$(SN_BOOTROM_DIR)/bootrom.elf $(SN_BOOTROM_DIR)/bootrom.dump $(SN_BOOTROM_DIR)/bootrom.bin $(SN_BOOTROM): $(SN_BOOTROM_DIR)/bootrom.S $(SN_BOOTROM_DIR)/bootrom.ld $(SN_BOOTROM_GEN) | $(SN_BOOTROM_DIR)
	$(RISCV_CC) -mabi=ilp32d -march=rv32imafd -static -nostartfiles -fuse-ld=$(RISCV_LD) -L$(SN_ROOT)/sw/runtime -T$(SN_BOOTROM_DIR)/bootrom.ld $< -o $(SN_BOOTROM_DIR)/bootrom.elf
	$(RISCV_OBJDUMP) -d $(SN_BOOTROM_DIR)/bootrom.elf > $(SN_BOOTROM_DIR)/bootrom.dump
	$(RISCV_OBJCOPY) -j .text -O binary $(SN_BOOTROM_DIR)/bootrom.elf $(SN_BOOTROM_DIR)/bootrom.bin
	$(SN_BOOTROM_GEN) --sv-module snitch_bootrom $(SN_BOOTROM_DIR)/bootrom.bin > $(SN_BOOTROM)

# General RTL targets
.PHONY: sn-rtl sn-clean-rtl

sn-rtl: $(SN_GEN_RTL_SRCS)

sn-clean-rtl:
	rm -f $(SN_GEN_RTL_SRCS)

$(SN_BOOTROM_DIR):
	mkdir -p $@
