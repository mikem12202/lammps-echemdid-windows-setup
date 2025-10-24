# ASUS TUF Dash F15 - LAMMPS Environment Setup Guide

**Target Configuration:**
- LAMMPS Version: 7 Aug 2019
- EChemDID Version: 22 Aug 2018 (from LAMMPS-hacks-public repo)
- Parallel Execution: MPI enabled (lmp_mingw64_mpi)
- Operating System: Windows 11
- Build System: MSYS2 MinGW64 (NOT Git Bash)
- Compiler: GCC 15.2.0 (MinGW-w64)
- MPI: MS-MPI 10.1.1

**Date Started:** October 24, 2025

**Critical Notes:**
- ⚠️ **MUST use MSYS2 MinGW64 terminal exclusively** - Git Bash will cause compilation issues
- ⚠️ **Windows requires special memory allocation fixes** - See Phase 5.7 for details
- ⚠️ Use pure 7Aug2019 tarball, not git clone (prevents version mixing)
- ⚠️ Executable name is `lmp_mingw64_mpi.exe`, not `lmp_mpi`

**Lessons from Previous Attempt:**
- ⚠️ Avoid mixing source code from different LAMMPS versions (caused compilation conflicts)
- ⚠️ Windows lacks POSIX `posix_memalign` - requires `_aligned_malloc` wrapper
- ⚠️ Memory allocated with `_aligned_malloc` MUST be freed with `_aligned_free`
- ⚠️ USER-QEQ is built-in to 2019 LAMMPS, not a separate USER package
- ⚠️ KISS FFT is simpler than FFTW3 for Windows builds
- ⚠️ Avoid Windows Git bash tools in PATH during compilation

---

## Phase 1: Complete Cleanup & Uninstallation

### 1.1 Remove Previous LAMMPS Installation (Windows)

**In PowerShell or Command Prompt:**
```cmd
# Navigate to Documents folder
cd %USERPROFILE%\Documents

# Remove LAMMPS source directories
rmdir /S /Q lammps-2019
rmdir /S /Q lammps
rmdir /S /Q lammps-*

# Remove auxiliary source directories from previous attempts
rmdir /S /Q reaxsrc
rmdir /S /Q qeqsrc

# Remove any LAMMPS binaries if moved to other locations
del /F lmp_serial.exe
del /F lmp_mpi.exe
```

### 1.2 Remove EChemDID and LAMMPS-hacks Installation

```cmd
# Remove LAMMPS-hacks-public directory
cd %USERPROFILE%\Documents
rmdir /S /Q LAMMPS-hacks-public

# Remove any standalone EChemDID directories
rmdir /S /Q EChemDID
rmdir /S /Q ECHEMDID-*
```

### 1.3 Clean Up Modified/Corrupted Source Files

**Important: From previous attempt, these files were manually edited:**
```cmd
# These will be removed with the directories above, but note what was changed:
# - src/my_pool_chunk.h (posix_memalign -> _aligned_malloc)
# - src/my_page.h (same memory allocation fix)
# - src/compute_reaxff_atom.cpp (added #include <vector>, commented utils/fmt)
# - src/pair_reaxff.h (added #include <vector>)
# - src/memory.cpp (cleaned up for compatibility)
# - Deleted: fix_qeq_ctip.cpp, fix_qeq_ctip.h (modern files conflicting with 2019)
```

**⚠️ KEY LESSON:** We will NOT manually edit these files in the new build. Instead, we'll use the correct build environment.

### 1.4 Review and Clean PATH Environment Variables

**Check current PATH for conflicts:**
```cmd
# In PowerShell, view PATH
$env:PATH -split ';'

# Look for and note these entries:
# - C:\Program Files\Git\usr\bin (caused 'tr' command issues)
# - C:\Progra~1\Git\usr\bin (also problematic)
# - Any LAMMPS-related paths
```

