// Copyright 2025 ETH Zurich and University of Bologna.
// Solderpad Hardware License, Version 0.51, see LICENSE for details.
// SPDX-License-Identifier: SHL-0.51
//
// Author: Cyrill Durrer <cdurrer@ee.ethz.ch>

`include "axi/assign.svh"
`include "axi/typedef.svh"
`include "tcdm_interface/typedef.svh"


module konark_top
  import snitch_cluster_pkg::*;
(
  input  logic                                   clk_i,
  input  logic                                   rst_ni,
  input  logic [snitch_cluster_pkg::NrCores-1:0] debug_req_i,
  input  logic [snitch_cluster_pkg::NrCores-1:0] meip_i,
  input  logic [snitch_cluster_pkg::NrCores-1:0] mtip_i,
  input  logic [snitch_cluster_pkg::NrCores-1:0] msip_i,
  input  logic [snitch_cluster_pkg::NrCores-1:0] mxip_i,
  input  logic [9:0]                             hart_base_id_i,
  input  logic [47:0]                            cluster_base_addr_i,
  input  logic                                   clk_d2_bypass_i,         // unused
  input  snitch_cluster_pkg::sram_cfgs_t         sram_cfgs_i,             // unused
  input  snitch_cluster_pkg::narrow_in_req_t     narrow_in_req_i,
  output snitch_cluster_pkg::narrow_in_resp_t    narrow_in_resp_o,
  output snitch_cluster_pkg::narrow_out_req_t    narrow_out_req_o,
  input  snitch_cluster_pkg::narrow_out_resp_t   narrow_out_resp_i,
  output snitch_cluster_pkg::wide_out_req_t      wide_out_req_o,
  input  snitch_cluster_pkg::wide_out_resp_t     wide_out_resp_i,
  input  snitch_cluster_pkg::wide_in_req_t       wide_in_req_i,
  output snitch_cluster_pkg::wide_in_resp_t      wide_in_resp_o
);

  ////////////////////
  // Snitch Cluster //
  ////////////////////

  snitch_cluster_pkg::narrow_in_req_t   cluster_narrow_in_req;
  snitch_cluster_pkg::narrow_in_resp_t  cluster_narrow_in_rsp;
  snitch_cluster_pkg::narrow_out_req_t  cluster_narrow_out_req;
  snitch_cluster_pkg::narrow_out_resp_t cluster_narrow_out_rsp;
  snitch_cluster_pkg::wide_out_req_t    cluster_wide_out_req;
  snitch_cluster_pkg::wide_out_resp_t   cluster_wide_out_rsp;
  snitch_cluster_pkg::wide_in_req_t     cluster_wide_in_req;
  snitch_cluster_pkg::wide_in_resp_t    cluster_wide_in_rsp;

  snitch_cluster_pkg::narrow_out_req_t  cluster_narrow_ext_req;
  snitch_cluster_pkg::narrow_out_resp_t cluster_narrow_ext_rsp;
  snitch_cluster_pkg::tcdm_dma_req_t    cluster_tcdm_ext_req_aligned;
  snitch_cluster_pkg::tcdm_dma_req_t    cluster_tcdm_ext_req_misaligned;
  snitch_cluster_pkg::tcdm_dma_rsp_t    cluster_tcdm_ext_rsp_aligned;
  snitch_cluster_pkg::tcdm_dma_rsp_t    cluster_tcdm_ext_rsp_misaligned;

  localparam int unsigned HWPECtrlAddrWidth = 32;
  localparam int unsigned HWPECtrlDataWidth = 32;
  typedef logic [HWPECtrlAddrWidth-1:0] addr_hwpe_ctrl_t;
  typedef logic [HWPECtrlDataWidth-1:0] data_hwpe_ctrl_t;
  typedef logic [3:0] strb_hwpe_ctrl_t;

  `AXI_TYPEDEF_ALL(cluster_narrow_out_dw_conv, snitch_cluster_pkg::addr_t,
                   snitch_cluster_pkg::narrow_out_id_t, data_hwpe_ctrl_t, strb_hwpe_ctrl_t,
                   snitch_cluster_pkg::user_t)

  cluster_narrow_out_dw_conv_req_t cluster_narrow_out_dw_conv_req, cluster_narrow_out_cut_req;
  cluster_narrow_out_dw_conv_resp_t cluster_narrow_out_dw_conv_rsp, cluster_narrow_out_cut_rsp;

  `TCDM_TYPEDEF_ALL(hwpectrl, addr_hwpe_ctrl_t, data_hwpe_ctrl_t, strb_hwpe_ctrl_t, logic)

  hwpectrl_req_t               hwpectrl_req;
  hwpectrl_rsp_t               hwpectrl_rsp;

  logic          [NrCores-1:0] mxip;

  snitch_cluster_wrapper i_cluster (
    .clk_i,
    .rst_ni,
    .debug_req_i,
    .meip_i,
    .mtip_i,
    .msip_i,
    .hart_base_id_i,
    .cluster_base_addr_i,
    .mxip_i           (mxip),
    .clk_d2_bypass_i  ('0),
    .sram_cfgs_i      ('0),
    .narrow_in_req_i,
    .narrow_in_resp_o,
    .narrow_out_req_o,
    .narrow_out_resp_i,
    .wide_out_req_o,
    .wide_out_resp_i,
    .wide_in_req_i,
    .wide_in_resp_o,
    .narrow_ext_req_o (cluster_narrow_ext_req),
    .narrow_ext_resp_i(cluster_narrow_ext_rsp),
    .tcdm_ext_req_i   (cluster_tcdm_ext_req_aligned),
    .tcdm_ext_resp_o  (cluster_tcdm_ext_rsp_aligned)
  );

  // Convert narrow AXI's 64 bit DW down to 32
  axi_dw_converter #(
    .AxiMaxReads        (1),
    .AxiSlvPortDataWidth(snitch_cluster_pkg::NarrowDataWidth),
    .AxiMstPortDataWidth(HWPECtrlDataWidth),
    .AxiAddrWidth       (snitch_cluster_pkg::AddrWidth),
    .AxiIdWidth         (snitch_cluster_pkg::NarrowIdWidthOut),
    .aw_chan_t          (snitch_cluster_pkg::narrow_out_aw_chan_t),
    .mst_w_chan_t       (cluster_narrow_out_dw_conv_w_chan_t),
    .slv_w_chan_t       (snitch_cluster_pkg::narrow_out_w_chan_t),
    .b_chan_t           (snitch_cluster_pkg::narrow_out_b_chan_t),
    .ar_chan_t          (snitch_cluster_pkg::narrow_out_ar_chan_t),
    .mst_r_chan_t       (cluster_narrow_out_dw_conv_r_chan_t),
    .slv_r_chan_t       (snitch_cluster_pkg::narrow_out_r_chan_t),
    .axi_mst_req_t      (cluster_narrow_out_dw_conv_req_t),
    .axi_mst_resp_t     (cluster_narrow_out_dw_conv_resp_t),
    .axi_slv_req_t      (snitch_cluster_pkg::narrow_out_req_t),
    .axi_slv_resp_t     (snitch_cluster_pkg::narrow_out_resp_t)
  ) i_axi_dw_hwpe (
    .clk_i     (clk_i),
    .rst_ni    (rst_ni),
    .slv_req_i (cluster_narrow_ext_req),
    .slv_resp_o(cluster_narrow_ext_rsp),
    .mst_req_o (cluster_narrow_out_dw_conv_req),
    .mst_resp_i(cluster_narrow_out_dw_conv_rsp)
  );

  axi_cut #(
    .Bypass    (0),
    .aw_chan_t (snitch_cluster_pkg::narrow_out_aw_chan_t),
    .w_chan_t  (cluster_narrow_out_dw_conv_w_chan_t),
    .b_chan_t  (snitch_cluster_pkg::narrow_out_b_chan_t),
    .ar_chan_t (snitch_cluster_pkg::narrow_out_ar_chan_t),
    .r_chan_t  (cluster_narrow_out_dw_conv_r_chan_t),
    .axi_req_t (cluster_narrow_out_dw_conv_req_t),
    .axi_resp_t(cluster_narrow_out_dw_conv_resp_t)
  ) i_cut_ext_narrow_slv (
    .clk_i     (clk_i),
    .rst_ni    (rst_ni),
    .slv_req_i (cluster_narrow_out_dw_conv_req),
    .slv_resp_o(cluster_narrow_out_dw_conv_rsp),
    .mst_req_o (cluster_narrow_out_cut_req),
    .mst_resp_i(cluster_narrow_out_cut_rsp)
  );

  axi_to_tcdm #(
    .axi_req_t (cluster_narrow_out_dw_conv_req_t),
    .axi_rsp_t (cluster_narrow_out_dw_conv_resp_t),
    .tcdm_req_t(hwpectrl_req_t),
    .tcdm_rsp_t(hwpectrl_rsp_t),
    .IdWidth   (snitch_cluster_pkg::NarrowIdWidthOut),
    .AddrWidth (HWPECtrlAddrWidth),
    .DataWidth (HWPECtrlDataWidth)
  ) i_axi_to_hwpe_ctrl (
    .clk_i     (clk_i),
    .rst_ni    (rst_ni),
    .axi_req_i (cluster_narrow_out_cut_req),
    .axi_rsp_o (cluster_narrow_out_cut_rsp),
    .tcdm_req_o(hwpectrl_req),
    .tcdm_rsp_i(hwpectrl_rsp)
  );

  snitch_tcdm_aligner #(
    .tcdm_req_t   (snitch_cluster_pkg::tcdm_dma_req_t),
    .tcdm_rsp_t   (snitch_cluster_pkg::tcdm_dma_rsp_t),
    .DataWidth    (snitch_cluster_pkg::WideDataWidth),
    .TCDMDataWidth(snitch_cluster_pkg::NarrowDataWidth),
    .AddrWidth    (snitch_cluster_pkg::TcdmAddrWidth)
  ) i_snitch_tcdm_aligner (
    .clk_i                (clk_i),
    .rst_ni               (rst_ni),
    .tcdm_req_misaligned_i(cluster_tcdm_ext_req_misaligned),
    .tcdm_req_aligned_o   (cluster_tcdm_ext_req_aligned),
    .tcdm_rsp_aligned_i   (cluster_tcdm_ext_rsp_aligned),
    .tcdm_rsp_misaligned_o(cluster_tcdm_ext_rsp_misaligned)
  );

  snitch_hwpe_subsystem #(
    .tcdm_req_t   (snitch_cluster_pkg::tcdm_dma_req_t),
    .tcdm_rsp_t   (snitch_cluster_pkg::tcdm_dma_rsp_t),
    .periph_req_t (hwpectrl_req_t),
    .periph_rsp_t (hwpectrl_rsp_t),
    //.HwpeDataWidth(288), // ToDo(cdurrer): leave hardcoded?
    .HwpeDataWidth(snitch_cluster_pkg::WideDataWidth), 
    //.IdWidth      (1), // ToDo(cdurrer)
    .IdWidth (snitch_cluster_pkg::NarrowIdWidthOut),
    //.NrCores      (2), // ToDo(cdurrer): count DMA core?
    .NrCores      (snitch_cluster_pkg::NrCores),
    .NrContext    (2),
    .PE_H         (6),
    .PE_W         (6),
    .TCDMDataWidth(snitch_cluster_pkg::NarrowDataWidth)
  ) i_snitch_hwpe_subsystem (
    .clk_i          (clk_i),
    .rst_ni         (rst_ni),
    .test_mode_i    (1'b0),
    .tcdm_req_o     (cluster_tcdm_ext_req_misaligned),
    .tcdm_rsp_i     (cluster_tcdm_ext_rsp_misaligned),
    .hwpe_ctrl_req_i(hwpectrl_req),
    .hwpe_ctrl_rsp_o(hwpectrl_rsp),
    .hwpe_evt_o     (mxip)
  );

endmodule
