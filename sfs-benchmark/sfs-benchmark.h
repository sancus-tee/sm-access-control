#ifndef SFS_BENCHMARK_H

extern struct SancusModule sfsBenchmarkSm;
extern struct SancusModule sfsBenchmarkHelperSm;

#ifdef RUN_FILES_BENCHMARK
    void SM_ENTRY("sfsBenchmarkSm") run_files_benchmark(void);
#endif

#ifdef RUN_ACL_BENCHMARK
    void SM_ENTRY("sfsBenchmarkSm") run_acl_benchmark(void);
#endif

#endif
