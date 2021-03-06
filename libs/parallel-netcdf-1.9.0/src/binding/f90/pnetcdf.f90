!
!  Copyright (C) 2013, Northwestern University and Argonne National Laboratory
!  See COPYRIGHT notice in top-level directory.
!
! $Id: pnetcdf.f90 1816 2014-10-11 04:19:56Z wkliao $
!
! This file is taken from netcdf.f90 with changes for PnetCDF use
!
!
 module pnetcdf
  

  private

  integer, parameter ::   OneByteInt = selected_int_kind(2), &
                          TwoByteInt = selected_int_kind(4), &
                         FourByteInt = selected_int_kind(9), &
                        EightByteInt = selected_int_kind(18)

  integer, parameter ::                                          &
                        FourByteReal = selected_real_kind(P =  6, R =  37), &
                       EightByteReal = selected_real_kind(P = 13, R = 307)

  include "nfmpi_constants.fh"
  include "nf90_constants.fh"
  include "api.fh"
  include "overloads.fh"
  include "visibility.fh"

contains
  include "file.fh"
  include "dims.fh"
  include "attributes.fh"
  include "variables.fh"
  include "getput_text.fh"
  include "getput_var.fh"
  include "getput_varn.fh"
  include "getput_vard.fh"
end module pnetcdf
