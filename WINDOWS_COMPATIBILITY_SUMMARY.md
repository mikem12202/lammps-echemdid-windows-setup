# Windows Compatibility Fixes for LAMMPS 7Aug2019

## Overview

This document summarizes the critical Windows-specific fixes required to successfully compile and run LAMMPS 7Aug2019 with EChemDID on Windows 11 using MSYS2 MinGW64.

**Date:** October 24, 2025  
**System:** Windows 11, ASUS TUF Dash F15  
**Compiler:** GCC 15.2.0 (MinGW-w64)  
**MPI:** MS-MPI 10.1.1  
**Build System:** MSYS2 MinGW64 (NOT Git Bash)

---

## Problem Statement

LAMMPS 7Aug2019 uses POSIX-specific memory alignment functions (`posix_memalign`) that are not available on Windows. Additionally, Windows requires that memory allocated with aligned allocation functions (`_aligned_malloc`) must be freed with the corresponding aligned free function (`_aligned_free`). Mixing these causes heap corruption (error code `0xc0000374`).

---

## Root Causes

### 1. Missing POSIX Functions
- **Function:** `posix_memalign()`
- **Issue:** Not available in Windows MinGW64
- **Solution:** Use Windows-specific `_aligned_malloc()` instead
- **Affected Files:** `my_pool_chunk.h`, `my_page.h`, `memory.cpp`

### 2. Memory Free Mismatch
- **Issue:** Memory allocated with `_aligned_malloc()` was being freed with `free()`
- **Result:** Heap corruption (0xc0000374) during object destruction
- **Solution:** Use `_aligned_free()` for aligned memory on Windows
- **Critical Impact:** Manifested primarily in MPI runs due to more object lifecycle events

### 3. Memory Reallocation Mismatch
- **Issue:** Using `realloc()` on memory allocated with `_aligned_malloc()`
- **Result:** Heap corruption during memory resizing
- **Solution:** Use `_aligned_realloc()` for Windows aligned memory
- **Affected Function:** `Memory::srealloc()` in `memory.cpp`

---

## Required Source Code Modifications

### File 1: `my_pool_chunk.h`

**Location 1 - allocate() function (around line 196-213):**

Replace:
```cpp
#if defined(LAMMPS_MEMALIGN)
  void *ptr;
  if (posix_memalign(&ptr, LAMMPS_MEMALIGN,
                     chunkperpage*chunksize[ibin]*sizeof(T)))
    errorflag = 2;
  pages[i] = (T *) ptr;
#else
```

With:
```cpp
#if defined(LAMMPS_MEMALIGN)
  void *ptr;
#if defined(_WIN32) || defined(_WIN64)
  // Windows: use _aligned_malloc instead of posix_memalign
  ptr = _aligned_malloc(chunkperpage * chunksize[ibin] * sizeof(T), LAMMPS_MEMALIGN);
  if (!ptr) errorflag = 2;
  pages[i] = (T*)ptr;
#else
  // POSIX systems: use posix_memalign
  int retval = posix_memalign(&ptr, LAMMPS_MEMALIGN,
      chunkperpage * chunksize[ibin] * sizeof(T));
  if (retval) errorflag = 2;
  pages[i] = (T*)ptr;
#endif
#else
```

**Location 2 - Destructor ~MyPoolChunk() (around line 102-112):**

Replace:
```cpp
~MyPoolChunk() {
  delete [] freehead;
  delete [] chunksize;
  if (npage) {
    free(freelist);
    for (int i = 0; i < npage; i++) free(pages[i]);
    free(pages);
    free(whichbin);
  }
}
```

With:
```cpp
~MyPoolChunk() {
  delete [] freehead;
  delete [] chunksize;
  if (npage) {
    free(freelist);
    for (int i = 0; i < npage; i++) {
#if defined(LAMMPS_MEMALIGN) && (defined(_WIN32) || defined(_WIN64))
      _aligned_free(pages[i]);
#else
      free(pages[i]);
#endif
    }
    free(pages);
    free(whichbin);
  }
}
```

---

### File 2: `my_page.h`

**Location 1 - allocate() function (around line 221-235):**