**Clean PATH temporarily (we'll set it correctly later):**
1. Press `Win + X` → System → Advanced system settings → Environment Variables
2. In "User variables" and "System variables", edit PATH
3. Remove or temporarily disable:
   - `C:\Program Files\Git\usr\bin` (Git bash tools conflict with MSYS2)
   - Any old LAMMPS paths
   - Any paths that might interfere with MSYS2

**Note:** Keep Git itself (for git.exe), just remove the Unix tools directory.

### 1.5 Clean MSYS2 Environment (if previously installed)

**Option A: Clean existing MSYS2 installation**
```bash
# In MSYS2 MinGW64 shell
cd ~
rm -rf lammps-2019
rm -rf LAMMPS-hacks-public
rm -rf reaxsrc
rm -rf qeqsrc

# Clean any built objects in MSYS2 home
rm -rf *.o *.exe
```

**Option B: Fresh MSYS2 installation (recommended)**
1. Uninstall MSYS2 from "Add or Remove Programs"
2. Delete: `C:\msys64` directory (if it still exists)
3. We'll reinstall fresh in Phase 2

### 1.6 Verify Visual Studio and MS-MPI Status

**Check Visual Studio (optional, not required for MSYS2 build):**
```cmd
# Visual Studio may or may not be needed - we'll use MSYS2 instead
dir "C:\Program Files\Microsoft Visual Studio\2022\Community"
```

**Check MS-MPI installation:**
```cmd
# Test if mpiexec works
mpiexec -n 2 hostname

# Check MPI headers exist (try both locations)
dir "C:\Program Files (x86)\Microsoft SDKs\MPI\Include\mpi.h"
dir "C:\Program Files\Microsoft SDKs\MPI\Include\mpi.h"
```

**⚠️ If MS-MPI is NOT installed (headers not found or mpiexec fails):**

You'll need to reinstall MS-MPI. Continue to Phase 1.7 for reinstallation instructions.

**✅ If MS-MPI IS installed:**

You can skip Phase 1.7 and proceed to Phase 1.8.

### 1.7 Reinstall MS-MPI (If Needed)

**⚠️ Only follow this if Phase 1.6 showed MS-MPI is missing**

**Download MS-MPI:**
1. Go to: https://github.com/microsoft/Microsoft-MPI/releases
2. Find the latest release (or use v10.1.2 which is stable)
3. Download BOTH files:
   - `msmpisetup.exe` (the runtime)
   - `msmpisdk.msi` (the SDK with headers/libraries)

**Install MS-MPI (IMPORTANT: Install in this order):**
```cmd
# 1. First, run msmpisetup.exe
#    - Double-click msmpisetup.exe
#    - Accept defaults and install
#    - This installs the MPI runtime (mpiexec.exe)

# 2. Then, run msmpisdk.msi
#    - Double-click msmpisdk.msi
#    - Accept defaults and install
#    - This installs headers and libraries for compiling

# 3. Restart PowerShell or Command Prompt
```

**Verify MS-MPI Installation:**
```cmd
# Open NEW PowerShell or Command Prompt window

# Test mpiexec
mpiexec -n 2 hostname

# Should show your computer name twice, like:
# DESKTOP-XXXXXX
# DESKTOP-XXXXXX

# Check for MPI headers (try both paths)
dir "C:\Program Files (x86)\Microsoft SDKs\MPI\Include\mpi.h"
dir "C:\Program Files\Microsoft SDKs\MPI\Include\mpi.h"

# One of these should succeed and show the mpi.h file

# Check MPI library files
dir "C:\Program Files (x86)\Microsoft SDKs\MPI\Lib\x64\*.lib"
```

**If still not working:**
- Reboot Windows
- Check if MS-MPI is in "Add or Remove Programs"
- Try uninstalling and reinstalling

### 1.8 Verify Cleanup Checklist

```cmd
# In PowerShell, verify these return "not found" or empty:
cd %USERPROFILE%\Documents
dir lammps-2019  # Should not exist
dir LAMMPS-hacks-public  # Should not exist
dir reaxsrc  # Should not exist
dir qeqsrc  # Should not exist

# MPI must work (if this fails, go back to Phase 1.7):
mpiexec -n 2 hostname  # Should succeed and show hostname twice

# Git should still work:
git --version  # Should succeed

# Visual Studio check (optional):
where cl.exe  # May or may not be in PATH, that's OK
```

**Phase 1 Complete! ✅**

Once all checks pass (especially MS-MPI), proceed to Phase 2.

---

## Phase 2: System Preparation & Dependencies

### 2.1 Install/Update MSYS2 (Fresh Installation)

**Download and Install MSYS2:**
1. Go to https://www.msys2.org
2. Download the installer (msys2-x86_64-[date].exe)
3. Run installer with default settings
4. Install to: `C:\msys64` (default)
5. **Important:** At the end, check "Run MSYS2 now"

**Initial MSYS2 Setup:**
```bash
# In the MSYS2 MSYS shell that opens, update the package database
pacman -Syu

# The terminal will close - this is normal
# Reopen "MSYS2 MSYS" from Start Menu and update again
pacman -Syu
```

### 2.2 Install Build Tools in MSYS2 MinGW64 Environment

**⚠️ CRITICAL: Use MSYS2 MinGW64, NOT MSYS2 MSYS**

1. Close any MSYS2 windows
2. Open "MSYS2 MinGW64" from Start Menu (should have blue icon)

**Install essential tools:**
```bash
# Update package database
pacman -Syu

# Install base development tools
pacman -S base-devel

# Install MinGW64 GCC toolchain
pacman -S mingw-w64-x86_64-gcc
pacman -S mingw-w64-x86_64-gcc-fortran

# Install make
pacman -S make

# Install additional utilities
pacman -S mingw-w64-x86_64-cmake
pacman -S git
pacman -S wget
pacman -S unzip
```

### 2.3 Install MS-MPI Development Tools in MSYS2

**Install MS-MPI package for MinGW64:**
```bash
# In MSYS2 MinGW64 shell
pacman -S mingw-w64-x86_64-msmpi

# This installs MPI headers and libraries for compilation
```

**⚠️ Important: The MSYS2 msmpi package provides headers/libs but not mpiexec**

The Windows MS-MPI `mpiexec` needs to be accessible from MSYS2. We'll add it to the PATH.

**Add Windows MS-MPI to MSYS2 PATH:**
```bash
# In MSYS2 MinGW64 shell
nano ~/.bashrc

# Add this line at the end (adds Windows Program Files to PATH):
export PATH="$PATH:/c/Program Files/Microsoft MPI/Bin"

# Save (Ctrl+O, Enter, Ctrl+X)

# Reload the configuration
source ~/.bashrc
```

**Verify MS-MPI is now accessible:**
```bash
# In MSYS2 MinGW64 shell
which mpiexec
# Should show: /c/Program Files/Microsoft MPI/Bin/mpiexec

mpiexec --version
# Should show MS-MPI version info

# Test MPI execution
mpiexec -n 2 hostname
# Should show your hostname twice
```

**If mpiexec still not found, try alternative path:**
```bash
# Some MS-MPI versions install to a different location
# Try this instead in ~/.bashrc:
export PATH="$PATH:/c/Program Files/Microsoft MPI/Bin:/c/Program Files (x86)/Microsoft SDKs/MPI/Bin"

source ~/.bashrc
which mpiexec
```

### 2.4 Set Up MSYS2 MinGW64 Environment

**Verify compiler installation:**
```bash
# In MSYS2 MinGW64 shell
gcc --version
g++ --version
gfortran --version
make --version
```

**Check for MPI compiler wrappers:**
```bash
# These should now be available from the mingw-w64-x86_64-msmpi package
which mpicc
which mpic++
which mpif90

# If found, test them:
mpicc --version
mpic++ --version
```

**⚠️ If MPI compiler wrappers are NOT found:**

This is actually OK! We can use the regular gcc/g++ compilers and link against MS-MPI libraries manually in the LAMMPS Makefile. The mingw-w64-x86_64-msmpi package provides the necessary headers and libraries.

**Set up environment variables in MSYS2 (if not already done):**
```bash
# Edit ~/.bashrc in MSYS2 MinGW64 (should already have MPI path from 2.3)
nano ~/.bashrc

# Verify these lines are present:
# export PATH="$PATH:/c/Program Files/Microsoft MPI/Bin"
# export PATH="/mingw64/bin:$PATH"

# If missing, add them and reload:
source ~/.bashrc
```

### 2.5 Create Working Directory Structure

**In MSYS2 MinGW64 shell:**
```bash
# Create directories in Windows home
cd ~
mkdir -p lammps_build
cd lammps_build

# Or in Windows Documents folder
cd /c/Users/$USER/Documents
mkdir -p lammps_build
cd lammps_build
```

### 2.6 Verify System Readiness

**Run these checks in MSYS2 MinGW64:**
```bash
# Check all critical tools
gcc --version          # Should show MinGW64 GCC (8.x or later)
make --version         # Should show GNU Make
git --version          # Should work
mpiexec --version      # Should show MS-MPI version

# Check system info
uname -a               # Should show MINGW64

# Test MPI execution
mpiexec -n 2 hostname  # Should execute on 2 processes and show hostname twice
```

**Expected output:**
```bash
$ gcc --version
gcc.exe (Rev...) 13.x.x or similar

$ mpiexec --version
Microsoft MPI Startup Program [Version 10.x.x.x]

$ mpiexec -n 2 hostname
ADAMLabFive  (or your computer name, twice)
ADAMLabFive
```

**✅ If all checks pass, you're ready for Phase 3!**

**⚠️ Common Issues from Previous Attempt:**
- Using Git bash 'tr' command instead of MSYS2's → **Fixed by using MSYS2 exclusively**
- Mixing sh.exe from different sources → **Fixed by clean PATH in MSYS2**
- Wrong shell environment → **Fixed by using MinGW64 specifically**

**Phase 2 Complete! ✅**

---

## Phase 3: Download Source Code

### 3.1 Download LAMMPS (7 Aug 2019) - Pure Tarball Method

**⚠️ CRITICAL: Use tarball download, NOT git clone + checkout**
- **Why:** Previous attempt used `git clone` + `git checkout stable_7Aug2019`, which can mix versions
- **Solution:** Download the actual release tarball for a pure, unmodified 7Aug2019 source

**In MSYS2 MinGW64 shell:**
```bash
# Navigate to Documents
cd /c/Users/$USER/Documents

# Download the official 7Aug2019 tarball
wget https://download.lammps.org/tars/lammps-7Aug19.tar.gz

# Or use the alternative GitHub release:
wget https://github.com/lammps/lammps/archive/refs/tags/stable_7Aug2019.tar.gz

# Extract the archive
tar -xzvf lammps-7Aug19.tar.gz

# This creates: lammps-7Aug19 directory
cd lammps-7Aug19

# Verify this is the correct version
ls -la
cat README  # Check for version info
```

**Alternative: Windows Download (if wget has issues):**
1. Open browser and go to: https://download.lammps.org/tars/lammps-7Aug19.tar.gz
2. Save to Downloads folder
3. In MSYS2 MinGW64:
```bash
cd /c/Users/$USER/Documents
cp /c/Users/$USER/Downloads/lammps-7Aug19.tar.gz .
tar -xzvf lammps-7Aug19.tar.gz
cd lammps-7Aug19
```

### 3.2 Download EChemDID (22 Aug 2018) from LAMMPS-hacks

**⚠️ Note from Previous Attempt:** Original EChemDID repository no longer available
- **Solution:** Use LAMMPS-hacks-public repository (this is what Shweta used successfully)

**In MSYS2 MinGW64 shell:**
```bash
cd /c/Users/$USER/Documents

# Clone the LAMMPS-hacks-public repository
git clone https://github.com/nonoFrio/LAMMPS-hacks-public.git

# Verify the ECHEMDID-22Aug18 directory exists
cd LAMMPS-hacks-public
ls -la

# You should see: ECHEMDID-22Aug18 folder
cd ECHEMDID-22Aug18
ls -la
```

**Verify contents:**
```bash
# Check for key files
ls -la *.cpp
ls -la *.h

# Should see files like:
# - fix_echemdid.cpp
# - fix_echemdid.h
# - atom_vec_echemdid.cpp
# - etc.
```

### 3.3 Verify Downloads Checklist

```bash
# In /c/Users/$USER/Documents you should now have:
cd /c/Users/$USER/Documents
ls -la

# Should see:
# - lammps-7Aug19/              (pure 7Aug2019 source)
# - LAMMPS-hacks-public/        (contains ECHEMDID-22Aug18)
# - lammps-7Aug19.tar.gz        (original archive, can keep for backup)

# Verify LAMMPS source structure
cd lammps-7Aug19
ls -la
# Should see: src/, examples/, doc/, etc.

cd src
ls -la
# Should see: Makefile, *.cpp, *.h files, MAKE/ directory, etc.
```

**⚠️ KEY DIFFERENCE from Previous Attempt:**
- ✅ Using official tarball (not git checkout)
- ✅ No mixing with other LAMMPS versions (no reaxsrc, qeqsrc clones)
- ✅ Clean source that hasn't been manually edited

---

## Phase 4: Integrate EChemDID into LAMMPS

**⚠️ Important:** EChemDID is integrated as a USER package within LAMMPS, not built separately

### 4.1 Copy EChemDID to LAMMPS as USER-ECHEMDID

**In MSYS2 MinGW64 shell:**
```bash
cd /c/Users/$USER/Documents

# Copy ECHEMDID-22Aug18 into LAMMPS src as USER-ECHEMDID
cp -r LAMMPS-hacks-public/ECHEMDID-22Aug18 lammps-7Aug19/src/USER-ECHEMDID

# Verify the copy
cd lammps-7Aug19/src
ls -la | grep USER

# You should now see USER-ECHEMDID directory
cd USER-ECHEMDID
ls -la
```

**Alternative using Windows (if cp has issues):**
```cmd
# In Windows Command Prompt or PowerShell
cd %USERPROFILE%\Documents

xcopy /E /I "LAMMPS-hacks-public\ECHEMDID-22Aug18" "lammps-7Aug19\src\USER-ECHEMDID"
```

### 4.2 Verify USER-ECHEMDID Contents

```bash
# In MSYS2 MinGW64
cd /c/Users/$USER/Documents/lammps-7Aug19/src/USER-ECHEMDID

# Check for essential files
ls -la *.cpp *.h

# Should see files like:
# - fix_echemdid.cpp / .h
# - atom_vec_echemdid.cpp / .h
# - And other electrochemistry-related files

# Check if there's an Install.sh script
ls -la Install.sh
cat Install.sh  # If it exists, read it
```

### 4.3 Check for USER Package Dependencies

**EChemDID typically requires:**
- USER-REAXC (ReaxFF reactive force field)
- USER-QEQ (charge equilibration)

**⚠️ CRITICAL LESSON from Previous Attempt:**
- USER-QEQ does NOT exist as a separate package in 7Aug2019 LAMMPS
- QEQ functionality is built into core LAMMPS in 2019 (not a USER package)
- USER-REAXC should already be included in the 7Aug2019 tarball

**Verify what USER packages exist:**
```bash
cd /c/Users/$USER/Documents/lammps-7Aug19/src
ls -la | grep USER-

# Should see directories like:
# - USER-REAXC (or just REAXC)
# - Various other USER packages
# - USER-ECHEMDID (the one we just added)
```

### 4.4 Create Install.sh for USER-ECHEMDID (Required!)

**⚠️ IMPORTANT:** LAMMPS needs an Install.sh file to recognize USER-ECHEMDID as a package.

Since the LAMMPS-hacks version doesn't include one, we need to create it:

```bash
cd /c/Users/$USER/Documents/lammps-7Aug19/src/USER-ECHEMDID

# Create the Install.sh file
cat > Install.sh << 'EOF'
# Install/unInstall package files in LAMMPS
# mode = 0/1/2 for uninstall/install/update

mode=$1

# arg1 = file, arg2 = file it depends on

action () {
  if (test $mode = 0) then
    rm -f ../$1
  elif (! cmp -s $1 ../$1) then
    if (test -z "$2" || test -e ../$2) then
      cp $1 ..
      if (test $mode = 2) then
        echo "  updating src/$1"
      fi
    fi
  elif (test -n "$2") then
    if (test ! -e ../$2) then
      rm -f ../$1
    fi
  fi
}

# list of files with optional dependencies

action fix_echemdid.cpp
action fix_echemdid.h
action fix_qeq.cpp
action fix_qeq.h
action fix_qeq_shielded.cpp
action fix_qeq_shielded.h
EOF

# Make it executable
chmod +x Install.sh

# Verify it was created
ls -la Install.sh
cat Install.sh
```

**Verify Install.sh was created successfully:**
```bash
# Should show the Install.sh file
ls -la /c/Users/$USER/Documents/lammps-7Aug19/src/USER-ECHEMDID/Install.sh
```

**⚠️ DO NOT:**
- Clone separate repositories for REAXC or QEQ
- Copy REAXC from different LAMMPS versions
- Mix source code from multiple versions

---

## Phase 5: Build LAMMPS with MPI Support

### 5.1 Navigate to LAMMPS Source

**In MSYS2 MinGW64 shell:**
```bash
cd /c/Users/$USER/Documents/lammps-7Aug19
ls -la

# The src directory contains the source code
cd src
ls -la

# You should see:
# - Makefile
# - MAKE/ directory
# - Many .cpp and .h files
# - USER-* directories including USER-ECHEMDID
```

### 5.2 Check Available Packages

```bash
cd /c/Users/$USER/Documents/lammps-7Aug19/src

# View available packages
make package-status

# This shows which packages are available and their status (yes/no)
```

### 5.3 Install Required Packages (CRITICAL ORDER!)

**⚠️ IMPORTANT: USER-ECHEMDID is not recognized by LAMMPS package system**

Since USER-ECHEMDID is a custom package not in the official 7Aug2019 release, we need a different approach:

**Step 1: Install standard packages FIRST (without ECHEMDID):**
```bash
cd /c/Users/$USER/Documents/lammps-7Aug19/src

# Install REAXC (required for EChemDID)
make yes-user-reaxc

# Install other useful packages (but NOT MANYBODY yet)
make yes-molecule
make yes-kspace
make yes-misc

# Verify these are installed
make package-status | grep YES
```

**Step 2: Manually integrate EChemDID files:**
```bash
# Copy only the EChemDID-specific file (NOT the QEQ files)
cd /c/Users/$USER/Documents/lammps-7Aug19/src

# Copy the main EChemDID fix
cp USER-ECHEMDID/fix_echemdid.cpp .
cp USER-ECHEMDID/fix_echemdid.h .

# DO NOT copy fix_qeq* files - they conflict with LAMMPS's built-in QEQ
# The EChemDID package works with the standard LAMMPS QEQ implementation

# Verify files were copied
ls -la fix_echemdid.*
```

**Step 3: Install MANYBODY and QEQ packages:**
```bash
# Install MANYBODY
make yes-manybody

# Install QEQ explicitly (required for EChemDID)
make yes-qeq

# Verify both are installed
make package-status | grep -E "MANYBODY|QEQ"
# Should show:
# Installed YES: package MANYBODY
# Installed YES: package QEQ
```

**⚠️ Why this approach:**
- USER-ECHEMDID's QEQ files are custom modifications
- But they conflict with LAMMPS's standard QEQ in MANYBODY
- EChemDID's main functionality (fix_echemdid) requires fix_qeq.h
- We explicitly install QEQ to ensure fix_qeq.h is available
- This avoids the file conflicts from previous attempts

**Verification:**
```bash
# Check what's in src directory now
cd /c/Users/$USER/Documents/lammps-7Aug19/src
ls -la fix_echemdid.*
ls -la fix_qeq.h

# Should see:
# - fix_echemdid.cpp and .h (from USER-ECHEMDID)
# - fix_qeq.h (from QEQ package - required by fix_echemdid.cpp)

# Check package status
make package-status | grep YES
# Should show: KSPACE, MOLECULE, MISC, MANYBODY, QEQ, USER-REAXC = YES
```

### 5.3a Clean Up if You Already Ran Phase 5.3 Incorrectly

**If you already installed packages and have conflicts, reset:**
```bash
cd /c/Users/$USER/Documents/lammps-7Aug19/src

# Uninstall all packages
make no-all

# Verify all are uninstalled
make package-status | grep YES
# Should show nothing or very few

# Now follow Step 1, 2, 3 above in the correct order
```

### 5.4 Choose Build Configuration (MPI)

**In LAMMPS 7Aug2019, traditional make is used (not cmake):**
```bash
cd /c/Users/$USER/Documents/lammps-7Aug19/src/MAKE

# List available makefiles
ls -la

# You should see various Makefile.* options
# We need one compatible with MinGW64 + MS-MPI
```

**Available options (check which exist):**
- `Makefile.mpi` - generic MPI (may need modification)
- `Makefile.mingw64-cross` - MinGW64 cross-compile (close to what we need)
- `Makefile.serial` - serial only (not what we want)

**Check if mingw-specific makefile exists:**
```bash
ls -la Makefile.*mingw*
ls -la Makefile.mpi
```

### 5.5 Create Custom Makefile for MinGW64 + MS-MPI

**We need to create a custom makefile:**
```bash
cd /c/Users/$USER/Documents/lammps-7Aug19/src/MAKE

# Start from serial or mpi as a base
cp Makefile.mpi Makefile.mingw64_mpi

# Edit the makefile
nano Makefile.mingw64_mpi
```

**Key settings to configure in Makefile.mingw64_mpi:**
```makefile
# Compiler settings for MinGW64 with MS-MPI
SHELL = /bin/bash

# Compiler/linker (MinGW64 with MPI)
CC =		mpicc
CCFLAGS =	-g -O2 -march=native -mtune=native
SHFLAGS =	-fPIC
DEPFLAGS =	-M

LINK =		mpic++
LINKFLAGS =	-g -O2 -march=native -mtune=native
LIB =		
SIZE =		size

ARCHIVE =	ar
ARFLAGS =	-rc
SHLIBFLAGS =	-shared

# MPI library (should be auto-detected by mpicc)
MPI_INC =       
MPI_PATH =      
MPI_LIB =       

# FFT library (use SMALLBIG for no FFT or add FFTW if available)
FFT_INC =       -DFFT_FFTW3
FFT_PATH = 
FFT_LIB =	-lfftw3

# JPEG library (optional, comment out if not available)
#JPG_INC =       
#JPG_PATH = 	
#JPG_LIB =	-ljpeg

# PNG library (optional, comment out if not available)
#PNG_INC =       
#PNG_PATH = 	
#PNG_LIB =	-lpng
```

**Save the file (Ctrl+O, Enter, Ctrl+X in nano)**

### 5.6 Build lmp_mingw64_mpi

```bash
cd /c/Users/$USER/Documents/lammps-7Aug19/src

# Clean any previous build attempts
make clean-all

# Build with our custom makefile
make mingw64_mpi -j4

# The -j4 uses 4 cores for parallel compilation (adjust based on your CPU)
# This will take several minutes
```

**⚠️ EXPECTED: Build will fail with `posix_memalign` errors on Windows**
- This is NORMAL - Windows lacks POSIX memory alignment functions
- Proceed to Phase 5.7 to apply Windows compatibility fixes

### 5.7 Apply Windows Compatibility Fixes

**Critical:** Windows MinGW64 does not provide `posix_memalign`. We must use `_aligned_malloc` and `_aligned_free` instead.

#### 5.7.1 Fix my_pool_chunk.h

```bash
cd /c/Users/$USER/Documents/lammps-7Aug19/src
nano my_pool_chunk.h
```

**Find the `allocate()` function (around line 196-213)** and replace the `posix_memalign` section with:

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
      pages[i] = (T *) malloc(chunkperpage*chunksize[ibin]*sizeof(T));
      size += chunkperpage*chunksize[ibin];
      if (!pages[i]) errorflag = 2;
#endif
```

**Find the destructor `~MyPoolChunk()` (around line 102-112)** and update the free loop:

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

Save and exit (Ctrl+O, Enter, Ctrl+X).

#### 5.7.2 Fix my_page.h

```bash
nano my_page.h
```

**Find the `allocate()` function (around line 221-235)** and replace with:

```cpp
void allocate() {
  npage += pagedelta;
  // Management array doesn't need alignment - use regular realloc
  pages = (T **) realloc(pages,npage*sizeof(T *));
  if (!pages) {
    errorflag = 2;
    return;
  }

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
    pages[i] = (T *) malloc(pagesize*sizeof(T));
    if (!pages[i]) errorflag = 2;
#endif
  }
}
```

**Find the destructor `~MyPage()` (around line 90)** and update:

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

**Find the `init()` function's free section (around line 84)** and update:

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

Save and exit.

#### 5.7.3 Fix memory.cpp

```bash
nano memory.cpp
```

**Find the `smalloc()` function (around line 42-64)** and update:

```cpp
void *Memory::smalloc(bigint nbytes, const char *name)
{
  if (nbytes == 0) return NULL;

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

#else
  void *ptr = malloc(nbytes);
#endif
  if (ptr == NULL) {
    char str[128];
    sprintf(str,"Failed to allocate " BIGINT_FORMAT " bytes for array %s",
            nbytes,name);
    error->one(FLERR,str);
  }
  return ptr;
}
```

**Find the `srealloc()` function (around line 75-105)** and update:

```cpp
void *Memory::srealloc(void *ptr, bigint nbytes, const char *name)
{
  if (nbytes == 0) {
    destroy(ptr);
    return NULL;
  }

#if defined(LMP_USE_TBB_ALLOCATOR)
  ptr = scalable_aligned_realloc(ptr, nbytes, LAMMPS_MEMALIGN);
#elif defined(LMP_INTEL_NO_TBB) && defined(LAMMPS_MEMALIGN) && \
      defined(__INTEL_COMPILER)

  ptr = realloc(ptr, nbytes);
  uintptr_t offset = ((uintptr_t)(const void *)(ptr)) % LAMMPS_MEMALIGN;
  if (offset) {
    void *optr = ptr;
    ptr = smalloc(nbytes, name);
    memcpy(ptr, optr, MIN(nbytes,malloc_usable_size(optr)));
    free(optr);
  }
#elif defined(LAMMPS_MEMALIGN) && (defined(_WIN32) || defined(_WIN64))
  // Windows: use _aligned_realloc for memory allocated with _aligned_malloc
  ptr = _aligned_realloc(ptr, nbytes, LAMMPS_MEMALIGN);
#else
  ptr = realloc(ptr,nbytes);
#endif
  if (ptr == NULL) {
    char str[128];
    sprintf(str,"Failed to reallocate " BIGINT_FORMAT " bytes for array %s",
            nbytes,name);
    error->one(FLERR,str);
  }
  return ptr;
}
```

**Find the `sfree()` function (around line 110-120)** and update:

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

Save and exit.

### 5.8 Rebuild with Windows Fixes

```bash
cd /c/Users/$USER/Documents/lammps-7Aug19/src

