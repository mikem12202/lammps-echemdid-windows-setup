/* ----------------------------------------------------------------------
   LAMMPS - Large-scale Atomic/Molecular Massively Parallel Simulator
   http://lammps.sandia.gov, Sandia National Laboratories
   Steve Plimpton, sjplimp@sandia.gov

   Copyright (2003) Sandia Corporation.  Under the terms of Contract
   DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government retains
   certain rights in this software.  This software is distributed under
   the GNU General Public License.

   See the README file in the top-level LAMMPS directory.
------------------------------------------------------------------------- */

/* ----------------------------------------------------------------------
   Contributing authors: Jim Shepherd (GA Tech) added SGI SCSL support
                         Axel Kohlmeyer (Temple U) added support for
                         FFTW3, KISS FFT, Dfti/MKL, and ACML.
                         Phil Blood (PSC) added single precision FFT.
                         Paul Coffman (IBM) added MPI collectives remap
------------------------------------------------------------------------- */

#include "fft3d.h"
#include <mpi.h>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include "remap.h"

#ifdef FFT_KISS
/* include kissfft implementation */
#include "kissfft.h"
#endif

#define MIN(A,B) ((A) < (B) ? (A) : (B))
#define MAX(A,B) ((A) > (B) ? (A) : (B))

/* ----------------------------------------------------------------------
   Data layout for 3d FFTs:

   data set of Nfast x Nmid x Nslow elements is owned by P procs
   on input, each proc owns a subsection of the elements
   on output, each proc will own a (possibly different) subsection
   my subsection must not overlap with any other proc's subsection,
     i.e. the union of all proc's input (or output) subsections must
     exactly tile the global Nfast x Nmid x Nslow data set
   when called from C, all subsection indices are
     C-style from 0 to N-1 where N = Nfast or Nmid or Nslow
   when called from F77, all subsection indices are
     F77-style from 1 to N where N = Nfast or Nmid or Nslow
   a proc can own 0 elements on input or output
     by specifying hi index < lo index
   on both input and output, data is stored contiguously on a processor
     with a fast-varying, mid-varying, and slow-varying index
------------------------------------------------------------------------- */

/* ----------------------------------------------------------------------
   Perform 3d FFT

   Arguments:
   in           starting address of input data on this proc
   out          starting address of where output data for this proc
                  will be placed (can be same as in)
   flag         1 for forward FFT, -1 for inverse FFT
   plan         plan returned by previous call to fft_3d_create_plan
------------------------------------------------------------------------- */

