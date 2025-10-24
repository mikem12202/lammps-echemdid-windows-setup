On the Asus:
Downloaded Visual Studio Community Edition
Launch the installer → select the workload
✅ “Desktop development with C++”
Under “Installation details,” ticked (only!):
“MSVC v143 – VS 2022 C++ x64/x86 build tools”
“C++ CMake tools for Windows”
“Windows 11 SDK”
Downloaded (from https://github.com/microsoft/Microsoft-MPI/releases) and installed in order:
Msmpisetup.exe
Msmpisdk.msi
Verified installation with: mpiexec -n 2 hostname
dir "C:\Program Files (x86)\Microsoft SDKs\MPI\Include\mpi.h"
Did not add mpiexec to path permanently, maybe will do later
Verified cmake: cmake --version
Getting LAMMPS (7 Aug 2019):
git clone https://github.com/lammps/lammps.git lammps-2019
Oh no, we have to install git
Installed from git-scm.com/install/windows
Used mostly default settings
Working
Executed git clone
cd lammps-2019
git checkout stable_7Aug2019
Getting EChemDID (22 Aug 2018)
cd %USERPROFILE%\Documents
Unfortunately, it looks like the original older EChemDID is no longer available.  Going back to Lammps-hacks (this is what Shweta did anyways)
cd %USERPROFILE%\Documents
git clone https://github.com/nonofrio/LAMMPS-hacks-public.git
xcopy /E /I "%USERPROFILE%\Documents\LAMMPS-hacks-public\ECHEMDID-22Aug18" "%USERPROFILE%\Documents\lammps-2019\src\USER-ECHEMDID"
USER-REAXC and USER-QEQ (best-guess 2021)
cd %USERPROFILE%\Documents
git clone https://github.com/lammps/lammps.git reaxsrc
cd reaxsrc
xcopy /E /I "%USERPROFILE%\Documents\reaxsrc\src\REAXFF" "%USERPROFILE%\Documents\lammps-2019\src\USER-REAXC"
xcopy /E /I "%USERPROFILE%\Documents\reaxsrc\src\QEQ" "%USERPROFILE%\Documents\lammps-2019\src\USER-QEQ"
Note the naming convention switch - to the old USER convention
On to setting up MAKE project (not using CMAKE since its 2019 lammps)
cd %USERPROFILE%\Documents\lammps-2019\src
Turns out windows doesn’t have make.  Getting Chocolatey so we can get make.
Set-ExecutionPolicy Bypass -Scope Process -Force; `
[System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072;
`iex ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))
choco install make -y
Now back to make-ing
cd %USERPROFILE%\Documents\lammps-2019\src
Still not working since ‘tr’ is a unix command.
Added tr to path manually (C:\Program Files\Git\usr\bin)
Had to use C:\Progra~1\Git\usr\bin instead to fix space error
make yes-user-reaxc
make yes-user-qeq
make yes-user-echemdid
make serial
Ran into an error related to parsing using git sh.exe.  So removed that bin from path again temporarily
Still running into related errors, trying MSYS2, installed from https://www.msys2.org 
In msys2 ran:
pacman -Syu
pacman -S base-devel mingw-w64-x86_64-gcc mingw-w64-x86_64-gcc-fortran
Then in msys2 MinGW64 tried make serial with a couple different flags.  Still didn’t work so:
Edited src/my_pool_chunk.h line 199 changing posix_memalign -> _aligned_malloc
#if defined(LAMMPS_MEMALIGN)
  void* ptr = _aligned_malloc(chunkperpage * chunksize[ibin] * sizeof(T), LAMMPS_MEMALIGN);
  if (!ptr) errorflag = 2;
  pages[i] = (T*)ptr;
#else
  pages[i] = (T *) malloc(chunkperpage*chunksize[ibin]*sizeof(T));
  size += chunkperpage*chunksize[ibin];
  if (!pages[i]) errorflag = 2;
#endif
Now hit other errors making changes to src/compute_reaxff_atom.cpp
Line 19: added #include <vector>
Commented out all lines with utils::
Commented out all lines with fmt::
Commented out all lines and dependents referencing kokkosable
Had to add #include <vector> to src/pair_reaxff.h
Copied in text_file_reader.h and text_file_reader.cpp from 23 June 2022
Also had to copy in tokenizer.h and tokenizer.cpp
This still wasn’t working.  Deleted the whole user-reaxc folder and copied another one from the 7Aug2019 tarball
Make clean-all and make yes-user-reaxc (doing just this for testing) then make serial
Changed all comm->forward_comm(this) to forward_comm() same for reverse_comm
Trying a different route.  I think our versions got mixed at some point.  Going back to the tarball download of 7Aug19, brought over the echemdid and qeq user packages (not reaxc).  Tried to make, ran into the pasixmemalign issue so copied over the my_pool_chunk file.
Still didn’t work, switching to mpi in the same source directory
pacman -Syu
pacman -S mingw-w64-x86_64-msmpi
Had to do the old _aligned_malloc(size, alignment) fix on my_page.h this time
Had to modify the old fix to elevate all T’s by one *
Then ran into ewald constant issue, I’m just adding a quick ewald_const.h file I’m defining the constant in
This didn’t work.  Deleting the full QEQ and trying a 2019 one
git clone https://github.com/lammps/lammps.git qeqsrc
cd qeqsrc
git checkout stable_7Aug2019
Wait so apparently back in 2019 they didn’t use a separate USER-QEQ folder.  So I guess I shouldn’t even be adding it to the make build (vastly different then how its done nowadays)
Had to delete some modern files that were conflicting (fix_qeq_ctip.cpp and fix_qeq_ctip.h and others of the same cateogry)
Had to clean up memory.cpp too
Finally built!!!
Turns out the executables aren’t working and we can’t execute.  No idea why this is.
Starting next time, I want to start over with just a pure lammps source and pure reaxc (no qeq).  I think editing the source files were part of the issue