# Clean completely
make clean-all

# Rebuild
make mingw64_mpi -j4
```

**Expected output:** Build should now complete successfully and create `lmp_mingw64_mpi.exe`

### 5.9 Verify Build

```bash
cd /c/Users/$USER/Documents/lammps-7Aug19/src

# Check if executable was created
ls -lh lmp_mingw64_mpi

# Test the executable
./lmp_mingw64_mpi -h

# Test with MPI
mpiexec -n 2 ./lmp_mingw64_mpi -h
```

**⚠️ Expected Issues from Previous Attempt:**
1. **posix_memalign error** - Windows doesn't have this POSIX function
2. **Memory allocation compatibility** - MinGW64 vs POSIX differences

**We'll address these in Phase 5.9 if they occur**

---

## Phase 5.9: Troubleshooting Build Errors (Apply Only If Needed)

**⚠️ ONLY follow this section if Phase 5.6 fails with specific errors**

### Issue 1: posix_memalign not found (my_pool_chunk.h and my_page.h)

**Error message:**
```
error: 'posix_memalign' was not declared in this scope
```

This error will appear in two files: `my_pool_chunk.h` and `my_page.h`

**Fix for my_pool_chunk.h:**
```bash
cd /c/Users/$USER/Documents/lammps-7Aug19/src
cp my_pool_chunk.h my_pool_chunk.h.backup