void fft_3d(FFT_DATA *in, FFT_DATA *out, int flag, struct fft_plan_3d *plan)
{
  int i,total,length,offset,num;
  FFT_SCALAR norm;
#if defined(FFT_FFTW3)
  FFT_SCALAR *out_ptr;
#endif
  FFT_DATA *data,*copy;

  // system specific constants

#if defined(FFT_FFTW3)
  FFTW_API(plan) theplan;
#else
  // nothing to do for other FFTs
#endif

  // pre-remap to prepare for 1st FFTs if needed
  // copy = loc for remap result

  if (plan->pre_plan) {
    if (plan->pre_target == 0) copy = out;
    else copy = plan->copy;
    remap_3d((FFT_SCALAR *) in, (FFT_SCALAR *) copy,
             (FFT_SCALAR *) plan->scratch, plan->pre_plan);
    data = copy;
  }
  else
    data = in;

  // 1d FFTs along fast axis

  total = plan->total1;
  length = plan->length1;

#if defined(FFT_MKL)
  if (flag == -1)
    DftiComputeForward(plan->handle_fast,data);
  else
    DftiComputeBackward(plan->handle_fast,data);
  /*
#elif defined(FFT_FFTW2)
  if (flag == -1)
    fftw(plan->plan_fast_forward,total/length,data,1,length,NULL,0,0);
  else
   fftw(plan->plan_fast_backward,total/length,data,1,length,NULL,0,0);
  */
#elif defined(FFT_FFTW3)
  if (flag == -1)
    theplan=plan->plan_fast_forward;
  else
    theplan=plan->plan_fast_backward;
  FFTW_API(execute_dft)(theplan,data,data);
#else
  if (flag == -1)
    for (offset = 0; offset < total; offset += length)
      kiss_fft(plan->cfg_fast_forward,&data[offset],&data[offset]);
  else
    for (offset = 0; offset < total; offset += length)
      kiss_fft(plan->cfg_fast_backward,&data[offset],&data[offset]);
#endif

  // 1st mid-remap to prepare for 2nd FFTs
  // copy = loc for remap result

  if (plan->mid1_target == 0) copy = out;
  else copy = plan->copy;
  remap_3d((FFT_SCALAR *) data, (FFT_SCALAR *) copy,
           (FFT_SCALAR *) plan->scratch, plan->mid1_plan);
  data = copy;

  // 1d FFTs along mid axis

  total = plan->total2;
  length = plan->length2;

#if defined(FFT_MKL)
  if (flag == -1)
    DftiComputeForward(plan->handle_mid,data);
  else
    DftiComputeBackward(plan->handle_mid,data);
  /*
#elif defined(FFT_FFTW2)
  if (flag == -1)
    fftw(plan->plan_mid_forward,total/length,data,1,length,NULL,0,0);
  else
    fftw(plan->plan_mid_backward,total/length,data,1,length,NULL,0,0);
  */
#elif defined(FFT_FFTW3)
  if (flag == -1)
    theplan=plan->plan_mid_forward;
  else
    theplan=plan->plan_mid_backward;
  FFTW_API(execute_dft)(theplan,data,data);
#else
  if (flag == -1)
    for (offset = 0; offset < total; offset += length)
      kiss_fft(plan->cfg_mid_forward,&data[offset],&data[offset]);
  else
    for (offset = 0; offset < total; offset += length)
      kiss_fft(plan->cfg_mid_backward,&data[offset],&data[offset]);
#endif

  // 2nd mid-remap to prepare for 3rd FFTs
  // copy = loc for remap result

  if (plan->mid2_target == 0) copy = out;
  else copy = plan->copy;
  remap_3d((FFT_SCALAR *) data, (FFT_SCALAR *) copy,
           (FFT_SCALAR *) plan->scratch, plan->mid2_plan);
  data = copy;

  // 1d FFTs along slow axis

  total = plan->total3;
  length = plan->length3;

#if defined(FFT_MKL)
  if (flag == -1)
    DftiComputeForward(plan->handle_slow,data);
  else
    DftiComputeBackward(plan->handle_slow,data);
  /*
#elif defined(FFT_FFTW2)
  if (flag == -1)
    fftw(plan->plan_slow_forward,total/length,data,1,length,NULL,0,0);
  else
    fftw(plan->plan_slow_backward,total/length,data,1,length,NULL,0,0);
  */
#elif defined(FFT_FFTW3)
  if (flag == -1)
    theplan=plan->plan_slow_forward;
  else
    theplan=plan->plan_slow_backward;
  FFTW_API(execute_dft)(theplan,data,data);
#else
  if (flag == -1)
    for (offset = 0; offset < total; offset += length)
      kiss_fft(plan->cfg_slow_forward,&data[offset],&data[offset]);
  else
    for (offset = 0; offset < total; offset += length)
      kiss_fft(plan->cfg_slow_backward,&data[offset],&data[offset]);
#endif

  // post-remap to put data in output format if needed
  // destination is always out

  if (plan->post_plan)
    remap_3d((FFT_SCALAR *) data, (FFT_SCALAR *) out,
             (FFT_SCALAR *) plan->scratch, plan->post_plan);

  // scaling if required
  if (flag == 1 && plan->scaled) {
    norm = plan->norm;
    num = plan->normnum;
#if defined(FFT_FFTW3)
    out_ptr = (FFT_SCALAR *)out;
#endif
    for (i = 0; i < num; i++) {
#if defined(FFT_FFTW3)
      *(out_ptr++) *= norm;
      *(out_ptr++) *= norm;
#elif defined(FFT_MKL)
      out[i] *= norm;
#else
      out[i].re *= norm;
      out[i].im *= norm;
#endif
    }
  }
}

/* ----------------------------------------------------------------------
   Create plan for performing a 3d FFT

   Arguments:
   comm                 MPI communicator for the P procs which own the data
   nfast,nmid,nslow     size of global 3d matrix
   in_ilo,in_ihi        input bounds of data I own in fast index
   in_jlo,in_jhi        input bounds of data I own in mid index
   in_klo,in_khi        input bounds of data I own in slow index
   out_ilo,out_ihi      output bounds of data I own in fast index
   out_jlo,out_jhi      output bounds of data I own in mid index
   out_klo,out_khi      output bounds of 