# CSOT - Low Latency - Week 1 - Replay Engine

> Mission: Build a minimal C++ trading-platform runner that executes a judge-defined strategy spec, replays historical market ticks against it, verifies correctness, measures end-to-end latency per decision, and prints a summary.
  
## BUILD INSTRUCTIONS
```bash
cmake -B build
cmake --build build -j
```

## RUN INSTRUCIONS
- RUN Replay Engine with provided hist and bench
```bash
./build/quant_runner ./build/spec_strategy.so data/synthetic_large1.csv 
```
- Rum Replay Engine with google benchmark
```bash
./build/spec_strategy.so ./data/synthetic_large1.csv
```

## HARDWARE
- CPU: Intel Ultra 5 225H
- RAM: 32 GB
- OS: Fedora Linux 44 (Workstation Edition)
- Kernel: Linux 7.0.10-201.fc44.x86_64
  
## Latency Results
| Metric        |      Value |
| ------------- | ---------: |
| CPU Frequency |   1700 MHz |
| Count         | 10,000,000 |
| p50           |    ≤ 16 ns |
| p90           |    ≤ 32 ns |
| p99           |    ≤ 32 ns |
| p999          |   ≤ 128 ns |

## Performance Profiling 
```bash
perf stat -ddd ./build/quant_bench ./build/spec_strategy.so ./data/synthetic_large1.csv
```
Note : Use `taskset -c` to avoid core migration and more consistent results.  
  
| Event                   |          Count | Derived Metric / Rate | Measurement Coverage |
| ----------------------- | -------------: | --------------------- | -------------------- |
| task-clock              |    8,902.17 ms | 1.000 CPUs utilized   | -                    |
| context-switches        |             72 | 8.1 /sec              | -                    |
| cpu-migrations          |              0 | 0.0 /sec              | -                    |
| page-faults             |        313,956 | 35.267 K/sec          | -                    |
| instructions            | 56,542,310,960 | 3.68 insn per cycle   | (75.00%)             |
| cycles                  | 15,369,802,663 | 1.726 GHz             | (66.67%)             |
| branches                | 12,436,868,370 | 1.397 G/sec           | (58.34%)             |
| branch-misses           |     39,793,025 | 0.32% of all branches | (66.68%)             |
| L1-dcache-load-misses   |     19,385,640 | miss rate unavailable | (74.99%)             |
| LLC-loads               |     11,011,845 | 26.7% LLC miss rate   | (75.01%)             |
| dTLB-loads              | 12,918,923,610 | ≈0.0% dTLB miss rate  | (66.67%)             |

## One thing that surprised me
- How small algebraic transformations can have a significant impact on latency.
- Interning symbols into a shared lookup table singinifiantly reduced latency by avoiding string comparisons.
- Reducing branch unpredictability improved p90 and p99 latency.