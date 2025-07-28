// Author: Cyrill Durrer <cdurrer@iis.ee.ethz.ch>

#include "snrt.h"
#include "data.h"

#include "printf.h"

// Define your kernel
void axpy(uint32_t l, double a, double *x, double *y, double *z) {
    int core_idx = snrt_cluster_core_idx();
    int offset = core_idx * l;

    for (int i = 0; i < l; i++) {
        z[offset] = a * x[offset] + y[offset];
        printf("[core_idx=%u] axpy: z[%d] = %f\n", core_idx, offset, z[offset]);
        offset++;
    }
    snrt_fpu_fence();
}

void print_core_info() {
    uint32_t hartid = snrt_hartid();
    uint32_t cluster_idx = snrt_cluster_idx();
    uint32_t core_idx = snrt_cluster_core_idx();
    uint32_t global_core_idx = snrt_global_core_idx();
    uint32_t num_clusters = snrt_cluster_num();
    uint32_t cores_per_cluster = snrt_cluster_core_num();
    uint32_t total_cores = snrt_global_core_num();
    uint32_t is_compute = snrt_is_compute_core();
    uint32_t is_dm = snrt_is_dm_core();

    printf("[core_idx=%u] Core Info:\n", core_idx);
    printf("[core_idx=%u]   Hart ID                : %u\n", core_idx, hartid);
    printf("[core_idx=%u]   Cluster Index          : %u\n", core_idx, cluster_idx);
    printf("[core_idx=%u]   Core Index in Cluster  : %u\n", core_idx, core_idx);
    printf("[core_idx=%u]   Global Core Index      : %u\n", core_idx, global_core_idx);
    printf("[core_idx=%u]   Number of Clusters     : %u\n", core_idx, num_clusters);
    printf("[core_idx=%u]   Cores per Cluster      : %u\n", core_idx, cores_per_cluster);
    printf("[core_idx=%u]   Total Cores in System  : %u\n", core_idx, total_cores);
    printf("[core_idx=%u]   Core Type              : %s\n", core_idx, is_compute ? "Compute Core" : "Data Mover (DM) Core");
}

int main() {
    // Read the mcycle CSR (this is our way to mark/delimit a specific code region for benchmarking)
    uint32_t start_cycle = snrt_mcycle();

    // DM core does not participate in the computation
    if(snrt_is_compute_core()) {
        axpy(L / snrt_cluster_compute_core_num(), a, x, y, z);
        print_core_info();
        printf("cdurrer_test: compute_core: computation done\n");
    }
    if(snrt_is_dm_core()) {
        print_core_info();
        printf("cdurrer_test: dm_core\n");
    }

    printf("cdurrer_test: all cores: test ended!\n");

    // Read the mcycle CSR
    uint32_t end_cycle = snrt_mcycle();
}
