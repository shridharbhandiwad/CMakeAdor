# Combined Radar Data Extraction Tool

This project combines the functionality of raw data extraction and target data extraction into a single executable.

## Features

- **Raw Data Extraction**: Extract raw ADC samples from radar sensors
- **Target Data Extraction**: Extract processed target detection data with UDP transmission
- **Combined Mode**: Run both extraction types simultaneously
- **Command Line Options**: Choose extraction mode via command line arguments
- **Cross-Platform**: Works on Windows (with UDP support) and Linux/Unix systems

## Building

### Prerequisites
- CMake 3.10 or higher
- C99 compatible compiler
- Radar communication library headers in `include/` directory

### Build Instructions

```bash
mkdir build
cd build
cmake ..
make
```

On Windows with Visual Studio:
```cmd
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

This will create an executable named `extract_data` (or `extract_data.exe` on Windows).

## Usage

### Command Line Options

```bash
./extract_data [options]
```

**Options:**
- `-r, --raw-only`: Extract raw ADC data only
- `-t, --target-only`: Extract target detection data only  
- `-b, --both`: Extract both raw and target data (default)
- `-h, --help`: Show help message

### Examples

```bash
# Extract only raw ADC data
./extract_data --raw-only

# Extract only target detection data (with UDP transmission on Windows)
./extract_data --target-only

# Extract both types of data simultaneously
./extract_data --both

# Show help
./extract_data --help
```

## Output

### Raw Data Mode
- Prints ADC sample values to console
- Shows first 10 samples per frame to avoid overwhelming output
- Runs with 1-second trigger intervals

### Target Data Mode  
- Prints target detection information (ID, position, velocity, etc.)
- On Windows: Sends data via UDP to `127.0.0.1:5000` every 20th frame
- Runs with 100-second trigger intervals

### Combined Mode
- Outputs both raw and target data with clear section headers
- Different trigger timings for each data type

## Architecture

The combined executable merges the functionality from:
- `extract_raw_data.c` - Original raw data extraction
- `extract_target_data.c` - Original target data extraction with UDP

Key improvements:
- Single executable for all functionality
- Command line argument parsing
- Platform-aware compilation (UDP only on Windows)
- Clear output formatting with section headers
- Configurable extraction modes

## Platform Notes

- **Windows**: Full functionality including UDP transmission of target data
- **Linux/Unix**: Raw and target data extraction (UDP features disabled)
- Automatic COM port detection and connection
- Cross-platform delay handling (Sleep/usleep)

## Dependencies

- Infineon radar communication library
- Windows: Winsock2 (ws2_32.lib) for UDP functionality
- Standard C library (stdio, stdlib, string, stdint)