# LAMMPS 7Aug2019 + EChemDID Setup for Windows 11

[![LAMMPS](https://img.shields.io/badge/LAMMPS-7Aug2019-blue.svg)](https://www.lammps.org/)
[![Windows](https://img.shields.io/badge/Windows-11-0078D6.svg)](https://www.microsoft.com/windows)
[![Build System](https://img.shields.io/badge/Build-MSYS2%20MinGW64-green.svg)](https://www.msys2.org/)
[![MPI](https://img.shields.io/badge/MPI-MS--MPI%2010.1.1-orange.svg)](https://docs.microsoft.com/en-us/message-passing-interface/microsoft-mpi)

Complete setup guide and Windows compatibility fixes for building LAMMPS with EChemDID electrochemistry package on Windows 11.

---

## 🎯 Project Overview

This repository provides a **tested, working solution** for compiling and running LAMMPS (Large-scale Atomic/Molecular Massively Parallel Simulator) with the EChemDID electrochemistry fix on Windows 11.

**What makes this special:**
- ✅ Addresses critical Windows memory allocation issues
- ✅ Provides step-by-step setup from scratch
- ✅ Includes source code patches for Windows compatibility
- ✅ Enables **parallel MPI execution** on Windows
- ✅ Successfully tested with electrochemistry simulations

---

## 🖥️ System Requirements

**Hardware:**
- CPU: Multi-core processor (tested on Intel i7)
- RAM: 8GB minimum, 16GB recommended
- Storage: 5GB free space

**Software:**
- **OS:** Windows 11 (tested on Build 22631)
- **Build Environment:** MSYS2 with MinGW64 toolchain
- **Compiler:** GCC 15.2.0 (MinGW-w64)
- **MPI:** MS-MPI 10.1.1
- **Required:** Admin rights for MS-MPI installation

**LAMMPS Version:**
- **LAMMPS:** 7 Aug 2019 (specific version required)
- **EChemDID:** 22 Aug 2018 (from LAMMPS-hacks-public)

---

## 🚀 Quick Start

### 1. Install Prerequisites
```bash
# Install MSYS2 from https://www.msys2.org/
# Install MS-MPI from Microsoft

# In MSYS2 MinGW64 terminal:
pacman -Syu
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-make
```

### 2. Download Source Code
```bash
cd ~/Documents
wget https://download.lammps.org/tars/lammps-7Aug19.tar.gz
tar -xzf lammps-7Aug19.tar.gz

git clone https://github.com/srtee/LAMMPS-hacks-public.git
```

### 3. Apply Windows Compatibility Fixes
Apply the patches from `warnings_results/` to the LAMMPS source:
- `my_pool_chunk.h` → Replace `posix_memalign` with `_aligned_malloc`
- `my_page.h` → Replace `posix_memalign` with `_aligned_malloc`
- `memory.cpp` → Add Windows-specific memory functions

**See [WINDOWS_COMPATIBILITY_SUMMARY.md](WINDOWS_COMPATIBILITY_SUMMARY.md) for detailed instructions.**

### 4. Compile
```bash
cd ~/Documents/lammps-7Aug19/src
cp /path/to/Makefile.mingw64_mpi MAKE/
make clean-all
make mingw64_mpi -j4
```

### 5. Test
```bash
# Single process
./lmp_mingw64_mpi.exe -in ../examples/melt/in.melt

# MPI parallel (4 processes)
mpiexec -n 4 ./lmp_mingw64_mpi.exe -in ../examples/melt/in.melt
```

**Success:** Simulation completes with timing statistics, no crashes!

---

## 📚 Documentation

### Main Guides

| Document | Description |
|----------|-------------|
| [**ASUS_LAMMPS_Setup_Guide.md**](ASUS_LAMMPS_Setup_Guide.md) | **START HERE** - Complete step-by-step setup from scratch |
| [**WINDOWS_COMPATIBILITY_SUMMARY.md**](WINDOWS_COMPATIBILITY_SUMMARY.md) | Technical reference for Windows-specific fixes |
| [**GIT_COMMIT_MESSAGES.md**](GIT_COMMIT_MESSAGES.md) | Detailed commit history and change documentation |

### Quick Navigation

**For first-time setup:**
1. Read [ASUS_LAMMPS_Setup_Guide.md](ASUS_LAMMPS_Setup_Guide.md)
2. Follow Phases 1-7 in order
3. Pay special attention to **Phase 5.7** (Windows fixes)

**For understanding the fixes:**
1. Read [WINDOWS_COMPATIBILITY_SUMMARY.md](WINDOWS_COMPATIBILITY_SUMMARY.md)
2. Review the "Root Causes" section
3. Apply fixes to your source code

**If you encounter issues:**
1. Check the Troubleshooting section in the setup guide
2. Review error messages in `warnings_results/` folder
3. Compare your fixes with the reference files

---

## 📂 Repository Structure

```
lammps-echemdid-windows-setup/
├── README.md                           # This file
├── ASUS_LAMMPS_Setup_Guide.md          # Complete setup guide
├── WINDOWS_COMPATIBILITY_SUMMARY.md    # Technical documentation
├── GIT_COMMIT_MESSAGES.md              # Commit documentation
│
├── warnings_results/                   # Reference implementations
│   ├── my_pool_chunk.h                 # Fixed header (Windows compatible)
│   ├── my_page.h                       # Fixed header (Windows compatible)
│   ├── memory.cpp                      # Fixed source (Windows compatible)
│   ├── Makefile.mingw64_mpi            # Custom Windows makefile
│   ├── build_output.txt                # Successful compilation log
│   ├── gdb_output.txt                  # Debugging session output
│   └── [test outputs]                  # Various test results
│
└── error_files/                        # Historical error logs
    ├── error_1_6.txt
    └── error_2_3.txt
```

---

## 🔧 Key Files

### Source Code Fixes (in `warnings_results/`)

| File | Purpose | Key Change |
|------|---------|------------|
| `my_pool_chunk.h` | Memory pooling template | `posix_memalign` → `_aligned_malloc` |
| `my_page.h` | Page-based allocation | `posix_memalign` → `_aligned_malloc` |
| `memory.cpp` | Core memory wrapper | Added Windows `_aligned_*` functions |
| `Makefile.mingw64_mpi` | Build configuration | KISS FFT, psapi library |

### Build Artifacts

| File | Content |
|------|---------|
| `build_output.txt` | Successful compilation log |
| `gdb_output.txt` | Heap corruption diagnosis |
| `7_2.txt`, `7_3.txt` | Testing session outputs |

---

## ⚠️ Critical Windows Requirements

### 1. Use MSYS2 MinGW64 **ONLY**
❌ **DO NOT USE:**
- Git Bash (incompatible runtime)
- MSYS2 MSYS terminal (wrong toolchain)
- WSL/Ubuntu (different build system)
- Cygwin (POSIX layer incompatibility)

✅ **MUST USE:** MSYS2 MinGW64 terminal

### 2. Memory Allocation Rules
Windows requires matched allocation/deallocation:
- `_aligned_malloc()` → MUST use `_aligned_free()`
- `_aligned_realloc()` → MUST use `_aligned_free()`
- **Never** mix with standard `malloc()`/`free()`

Violating this causes heap corruption (error `0xc0000374`)

### 3. Version Specificity
- **LAMMPS:** Exactly 7 Aug 2019 (not 2018, not 2020)
- **EChemDID:** Exactly 22 Aug 2018
- Mixing versions causes compilation conflicts

---

## ✅ Validation & Testing

### Successful Build Indicators
- ✅ Compilation completes without errors
- ✅ Creates `lmp_mingw64_mpi.exe` (~15-25 MB)
- ✅ `./lmp_mingw64_mpi.exe -h` displays help
- ✅ Single process runs complete successfully
- ✅ MPI runs (2, 4 processes) work without crashes

### Known Working Configuration
```
OS: Windows 11 Build 22631
CPU: Intel i7 (12th gen)
RAM: 16GB
Compiler: GCC 15.2.0 (MinGW-w64)
MPI: MS-MPI 10.1.1
Build Time: ~5-10 minutes (4 cores)
```

### Test Commands
```bash
# Basic functionality
lmp_mingw64_mpi.exe -in examples/melt/in.melt

# MPI scaling
mpiexec -n 2 lmp_mingw64_mpi.exe -in examples/melt/in.melt
mpiexec -n 4 lmp_mingw64_mpi.exe -in examples/melt/in.melt

# EChemDID presence
lmp_mingw64_mpi.exe -h | grep -i echemdid
```

---

## 🐛 Common Issues & Solutions

### Issue: `posix_memalign` not declared
**Cause:** Windows lacks this POSIX function  
**Solution:** Apply fixes from Phase 5.7 in setup guide

### Issue: Heap corruption (0xc0000374) in MPI
**Cause:** Memory free/alloc mismatch  
**Solution:** Ensure all `_aligned_malloc` uses `_aligned_free`

### Issue: `command not found: lmp_mingw64_mpi`
**Cause:** Executable not in PATH or wrong name  
**Solution:** Use full path or add to PATH; include `.exe` extension

### Issue: MPI processes fail to start
**Cause:** MS-MPI not in PATH  
**Solution:** Add `C:\Program Files\Microsoft MPI\Bin` to PATH

---

## 📖 Background & Motivation

This project was created to enable electrochemistry simulations using the EChemDID fix in LAMMPS on Windows systems. Previous attempts failed due to:
1. Mixed LAMMPS versions causing conflicts
2. Improper handling of Windows memory allocation APIs
3. Incomplete or incorrect setup guides

This repository provides a **clean, tested solution** that works from scratch.

---

## 🤝 Contributing

Found an issue or have improvements? Contributions welcome!

1. Test the setup guide on your Windows system
2. Document any deviations or additional fixes needed
3. Submit issues or pull requests with:
   - Your Windows version
   - Error messages (complete)
   - Steps to reproduce
   - Proposed solutions

---

## 📄 License

- **LAMMPS:** GPL v2 (See LAMMPS license)
- **EChemDID:** Original license from LAMMPS-hacks-public
- **This Guide:** MIT License (documentation and fixes)

```
MIT License - Documentation and Windows compatibility fixes
Copyright (c) 2025

Permission is hereby granted, free of charge, to any person obtaining a copy
of this documentation and associated fixes, to deal in the Documentation without
restriction, including without limitation the rights to use, copy, modify, merge,
publish, distribute, sublicense, and/or sell copies of the Documentation, and to
permit persons to whom the Documentation is furnished to do so.
```

---

## 🔗 External Resources

- [LAMMPS Official Site](https://www.lammps.org/)
- [LAMMPS Documentation](https://docs.lammps.org/)
- [LAMMPS-hacks-public (EChemDID source)](https://github.com/srtee/LAMMPS-hacks-public)
- [MSYS2](https://www.msys2.org/)
- [MS-MPI Download](https://docs.microsoft.com/en-us/message-passing-interface/microsoft-mpi)
- [Windows Aligned Memory API](https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/aligned-malloc)

---

## 📞 Support

**For LAMMPS questions:** [LAMMPS Users Mailing List](https://www.lammps.org/mail.html)  
**For EChemDID questions:** [LAMMPS-hacks-public Issues](https://github.com/srtee/LAMMPS-hacks-public/issues)  
**For this guide:** Open an issue in this repository

---

## ✨ Acknowledgments

- Sandia National Laboratories for LAMMPS
- Shu-Ting (Raymond) Tsai for EChemDID
- MSYS2 Project for the excellent build environment
- Microsoft for MS-MPI

---

**Built with:** ❤️ and lots of debugging

**Last Updated:** October 24, 2025  
**Guide Version:** 1.0  
**Status:** ✅ Tested and Working
