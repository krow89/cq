# GitHub Actions Workflow

This workflow automatically builds and tests `cq` on multiple platforms whenever code is pushed to the `main` branch or a pull request is created.

## Supported Platforms

- **macOS x86_64** (Intel) - `macos-13`
- **macOS ARM64** (Apple Silicon) - `macos-latest`
- **Linux x86_64** - `ubuntu-latest`
- **Linux ARM64** (Raspberry Pi) - `ubuntu-latest` with cross-compilation
- **Windows x86_64** - `windows-latest` (MSVC)

## What it does

1. **Checkout code** from the repository
2. **Build** the project:
   - Unix-like: Uses standard `Makefile`
   - Windows: Compiles with MSVC (cl.exe)
3. **Run tests** (Unix only for now)
4. **Upload artifacts**: Binary executables for each platform

## Viewing Results

- Go to the [Actions tab](https://github.com/iworokonit/cq/actions)
- Click on the latest workflow run
- View build logs for each platform
- Download build artifacts (binaries) from the workflow summary

## Local Testing

To test the build locally before pushing:

**macOS/Linux:**
```bash
make clean && make && make test
```

**Windows (MSVC):**
```cmd
build-windows.bat
```

## Platform-Specific Notes

### Linux ARM64 (Raspberry Pi)
- Cross-compiled using `gcc-aarch64-linux-gnu` on x86_64 runner
- Binary is ready for Raspberry Pi 3/4/5 (64-bit)
- Tests are skipped (would require ARM64 runner or QEMU emulation)
- To test on actual Raspberry Pi: download artifact and run locally

### Windows
- Uses MSVC compiler (cl.exe) from Visual Studio Build Tools
- Test framework requires porting to work on Windows
- Currently runs basic smoke test only

### macOS
- `macos-13`: x86_64 architecture (Intel)
- `macos-latest`: ARM64 architecture (Apple Silicon, M1/M2/M3)

### Linux
- Uses default GCC compiler on Ubuntu
- Full test suite runs successfully