Replace:
```cpp
for (int i = npage-pagedelta; i < npage; i++) {
#if defined(LAMMPS_MEMALIGN)
  void *ptr;
  if (posix_memalign(&ptr, LAMMPS_MEMALIGN, pagesize*sizeof(T)))
    errorflag = 2;
  pages[i] = (T *) ptr;
#else
```

With:
```cpp
for (int i = npage-pagedelta; i < npage; i++) {
#if defined(LAMMPS_MEMALIGN)
  void *ptr;
#if defined(_WIN32) || defined(_WIN64)
  // Windows: use _aligned_malloc instead of posix_memalign
  ptr = _aligned_malloc(pagesize*sizeof(T), LAMMPS_MEMALIGN);
  if (!ptr) errorflag = 2;
  pages[i] = (T *) ptr;
#else
  // POSIX systems: use posix_memalign
  if (posix_memalign(&ptr, LAMMPS_MEMALIGN, pagesize*sizeof(T)))
    errorflag = 2;
  pages[i] = (T *) ptr;
#endif
#else
```

**Location 2 - Destructor ~MyPage() (around line 90):**

Replace:
```cpp
~MyPage() {
  for (int i = 0; i < npage; i++) free(pages[i]);
  free(pages);
}
```

With:
```cpp
~MyPage() {
  for (int i = 0; i < npage; i++) {
#if defined(LAMMPS_MEMALIGN) && (defined(_WIN32) || defined(_WIN64))
    _aligned_free(pages[i]);
#else
    free(pages[i]);
#endif
  }
  free(pages);
}
```

**Location 3 - init() function (around line 84):**

Replace:
```cpp
// free any previously allocated pages
for (int i = 0; i < npage; i++) free(pages[i]);
free(pages);
```

With:
```cpp
// free any previously allocated pages
for (int i = 0; i < npage; i++) {
#if defined(LAMMPS_MEMALIGN) && (defined(_WIN32) || defined(_WIN64))
  _aligned_free(pages[i]);
#else
  free(pages[i]);
#endif
}
free(pages);
```

---

### File 3: `memory.cpp`

**Location 1 - smalloc() function (around line 42-64):**

Replace:
```cpp
#if defined(LAMMPS_MEMALIGN)
  void *ptr;

#if defined(LMP_USE_TBB_ALLOCATOR)
  ptr = scalable_aligned_malloc(nbytes, LAMMPS_MEMALIGN);
#else
  int retval = posix_memalign(&ptr, LAMMPS_MEMALIGN, nbytes);
  if (retval) ptr = NULL;
#endif
```

With:
```cpp
#if defined(LAMMPS_MEMALIGN)
  void *ptr;

#if defined(LMP_USE_TBB_ALLOCATOR)
  ptr = scalable_aligned_malloc(nbytes, LAMMPS_MEMALIGN);
#elif defined(_WIN32) || defined(_WIN64)
  // Windows: use _aligned_malloc instead of posix_memalign
  ptr = _aligned_malloc(nbytes, LAMMPS_MEMALIGN);
#else
  // POSIX systems: use posix_memalign
  int retval = posix_memalign(&ptr, LAMMPS_MEMALIGN, nbytes);
  if (retval) ptr = NULL;
#endif
```

**Location 2 - srealloc() function (around line 75-105):**

Add Windows branch:
```cpp
#if defined(LMP_USE_TBB_ALLOCATOR)
  ptr = scalable_aligned_realloc(ptr, nbytes, LAMMPS_MEMALIGN);
#elif defined(LMP_INTEL_NO_TBB) && defined(LAMMPS_MEMALIGN) && \
      defined(__INTEL_COMPILER)
  [existing Intel code]
#elif defined(LAMMPS_MEMALIGN) && (defined(_WIN32) || defined(_WIN64))
  // Windows: use _aligned_realloc for memory allocated with _aligned_malloc
  ptr = _aligned_realloc(ptr, nbytes, LAMMPS_MEMALIGN);
#else
  ptr = realloc(ptr,nbytes);
#endif
```

**Location 3 - sfree() function (around line 110-120):**

Replace:
```cpp
void Memory::sfree(void *ptr)
{
  if (ptr == NULL) return;
#if defined(LMP_USE_TBB_ALLOCATOR)
  scalable_aligned_free(ptr);
#else
  free(ptr);
#endif
}
```

