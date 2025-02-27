// benchmark.c
#include "kernel/types.h"
#include "user/user.h"

void print_divider() {
    printf("----------------------------------------\n");
}

void run_test(char *name) {
    print_divider();
    printf("Running %s...\n", name);
    
    if (fork() == 0) {
        char *args[] = {name, 0};
        exec(name, args);
        printf("Exec of %s failed\n", name);
        exit(1);
    }
    wait(0);
}

int main() {
    struct memstat baseline;
    
    getmemstat(&baseline);
    
    printf("XV6 DEMAND PAGING & COW BENCHMARKS\n");
    print_divider();
    
    printf("Initial system state:\n");
    printf("  Allocated pages: %ld\n", baseline.total_allocated_pages);
    printf("  Total allocations: %ld\n", baseline.total_allocations);

    // Basic tests
    run_test("dpcow_basicmem");
    run_test("dpcow_cowtest");
    
    // Advanced demand paging test
    run_test("dpcow_dpeff");
    
    // Advanced COW test
    run_test("dpcow_encow");
    
    // Process tree tests
    run_test("dpcow_forkch");
    run_test("dpcow_exec");
    
    // Realistic workload simulation
    run_test("dpcow_irl");
    
    print_divider();
    
    // Get final system state
    struct memstat final;
    getmemstat(&final);
    
    printf("COMPREHENSIVE BENCHMARK SUMMARY\n");
    print_divider();
    
    // Demand Paging metrics
    printf("DEMAND PAGING METRICS:\n");
    printf("  Total demand page faults: %ld\n", final.demand_page_faults - baseline.demand_page_faults);
    
    // Estimate virtual pages requested vs physically allocated
    // This is an approximation based on fault patterns
    uint64 dp_savings_estimate = (final.demand_page_faults - baseline.demand_page_faults) / 3;
    uint64 virtual_pages_requested = (final.total_allocations - baseline.total_allocations) + dp_savings_estimate;
    uint64 pages_physically_allocated = final.total_allocations - baseline.total_allocations;
    
    printf("  Est. virtual pages requested: %ld\n", virtual_pages_requested);
    printf("  Pages physically allocated: %ld\n", pages_physically_allocated);
    printf("  Est. memory saved by DP: ~%ld pages (%ld KB)\n", 
           dp_savings_estimate, dp_savings_estimate * 4);
    
    if (virtual_pages_requested > 0) {
        uint64 dp_efficiency = (100 * dp_savings_estimate) / virtual_pages_requested;
        printf("  Demand paging efficiency: %ld%%\n", dp_efficiency);
    }
    
    print_divider();
    
    // Copy-on-Write metrics
    printf("COPY-ON-WRITE METRICS:\n");
    printf("  COW page faults: %ld\n", final.cow_page_faults - baseline.cow_page_faults);
    printf("  Pages shared through COW: %ld\n", final.cow_pages_shared - baseline.cow_pages_shared);
    printf("  COW pages copied: %ld\n", final.cow_copies_made - baseline.cow_copies_made);
    
    uint64 cow_savings = (final.cow_pages_shared - baseline.cow_pages_shared) - 
                        (final.cow_copies_made - baseline.cow_copies_made);
    printf("  Memory saved by COW: %ld pages (%ld KB)\n", cow_savings, cow_savings * 4);
    
    if ((final.cow_pages_shared - baseline.cow_pages_shared) > 0) {
        uint64 cow_efficiency = (100 * cow_savings) / 
                               (final.cow_pages_shared - baseline.cow_pages_shared);
        printf("  COW efficiency: %ld%%\n", cow_efficiency);
    }
    
    print_divider();
    
    // Combined metrics and theoretical comparison
    printf("COMBINED METRICS AND MEMORY USAGE COMPARISON:\n");
    printf("  Peak memory usage: %ld pages\n", final.peak_allocated_pages);
    
    // Calculate theoretical memory usage without optimizations
    uint64 theoretical_pages_without_optimizations = 
        virtual_pages_requested + final.cow_pages_shared;
    
    printf("  Theoretical memory usage without optimizations: %ld pages (%ld KB)\n", 
           theoretical_pages_without_optimizations,
           theoretical_pages_without_optimizations * 4);
    
    printf("  Actual memory usage with optimizations: %ld pages (%ld KB)\n",
           final.peak_allocated_pages,
           final.peak_allocated_pages * 4);
           
    uint64 total_savings = theoretical_pages_without_optimizations - final.peak_allocated_pages;
    
    printf("  Total memory saved: %ld pages (%ld KB)\n", 
           total_savings,
           total_savings * 4);
    
    if (theoretical_pages_without_optimizations > 0) {
        uint64 overall_efficiency = (100 * total_savings) / theoretical_pages_without_optimizations;
        printf("  Overall memory efficiency: %ld%%\n", overall_efficiency);
        printf("  Memory reduction: %ld.%ld×\n", 
               theoretical_pages_without_optimizations / final.peak_allocated_pages,
               (10 * theoretical_pages_without_optimizations / final.peak_allocated_pages) % 10);
    }
    
    exit(0);
}
