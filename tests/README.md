# Assembly Test Cases

This directory contains test cases for the multicore processor simulator.

## Test Cases

### 1. example
Basic test case demonstrating core functionality.

### 2. counter
Each core increments a counter in shared memory, testing:
- Basic arithmetic operations
- Memory access and cache coherency
- MESI protocol

### 3. mulparallel
Parallel multiplication test where cores work together to multiply matrices/arrays.

### 4. mulserial
Serial multiplication test for performance comparison.

## File Structure

Each test directory contains:
- `imem0.txt`, `imem1.txt`, `imem2.txt`, `imem3.txt` - Instruction memory for each core
- `memin.txt` - Initial main memory contents
- `*.asm` files (where available) - Assembly source code
- Expected outputs:
  - `regout0-3.txt` - Register values after execution
  - `dsram0-3.txt` - Data cache contents (512 words for 2026)
  - `tsram0-3.txt` - Tag cache contents (64 frames for 2026)
  - `bustrace.txt` - Bus transaction log
  - `core0-3trace.txt` - Core execution traces
  - `memout.txt` - Final main memory state
  - `stats0-3.txt` - Performance statistics

## Running Tests

From `src/MulticoreProccessor` directory:

```bash
# Copy test files
cp ../../tests/example/{imem0.txt,imem1.txt,imem2.txt,imem3.txt,memin.txt} .

# Run simulator
./MulticoreProcessor.exe

# Compare outputs with expected
diff regout0.txt ../../tests/example/regout0.txt
```