nano my_pool_chunk.h
```

**Find this section (around line 196-208):**
```cpp
    for (int i = oldpage; i < npage; i++) {
      whichbin[i] = ibin;
#if defined(LAMMPS_MEMALIGN)
      void *ptr;
      if (posix_memalign(&ptr, LAMMPS_MEMALIGN,
                         chunkperpage*chunksize[ibin]*sizeof(T)))
        errorflag = 2;
      pages[i] = (T *) ptr;
#else
```

**Replace with:**
```cpp
    for (int i = oldpage; i < npage; i++) {
      whichbin[i] = ibin;
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

**Fix for my_page.h:**
```bash
cd /c/Users/$USER/Documents/lammps-7Aug19/src
cp my_page.h my_page.h.backup

nano my_page.h
```

**Find this section (around line 221-230):**
```cpp
    for (int i = npage-pagedelta; i < npage; i++) {
#if defined(LAMMPS_MEMALIGN)
      void *ptr;
      if (posix_memalign(&ptr, LAMMPS_MEMALIGN, pagesize*sizeof(T)))
        errorflag = 2;
      pages[i] = (T *) ptr;
#else
```

**Replace with:**
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

**After fixing both files, retry build:**
```bash
make clean-all
make mingw64_mpi -j4
```---

## Phase 6: Installation and Setup

### 6.1 Verify Successful Build

```bash
cd /c/Users/$USER/Documents/lammps-7Aug19/src

# Check if executable exists
ls -lh lmp_mingw64_mpi.exe

# File should be several MB in size (typically 15-25 MB)
file lmp_mingw64_mpi.exe  # Should show: PE32+ executable

# Test basic execution
./lmp_mingw64_mpi.exe -h

# Should display LAMMPS help and version info
```

### 6.2 Test MPI Functionality

```bash
# Test with 2 processes
mpiexec -n 2 ./lmp_mingw64_mpi.exe -h

# Test with 4 processes  
mpiexec -n 4 ./lmp_mingw64_mpi.exe -h

# Both should succeed without errors and show LAMMPS help
```

### 6.3 Add Executable to PATH (Recommended)

**Option 1: Add to Windows PATH (Permanent)**
```cmd
# In Windows Command Prompt (as Administrator)
setx PATH "%PATH%;%USERPROFILE%\Documents\lammps-7Aug19\src" /M

# Restart MSYS2 MinGW64 terminal after this
```

**Option 2: Add to MSYS2 PATH (MSYS2 only)**
```bash
# In MSYS2 MinGW64
nano ~/.bashrc

# Add these lines at the end:
export LAMMPS_HOME="/c/Users/$USER/Documents/lammps-7Aug19"
export PATH="$PATH:$LAMMPS_HOME/src"

# Save and reload
source ~/.bashrc

# Test
lmp_mingw64_mpi.exe -h
```

**Note:** We use `lmp_mingw64_mpi.exe` directly (no alias needed)

### 6.4 Verify Installation
source ~/.bashrc

# Verify
echo $LAMMPS_HOME
```

### 6.5 Final Verification

```bash
# Test LAMMPS command availability
which lmp_mpi
# or
which lmp_mingw64_mpi

# Test execution
lmp_mingw64_mpi.exe -h

# Test MPI with different processor counts
mpiexec -n 1 lmp_mingw64_mpi.exe -h
### 6.4 Verify Installation

```bash
# Check executable location
which lmp_mingw64_mpi.exe

# Test basic help
lmp_mingw64_mpi.exe -h

# Test MPI execution
mpiexec -n 2 lmp_mingw64_mpi.exe -h
mpiexec -n 4 lmp_mingw64_mpi.exe -h

# Check version info
lmp_mingw64_mpi.exe -h | head -n 10
# Should show: LAMMPS (7 Aug 2019)
```

---

## Phase 7: Testing and Validation

### 7.1 Navigate to Examples Directory

```bash
# In MSYS2 MinGW64
cd /c/Users/$USER/Documents/lammps-7Aug19/examples

# List available examples
ls -la

# You should see many example directories like:
# - melt/
# - crack/
# - friction/
# - etc.
```

### 7.2 Run Basic LAMMPS Example (Serial - Single Process)

```bash
# Test with a simple melt example first
cd /c/Users/$USER/Documents/lammps-7Aug19/examples/melt

# List files
ls -la

# Run in single process mode first
lmp_mingw64_mpi.exe -in in.melt

# Watch for output - should show:
# - Initialization
# - Timestep progress
# - Performance statistics
# - "Loop time of X on 1 procs"
# - NO errors
```

**Expected output:**
```
LAMMPS (7 Aug 2019)
Lattice spacing in x,y,z = 1.6796 1.6796 1.6796
Created orthogonal box = (0 0 0) to (16.796 16.796 16.796)
  1 by 1 by 1 MPI processor grid
Created 4000 atoms
...
Setting up Verlet run ...
...
Step Temp E_pair E_mol TotEng Press
   0    3   -6.7733681    0   -2.2744931   -3.7033504
  50    1.6758903   -4.7955425    0   -2.2823355     5.670064
...
Loop time of 0.322 on 1 procs for 250 steps with 4000 atoms
```

### 7.3 Run LAMMPS Example with MPI (Parallel)

```bash
# Still in examples/melt directory
cd /c/Users/$USER/Documents/lammps-7Aug19/examples/melt

# Run with 2 processes
mpiexec -n 2 lmp_mingw64_mpi.exe -in in.melt

# Run with 4 processes
mpiexec -n 4 lmp_mingw64_mpi.exe -in in.melt

# Check exit status
echo $?  # Should return 0 for success

# Verify log file was created
ls -la log.lammps
cat log.lammps  # Should show successful completion
```

**Expected output for MPI run:**
```
LAMMPS (7 Aug 2019)
...
  2 by 1 by 1 MPI processor grid  # (for -n 2)
...
Loop time of X on 2 procs for 250 steps with 4000 atoms
```

**Success indicators:**
- No "job aborted" or "crashed" messages
- No heap corruption errors (0xc0000374)
- Creates output files (log.lammps, dump.melt if specified)
- Exit code is 0

### 7.4 Test EChemDID Functionality

**First, check if fix echemdid is available:**
```bash
lmp_mingw64_mpi.exe -h | grep -i echemdid

# Look for example inputs or test cases
find . -name "*.in" -o -name "*.lmp"
find . -name "*example*" -type d
```

**If EChemDID examples exist:**
```bash
# Navigate to example directory
cd [path-to-echemdid-examples]

# Run test case
mpiexec -n 4 lmp_mingw64_mpi.exe -in [input-file.in]
```

**If no examples, create simple test:**
```bash
# Test that ECHEMDID fixes are available
lmp_mingw64_mpi.exe -h | grep -i echemdid
# or
lmp_mingw64_mpi.exe -h > lammps_help.txt
grep -i echemdid lammps_help.txt
grep -i "fix.*echem" lammps_help.txt
```

### 7.5 Performance Testing

```bash
cd /c/Users/$USER/Documents/lammps-7Aug19/examples/melt

# Test scaling with different processor counts
echo "Testing with 1 processor:"
time mpiexec -n 1 lmp_mingw64_mpi.exe -in in.melt

echo "Testing with 2 processors:"
time mpiexec -n 2 lmp_mingw64_mpi.exe -in in.melt

echo "Testing with 4 processors:"
time mpiexec -n 4 lmp_mingw64_mpi.exe -in in.melt

# Compare "Total wall time" in the output
# Should see speedup with more processors
```

### 7.6 Verify Output Files

```bash
# After running examples, check for output files
cd /c/Users/$USER/Documents/lammps-7Aug19/examples/melt

ls -la

# Should see new files:
# - log.lammps (log file)
# - Any dump files specified in input script

# Check log file
tail -n 50 log.lammps
# Should show completion statistics, no errors

# Check for any error messages
grep -i error log.lammps
grep -i warning log.lammps
```

### 7.7 Document Test Results

**Create a test results file:**
```bash
cd /c/Users/$USER/Documents

# Create test log
cat > lammps_test_results.txt << 'EOF'
LAMMPS Testing Results
======================
Date: [fill in]
Version: 7 Aug 2019
Build: mingw64_mpi

Test 1: Serial execution
- Example: melt
- Status: [PASS/FAIL]
- Notes: 

Test 2: MPI execution (n=2)
- Example: melt
- Status: [PASS/FAIL]
- Notes:

Test 3: MPI execution (n=4)
- Example: melt
- Status: [PASS/FAIL]
- Notes:

Test 4: EChemDID functionality
- Status: [PASS/FAIL/NOT TESTED]
- Notes:

Performance:
- 1 proc: [time]
- 2 proc: [time] (speedup: [X]x)
- 4 proc: [time] (speedup: [X]x)

Issues encountered:
[List any problems]

EOF

# Edit to fill in results
nano lammps_test_results.txt
```

---

## Phase 8: Troubleshooting Guide

### 8.1 Common Build Errors

**Error: MPI compiler not found (mpicc, mpic++)**
```bash
# In MSYS2 MinGW64
# Verify MS-MPI package is installed
pacman -Qs msmpi

# If not installed:
pacman -S mingw-w64-x86_64-msmpi

# Verify MPI tools
which mpicc
which mpic++
which mpiexec

# If still not found, check Windows MPI installation:
# In Windows Command Prompt:
mpiexec --version
dir "C:\Program Files (x86)\Microsoft SDKs\MPI\Include\mpi.h"
```

**Error: FFTW library not found**
```bash
# Option 1: Install FFTW in MSYS2
pacman -S mingw-w64-x86_64-fftw

# Option 2: Disable FFTW in Makefile
cd /c/Users/$USER/Documents/lammps-7Aug19/src/MAKE
nano Makefile.mingw64_mpi

# Change FFT settings to:
FFT_INC =       -DFFT_NONE
FFT_PATH = 
FFT_LIB =
```

**Error: Package installation fails**
```bash
cd /c/Users/$USER/Documents/lammps-7Aug19/src

# Check package status
make package-status

# Remove problematic package
make no-[package-name]

# For example:
make no-user-reaxc

# Try to reinstall
make yes-user-reaxc

# Rebuild
make clean-all
make mingw64_mpi -j4
```

**Error: posix_memalign undefined**
- See Phase 5.9 for the fix
- This is expected on Windows
- Apply minimal conditional compilation fix

**Error: Undefined references to utils:: or fmt::**
```bash
# This indicates version mixing (from previous attempt)
# Solution: Start over with clean tarball download
# DO NOT copy files from different LAMMPS versions
```

### 8.2 Runtime Errors

**Error: "lmp_mpi: command not found"**
```bash
# In MSYS2 MinGW64
# Check if executable exists
ls -la /c/Users/$USER/Documents/lammps-7Aug19/src/lmp_mingw64_mpi

# Use full path
/c/Users/$USER/Documents/lammps-7Aug19/src/lmp_mingw64_mpi -h

# Or set up alias (see Phase 6.3)
```

**Error: Executable won't run / crashes immediately**
```bash
# Check if it's actually a Windows executable
file /c/Users/$USER/Documents/lammps-7Aug19/src/lmp_mingw64_mpi
# Should show: PE32+ executable

# Try running from Command Prompt instead
# Open Windows Command Prompt:
cd %USERPROFILE%\Documents\lammps-7Aug19\src
lmp_mingw64_mpi.exe -h

# Check for missing DLLs
ldd lmp_mingw64_mpi | grep "not found"

# If DLLs are missing, make sure MinGW64 bin is in PATH:
export PATH="/mingw64/bin:$PATH"
```

**Error: MPI execution fails**
```bash
# Test MPI separately
mpiexec -n 2 hostname

# If this fails, MPI installation issue
# Reinstall MS-MPI in Windows:
# Download and run: msmpisetup.exe and msmpisdk.msi

# In MSYS2, reinstall:
pacman -S mingw-w64-x86_64-msmpi

# Check firewall settings (Windows Security)
# Allow mpiexec.exe through firewall
```

**Error: "Could not open file" during simulation**
```bash
# Check file paths - use Unix-style paths in MSYS2
# Windows: C:\Users\Name\file.in
# MSYS2:   /c/Users/Name/file.in

# Make sure input files exist
ls -la in.melt
cat in.melt  # Check for correct paths inside
```

**Error: Simulation runs but produces no output**
```bash
# Check log file
cat log.lammps

# Check for permission issues
ls -la
chmod 644 log.lammps

# Make sure dump/write commands in input script have correct paths
```

### 8.3 Issues from Previous Attempt - What Went Wrong

**1. Version Mixing:**
- ❌ Used git clone + checkout (can pull in newer dependencies)
- ❌ Copied REAXC from different version (incompatible)
- ❌ Copied QEQ from newer version (different structure)
- ✅ Solution: Use pure 7Aug2019 tarball only

**2. Manual Source Edits:**
- ❌ Edited multiple core files (my_pool_chunk.h, memory.cpp, compute_reaxff_atom.cpp)
- ❌ Commented out utils:: and fmt:: calls (broke functionality)
- ✅ Solution: Minimal edits with conditional compilation only

**3. Wrong Build Environment:**
- ❌ Mixed Git bash tools with MSYS2
- ❌ Used 'make serial' instead of MPI makefile
- ✅ Solution: Clean MSYS2 MinGW64 environment only

**4. USER-QEQ Package:**
- ❌ Tried to add USER-QEQ as separate package
- ❌ It's built-in to 2019 core LAMMPS
- ✅ Solution: Don't add QEQ package

**5. Executable Not Working:**
- ❌ Built with mixed sources and manual edits
- ❌ Possible missing DLLs or wrong architecture
- ✅ Solution: Clean build with proper makefile

### 8.4 Getting Help

**LAMMPS Resources:**
```bash
# Local documentation
cd /c/Users/$USER/Documents/lammps-7Aug19/doc

# Online resources:
# - https://docs.lammps.org/Manual.html (general docs)
# - https://docs.lammps.org/Build.html (build instructions)
# - https://lammps.sandia.gov/mail.html (mailing list)
```

**Windows-Specific Help:**
- LAMMPS GitHub issues: Search for "windows" or "mingw"
- MSYS2 documentation: https://www.msys2.org/docs/
- MS-MPI documentation: Microsoft MPI documentation

**EChemDID Resources:**
- LAMMPS-hacks-public repository
- Original papers/documentation
- Contact Shweta or research group

---

## Phase 9: Documentation and Notes

### 9.1 System Information

**Record your system details:**
```bash
# In MSYS2 MinGW64
# Operating system
uname -a
echo $MSYSTEM

# In Windows Command Prompt
systeminfo | findstr /B /C:"OS Name" /C:"OS Version"
echo %PROCESSOR_ARCHITECTURE%

# CPU information
wmic cpu get name

# Memory
wmic memorychip get capacity

# GCC version (in MSYS2)
gcc --version
g++ --version

# MPI version
mpiexec --version
```

**Create system documentation file:**
```bash
cd /c/Users/$USER/Documents

cat > lammps_system_info.txt << 'EOF'
ASUS TUF Dash F15 - LAMMPS Build Configuration
===============================================

Hardware:
- CPU: [copy from wmic cpu get name]
- RAM: [copy from wmic memorychip]
- OS: Windows 11

Software Versions:
- LAMMPS: 7 Aug 2019
- EChemDID: 22 Aug 2018 (from LAMMPS-hacks-public)
- Build Environment: MSYS2 MinGW64
- Compiler: GCC [version from gcc --version]
- MPI Implementation: MS-MPI [version from mpiexec --version]

Build Details:
- Build Date: [fill in]
- Makefile: Makefile.mingw64_mpi
- Executable: lmp_mingw64_mpi
- Build Directory: C:\Users\[USER]\Documents\lammps-7Aug19

LAMMPS Packages Installed:
- USER-REAXC: YES
- USER-ECHEMDID: YES
- MOLECULE: YES
- KSPACE: YES
- MANYBODY: YES
- MISC: YES

Source Modifications:
[List any files that were modified, if any]

Known Issues:
[Document any issues encountered]

EOF

nano lammps_system_info.txt
```

### 9.2 Build Configuration Checklist

**Verify and document:**
```bash
# In MSYS2 MinGW64
cd /c/Users/$USER/Documents/lammps-7Aug19/src

# Check installed packages
make package-status > installed_packages.txt
cat installed_packages.txt

# Check makefile used
cat MAKE/Makefile.mingw64_mpi > build_makefile_backup.txt

# Document executable
ls -lh lmp_mingw64_mpi
file lmp_mingw64_mpi
lmp_mingw64_mpi -h | head -n 20 > lammps_version_info.txt
```

### 9.3 Performance Benchmarks

**Create benchmark documentation:**
```bash
cd /c/Users/$USER/Documents

cat > lammps_benchmarks.txt << 'EOF'
LAMMPS Performance Benchmarks
==============================
Date: [fill in]
System: ASUS TUF Dash F15
Test Case: examples/melt

Single Run Results:
-------------------
1 processor:  [time] seconds
2 processors: [time] seconds (speedup: [X]x)
4 processors: [time] seconds (speedup: [X]x)
8 processors: [time] seconds (speedup: [X]x)

Average of 3 Runs:
------------------
[Fill in after multiple test runs]

Scaling Efficiency:
-------------------
[Calculate: speedup / number of processors]

Notes:
------
[Any observations about performance]

EOF

nano lammps_benchmarks.txt
```

### 9.4 Create Backup of Working Configuration

**After successful build and testing:**
```bash
# In MSYS2 MinGW64
cd /c/Users/$USER/Documents

# Create backup directory
mkdir -p lammps_backup_$(date +%Y%m%d)

# Copy executable
cp lammps-7Aug19/src/lmp_mingw64_mpi lammps_backup_$(date +%Y%m%d)/

# Copy makefile
cp lammps-7Aug19/src/MAKE/Makefile.mingw64_mpi lammps_backup_$(date +%Y%m%d)/

# Copy any modified source files (if any)
# Example:
# cp lammps-7Aug19/src/my_pool_chunk.h lammps_backup_$(date +%Y%m%d)/

# Create README
cat > lammps_backup_$(date +%Y%m%d)/README.txt << 'EOF'
This backup contains the working LAMMPS 7Aug2019 build
Created: [date]
- lmp_mingw64_mpi: Working executable
- Makefile.mingw64_mpi: Build configuration used
- [List any other files]

To restore: Copy files back to lammps-7Aug19/src/
EOF
```

### 9.5 Document Common Usage Patterns

**Create quick reference:**
```bash
cd /c/Users/$USER/Documents

cat > lammps_quick_reference.txt << 'EOF'
LAMMPS Quick Reference - ASUS Setup
====================================

Starting MSYS2 MinGW64:
- Start Menu → "MSYS2 MinGW64" (blue icon)

Running LAMMPS:
---------------
# Serial (single processor)
lmp_mingw64_mpi.exe -in input.in

# Parallel (4 processors)
mpiexec -n 4 lmp_mingw64_mpi.exe -in input.in

# With log file
mpiexec -n 4 lmp_mingw64_mpi.exe -in input.in -log my_log.lammps

# With screen output
mpiexec -n 4 lmp_mingw64_mpi.exe -in input.in -screen output.txt

Common File Locations:
---------------------
LAMMPS Source: /c/Users/$USER/Documents/lammps-7Aug19
Executable: /c/Users/$USER/Documents/lammps-7Aug19/src/lmp_mingw64_mpi
Examples: /c/Users/$USER/Documents/lammps-7Aug19/examples
EChemDID: /c/Users/$USER/Documents/lammps-7Aug19/src/USER-ECHEMDID

Checking Simulation Progress:
-----------------------------
# View log file
tail -f log.lammps

# Check running processes
ps aux | grep lmp_mpi

# Monitor CPU usage
top

Troubleshooting:
----------------
# Test MPI
mpiexec -n 2 hostname

# Test LAMMPS
lmp_mingw64_mpi.exe -h

# Check for errors
grep -i error log.lammps

EOF
```

---

## Quick Reference Commands

### Starting MSYS2 MinGW64
```
Windows Start Menu → "MSYS2 MinGW64" (blue icon, NOT the purple MSYS icon)
```

### Basic LAMMPS Execution
```bash
# Serial execution
lmp_mingw64_mpi.exe -in input_file.in

# Parallel (4 processors)
mpiexec -n 4 lmp_mingw64_mpi.exe -in input_file.in

# Parallel with log redirection
mpiexec -n 4 lmp_mingw64_mpi.exe -in input_file.in -log my_simulation.log

# Parallel with both log and screen output
mpiexec -n 4 lmp_mingw64_mpi.exe -in input_file.in -log file.log -screen screen.txt
```

### Check LAMMPS Version and Installed Features
```bash
# Version info
lmp_mingw64_mpi.exe -h | head -n 10

# List all available commands/fixes/computes
lmp_mingw64_mpi.exe -h > lammps_features.txt
grep "fix.*echem" lammps_features.txt
```

### Monitor Simulation Progress
```bash
# Watch log file in real-time
tail -f log.lammps

# Check for running LAMMPS processes
ps aux | grep lmp_mpi

# Monitor CPU usage
top
```

### File Path Conversion (Windows ↔ MSYS2)
```bash
# Windows path: C:\Users\Name\Documents\file.in
# MSYS2 path:   /c/Users/Name/Documents/file.in

# Convert automatically:
cd $(cygpath -u "C:\Users\Name\Documents")
```

---

## Important Notes & Lessons Learned

### Critical Success Factors

1. **Use Pure Source**: Download official 7Aug2019 tarball, NOT git clone + checkout
2. **No Version Mixing**: Never mix source files from different LAMMPS versions
3. **Minimal Edits**: Only edit if absolutely necessary, use conditional compilation
4. **Right Environment**: Always use MSYS2 MinGW64, never mix with Git bash tools
5. **QEQ is Built-in**: USER-QEQ is part of 2019 core, don't add it as a package

### Before Starting Work

1. **Backup Configuration**: After successful setup, backup the executable and makefiles
2. **Document Everything**: Record versions, modifications, and issues encountered
3. **Test Incrementally**: Test with simple examples before complex simulations
4. **Keep Notes**: Update this guide with any new findings

### When Things Go Wrong

1. **Don't Panic-Edit**: Don't start randomly editing source files
2. **Check Logs**: Always read log.lammps for actual error messages
3. **Test MPI Separately**: Run `mpiexec -n 2 hostname` to isolate MPI issues
4. **Start Fresh**: If too many changes, start over with clean source

---

## Status Log

| Date | Phase | Status | Notes |
|------|-------|--------|-------|
| 2025-10-24 | Guide Created | ✅ Complete | Initial comprehensive guide |
| | Cleanup | 🔲 Not Started | Remove previous installations |
| | MSYS2 Setup | 🔲 Not Started | Install/configure build environment |
| | Download Sources | 🔲 Not Started | Get LAMMPS 7Aug19 + EChemDID |
| | Integrate EChemDID | 🔲 Not Started | Copy to USER-ECHEMDID |
| | Configure Build | 🔲 Not Started | Create Makefile.mingw64_mpi |
| | Compile LAMMPS | 🔲 Not Started | Build lmp_mingw64_mpi |
| | Test Basic | 🔲 Not Started | Run examples/melt |
| | Test MPI | 🔲 Not Started | Test parallel execution |
| | Validate EChemDID | 🔲 Not Started | Verify EChemDID features |
| | Documentation | 🔲 Not Started | Record system info & benchmarks |

**Legend:**
- ✅ Complete
- 🔄 In Progress  
- 🔲 Not Started
- ⚠️ Issue/Blocked
- ❌ Failed

---

## Key Differences from Previous Attempt

| Previous Attempt | This Guide |
|-----------------|------------|
| Used `git clone` + `git checkout` | Use official tarball download |
| Copied REAXC from different version | Use included REAXC from 7Aug19 |
| Added USER-QEQ as package | Recognize QEQ is built-in |
| Manually edited many source files | Minimal edits with #ifdef |
| Mixed Git bash and MSYS2 | Exclusive MSYS2 MinGW64 use |
| Built with `make serial` | Build with `make mingw64_mpi` |
| Executable didn't work | Proper build → working executable |

---

## Additional Resources

**LAMMPS:**
- Official Documentation: https://docs.lammps.org
- GitHub Repository: https://github.com/lammps/lammps
- Download Archives: https://download.lammps.org/tars/
- Mailing List: https://lammps.sandia.gov/mail.html

**Build Environment:**
- MSYS2: https://www.msys2.org
- MS-MPI: https://docs.microsoft.com/en-us/message-passing-interface/microsoft-mpi
- MinGW-w64: https://www.mingw-w64.org

**EChemDID:**
- LAMMPS-hacks-public: https://github.com/nonoFrio/LAMMPS-hacks-public
- Related papers and documentation from research group

**Windows Building:**
- LAMMPS Windows Build Notes: https://docs.lammps.org/Build_windows.html
- GitHub Issues tagged "windows": Search LAMMPS issues for Windows-specific problems

---

## Contact & Support

**For LAMMPS Issues:**
- LAMMPS Mailing List: lammps-users@lists.sourceforge.net
- GitHub Issues: https://github.com/lammps/lammps/issues

**For EChemDID Questions:**
- Contact Shweta or research group
- Check LAMMPS-hacks-public repository issues

**Internal Notes:**
- Previous work documented in: Previous_work.md
- This guide: ASUS_LAMMPS_Setup_Guide.md

---

**Last Updated:** October 24, 2025

**Next Steps:**
1. Read through entire guide
2. Start with Phase 1 (Cleanup)
3. Work through phases sequentially
4. Document results in Status Log
5. Report any issues or deviations from expected behavior
