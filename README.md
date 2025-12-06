# Multicore Processor Simulator - 2026

A cycle-accurate simulator for a 4-core processor with MESI cache coherency protocol, pipelined execution, and shared memory bus.

[![Tests](https://img.shields.io/badge/tests-8%20passing-brightgreen)]()
[![Architecture](https://img.shields.io/badge/cores-4-blue)]()
[![Cache](https://img.shields.io/badge/cache-MESI-orange)]()

## Features

- **4 parallel cores** with independent instruction memories
- **5-stage pipeline** (Fetch → Decode → Execute → Mem → Write Back)
- **MESI cache coherency** with write-back, write-allocate policy
- **Round-robin bus arbitration** for fair memory access
- **Cycle-accurate simulation** with detailed tracing
- **Data hazard detection** with pipeline stalling
- **Branch delay slot** support

## Project Structure

```
multicore-processor/
├── src/
│   ├── Assembler/              # Assembly language tools
│   │   └── ca2025asm/          # Assembler (converts .asm → imem*.txt)
│   └── MulticoreProccessor/
│       ├── Core/
│       │   ├── include/        # Core, Pipeline, Cache, Opcodes headers
│       │   └── src/            # Core implementation files
│       ├── Interface/
│       │   ├── include/        # Bus, MainMemory, Files, Helpers headers
│       │   └── src/            # Interface implementation files
│       └── MulticoreProcessor.c
├── tests/
│   ├── counter/           # 4-core shared counter (cache coherency)
│   ├── mulserial/         # Single-core 16×16 matrix multiplication
│   ├── mulparallel/       # 4-core parallel matrix multiplication
│   ├── edge_cache/        # Cache conflict miss testing
│   ├── edge_mesi/         # MESI protocol state transitions
│   ├── edge_bus/          # Bus arbitration under contention
│   ├── edge_hazards/      # Pipeline hazards & branch delay slots
│   ├── example/           # Basic loop with delay slot
│   └── run_all_tests.sh   # Automated test runner
└── docs/                  # Documentation
```

## 2026 Specifications

### Key Changes from 2025
| Feature | 2025 | 2026 |
|---------|------|------|
| Cache size | 256 words | **512 words** |
| Block size | 4 words | **8 words** |
| Cache lines | 64 | 64 |
| Address space | 20 bits (1M words) | **21 bits (2M words)** |
| Bus trace addr | 5 hex digits | **6 hex digits** |
| Stats format | `decode_stall` | **`decode stall`** |

### Architecture Overview

```
┌─────────────────────────────────────────────────────────────┐
│                      Main Memory (2MB)                       │
└──────────────────────────┬──────────────────────────────────┘
                           │
              ┌────────────┴────────────┐
              │     Shared Bus (MESI)    │
              │   Round-Robin Arbitration │
              └─┬──────┬──────┬──────┬──┘
                │      │      │      │
         ┌──────┴┐ ┌───┴──┐ ┌─┴────┐ ┌┴─────┐
         │Core 0 │ │Core 1│ │Core 2│ │Core 3│
         ├───────┤ ├──────┤ ├──────┤ ├──────┤
         │ IMEM  │ │ IMEM │ │ IMEM │ │ IMEM │
         │ 1024w │ │1024w │ │1024w │ │1024w │
         ├───────┤ ├──────┤ ├──────┤ ├──────┤
         │Cache  │ │Cache │ │Cache │ │Cache │
         │ 512w  │ │ 512w │ │ 512w │ │ 512w │
         ├───────┤ ├──────┤ ├──────┤ ├──────┤
         │16 Regs│ │16Regs│ │16Regs│ │16Regs│
         └───────┘ └──────┘ └──────┘ └──────┘
```

### Pipeline Stages

```
┌───────┐   ┌────────┐   ┌─────────┐   ┌─────┐   ┌───────────┐
│ Fetch │ → │ Decode │ → │ Execute │ → │ Mem │ → │ WriteBack │
└───────┘   └────────┘   └─────────┘   └─────┘   └───────────┘
    │            │
    │            └── Branch resolution (with delay slot)
    │
    └── Instruction fetch from private IMEM
```

- **Hazard handling**: Stall in Decode stage (no forwarding/bypassing)
- **Memory stall**: Stall in Mem stage on cache miss
- **Delay slot**: Instruction after branch always executes

### MESI Cache Coherency

| State | Code | Description |
|-------|------|-------------|
| Invalid | 0 | Block not in cache |
| Shared | 1 | Clean copy, may exist in other caches |
| Exclusive | 2 | Clean copy, only copy in system |
| Modified | 3 | Dirty copy, must write back before eviction |

### Instruction Set

| Opcode | Name | Operation |
|--------|------|-----------|
| 0-8 | ADD, SUB, AND, OR, XOR, MUL, SLL, SRA, SRL | R[rd] = R[rs] op R[rt] |
| 9-14 | BEQ, BNE, BLT, BGT, BLE, BGE | Branch if condition |
| 15 | JAL | Jump and link |
| 16 | LW | Load word from memory |
| 17 | SW | Store word to memory |
| 20 | HALT | Stop execution |

## Building

### Windows (Visual Studio)
```bash
cd src/MulticoreProccessor
start MulticoreProcessor.sln
# Build with Ctrl+Shift+B
```

### Windows/Linux (GCC)
```bash
cd src/MulticoreProccessor
gcc -O2 -Wl,--stack,33554432 -I./Core/include -I./Interface/include \
    MulticoreProcessor.c Core/src/*.c Interface/src/*.c \
    -o sim.exe
```

## Running Tests

### Quick Start
```bash
# Run all tests
cd tests
bash run_all_tests.sh
```

### Manual Test Execution
```bash
# Navigate to test directory
cd tests/counter

# Run simulator (reads imem*.txt and memin.txt, writes output files)
./sim.exe

# Check results
cat memout.txt      # Memory output
cat regout0.txt     # Core 0 registers (r2-r15)
cat stats0.txt      # Core 0 statistics
```

### Test Suite

| Test | Description | Expected Result |
|------|-------------|-----------------|
| `counter` | 4 cores increment shared counter | MEM[0] = 512 |
| `mulserial` | 16×16 matrix multiply (1 core) | Correct product matrix |
| `mulparallel` | 16×16 matrix multiply (4 cores) | Same result, faster |
| `edge_cache` | Cache conflict misses | Values at conflicting addresses |
| `edge_mesi` | MESI state transitions | Cores see each other's writes |
| `edge_bus` | Bus arbitration | All cores write successfully |
| `edge_hazards` | RAW hazards + branches | Correct pipeline stalling |
| `example` | Loop with delay slot | r2-r8 = 100 |

## Output Files

| File | Description | Format |
|------|-------------|--------|
| `memout.txt` | Final main memory state | 8-digit hex per line |
| `regout0-3.txt` | Final register values (r2-r15) | 8-digit hex, 14 lines |
| `dsram0-3.txt` | Data cache contents | 8-digit hex, 512 lines |
| `tsram0-3.txt` | Tag/MESI cache state | 8-digit hex, 64 lines |
| `core0-3trace.txt` | Cycle-by-cycle execution | See format below |
| `bustrace.txt` | Bus transactions | See format below |
| `stats0-3.txt` | Performance counters | Key-value pairs |

### Core Trace Format
```
<cycle> <FE_PC> <DE_PC> <EX_PC> <ME_PC> <WB_PC> <r2> <r3> ... <r15>
0 000 --- --- --- --- 00000000 00000000 ... 00000000
```

### Bus Trace Format
```
<cycle> <origid> <cmd> <addr> <data> <shared>
5 2 1 00000F 00000000 0
```
- `origid`: 0-3 = cores, 4 = main memory
- `cmd`: 0 = none, 1 = BusRd, 2 = BusRdX, 3 = Flush

### Statistics Format
```
cycles 805
instructions 801
read_hit 100
write_hit 50
read_miss 10
write_miss 5
decode stall 20
mem stall 30
```

## Assembler

Convert assembly files to instruction memory format:

```bash
cd src/Assembler/ca2025asm/ca2025asm

# Assemble a program
./asm.exe program.asm imem0.txt

# The assembler generates:
# - imem0.txt: Instruction memory (hex-encoded instructions)
# - dsram0.txt: Initial data memory (if .data section exists)
```

### Assembly Syntax
```asm
# Comments start with #
add $r2, $r3, $imm, 5      # r2 = r3 + 5
lw $r4, $r5, $imm, 100     # r4 = MEM[r5 + 100]
sw $r4, $r5, $imm, 100     # MEM[r5 + 100] = r4
beq $imm, $r2, $r3, 10     # if r2 == r3, jump to PC=10
halt $zero, $zero, $zero, 0
```

## Implementation Details

### Source Files

| File | Purpose |
|------|---------|
| `MulticoreProcessor.c` | Main loop, core coordination |
| `Core/src/Core.c` | Core initialization, iteration, teardown |
| `Core/src/Pipeline.c` | 5-stage pipeline, hazard detection |
| `Core/src/Cache.c` | MESI cache, snooping, bus interface |
| `Core/src/Opcodes.c` | ALU operations, branches |
| `Interface/src/Bus.c` | Bus arbitration, transaction handling |
| `Interface/src/MainMemory.c` | Memory read/write, bus response |
| `Interface/src/Files.c` | File I/O management |

### Key Data Structures

```c
// Core structure
typedef struct {
    uint32_t register_array[16];
    uint32_t instructions_memory[1024];
    Pipeline_s pipeline;
    // ...
} Core_s;

// Cache structure (inside Pipeline)
typedef struct {
    uint32_t dsram[512];        // Data storage
    tsram_s tsram[64];          // Tag + MESI state
    // ...
} CacheData_s;
```

## Debugging Tips

1. **Check bus trace** for cache coherency issues:
   ```bash
   grep "BusRdX" bustrace.txt  # Write invalidations
   grep "Flush" bustrace.txt   # Data transfers
   ```

2. **Check core trace** for pipeline behavior:
   ```bash
   head -20 core0trace.txt     # First 20 cycles
   ```

3. **Verify cache state**:
   ```bash
   cat tsram0.txt | grep -v "00000000"  # Non-empty cache lines
   ```

## License

This project is part of the Computer Architecture course (0512.4461) at Tel Aviv University.

## Contributing

1. Fork the repository
2. Create a feature branch
3. Run all tests: `cd tests && bash run_all_tests.sh`
4. Submit a pull request