With:
```cpp
void Memory::sfree(void *ptr)
{
  if (ptr == NULL) return;
#if defined(LMP_USE_TBB_ALLOCATOR)
  scalable_aligned_free(ptr);
#elif defined(LAMMPS_MEMALIGN) && (defined(_WIN32) || defined(_WIN64))
  // Windows: use _aligned_free for memory allocated with _aligned_malloc
  _aligned_free(ptr);
#else
  free(ptr);
#endif
}
```

---

## Build Process

### Prerequisites
1. MSYS2 MinGW64 (NOT Git Bash or MSYS2 MSYS)
2. MS-MPI 10.1.1 installed
3. MinGW64 packages: gcc, g++, make

### Compilation Steps

```bash
# In MSYS2 MinGW64 terminal
cd /c/Users/$USER/Documents/lammps-7Aug19/src

# Apply all fixes above first

# Clean build
make clean-all

# Compile
make mingw64_mpi -j4

# Verify
ls -lh lmp_mingw64_mpi.exe
```

---

## Testing

### Single Process Test
```bash
cd ~/lammps-7Aug19/examples/melt
lmp_mingw64_mpi.exe -in in.melt
```

**Expected:** Completes successfully with performance statistics

### MPI Test
```bash
mpiexec -n 2 lmp_mingw64_mpi.exe -in in.melt
mpiexec -n 4 lmp_mingw64_mpi.exe -in in.melt
```

**Expected:** Both complete without heap corruption errors

---

## Key Lessons

1. **Always match allocation and free functions on Windows:**
   - `_aligned_malloc()` → `_aligned_free()`
   - `_aligned_realloc()` → `_aligned_free()`
   - Never mix with standard `malloc()`/`free()`

2. **MPI amplifies memory management issues:**
   - Single process may work even with minor issues
   - MPI creates/destroys more objects, exposing heap corruption

3. **Conditional compilation is essential:**
   - Use `#if defined(_WIN32) || defined(_WIN64)` guards
   - Preserve POSIX code paths for Linux/Mac compatibility

4. **Management arrays don't need alignment:**
   - Only actual data pages need `_aligned_malloc()`
   - Pointer arrays can use regular `realloc()`

5. **Use MSYS2 MinGW64 exclusively:**
   - Git Bash lacks proper compiler toolchain
   - MSYS2 MSYS uses incompatible runtime

---

## Debugging Tips

### If heap corruption still occurs:
```bash
# Run with debugger
pacman -S mingw-w64-x86_64-gdb
gdb lmp_mingw64_mpi.exe
(gdb) run -in input.in
(gdb) bt  # Get backtrace on crash
```

### Check for alignment issues:
```bash
# Search for remaining posix_memalign calls
grep -r "posix_memalign" src/

# Search for free() calls on aligned memory
grep -A5 "LAMMPS_MEMALIGN" src/*.cpp src/*.h | grep "free("
```

---

## Makefile Configuration

**Key settings in Makefile.mingw64_mpi:**
```makefile
CC = mpicxx
LINK = mpic++
LMP_INC = -DLAMMPS_GZIP -DLAMMPS_MEMALIGN=64
FFT_INC = -DFFT_KISS
LIB = -lpsapi
```

**Why these settings:**
- `LAMMPS_MEMALIGN=64`: Enables 64-byte alignment for performance
- `FFT_KISS`: Simpler than FFTW3, no external dependencies
- `-lpsapi`: Required for Windows process memory info functions

---

## Success Criteria

✅ Compiles without errors  
✅ Single process execution completes  
✅ MPI execution with 2+ processes completes  
✅ No heap corruption errors (0xc0000374)  
✅ EChemDID fix appears in help output  
✅ Examples run to completion  

---

## Version Information

- **LAMMPS:** 7 Aug 2019 (tarball from https://download.lammps.org/tars/)
- **EChemDID:** 22 Aug 2018 (from LAMMPS-hacks-public GitHub)
- **Tested on:** Windows 11, Build 22631
- **Compiler:** GCC 15.2.0 (x86_64-w64-mingw32)
- **MPI:** MS-MPI 10.1.1.16471

---

## References

- Windows Aligned Memory Functions: https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/aligned-malloc
- LAMMPS Documentation: https://docs.lammps.org
- MSYS2 Documentation: https://www.msys2.org/

---

**Document Version:** 1.0  
**Last Updated:** October 24, 2025
