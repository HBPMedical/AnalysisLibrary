/* src/include/config.h.  Generated from config.h.in by configure.  */
/* src/include/config.h.in.  Generated from configure.ac by autoheader.  */

#ifndef _CONFIG_H
#define _CONFIG_H

/* Define if building universal (internal helper macro) */
/* #undef AC_APPLE_UNIVERSAL_BUILD */

/* Define if to disable MPI_File_sync */
/* #undef DISABLE_FILE_SYNC */

/* Define if to disable in-place byte swap */
/* #undef DISABLE_IN_PLACE_SWAP */

/* Define if to enable single large MPI-IO request */
/* #undef ENABLE_LARGE_REQ */

/* Define if to enable strict null-byte padding in file header */
/* #undef ENABLE_NULL_BYTE_HEADER_PADDING */

/* Define if able to support request aggregation in nonblocking routines */
#define ENABLE_REQ_AGGREGATION /**/

/* Define if to enable subfiling feature */
/* #undef ENABLE_SUBFILING */

/* Define if Fortran names are lower case */
/* #undef F77_NAME_LOWER */

/* Define if Fortran names are lower case with two trailing underscore2 */
/* #undef F77_NAME_LOWER_2USCORE */

/* Define if Fortran names are lower case with one trailing underscore */
/* #undef F77_NAME_LOWER_USCORE */

/* Define if Fortran names are uppercase */
/* #undef F77_NAME_UPPER */

/* Define to dummy `main' function (if any) required to link to the Fortran
   libraries. */
/* #undef FC_DUMMY_MAIN */

/* Define if F77 and FC dummy `main' functions are identical. */
/* #undef FC_DUMMY_MAIN_EQ_F77 */

/* Define to 1 if you have the `access' function. */
#define HAVE_ACCESS 1

/* available */
#define HAVE_DECL_MPI_BYTE 1

/* available */
#define HAVE_DECL_MPI_CHAR 1

/* available */
/* #undef HAVE_DECL_MPI_CHARACTER */

/* Define to 1 if you have the declaration of `MPI_COMBINER_DARRAY', and to 0
   if you don't. */
#define HAVE_DECL_MPI_COMBINER_DARRAY 1

/* Define to 1 if you have the declaration of `MPI_COMBINER_DUP', and to 0 if
   you don't. */
#define HAVE_DECL_MPI_COMBINER_DUP 1

/* Define to 1 if you have the declaration of `MPI_COMBINER_F90_COMPLEX', and
   to 0 if you don't. */
#define HAVE_DECL_MPI_COMBINER_F90_COMPLEX 1

/* Define to 1 if you have the declaration of `MPI_COMBINER_F90_INTEGER', and
   to 0 if you don't. */
#define HAVE_DECL_MPI_COMBINER_F90_INTEGER 1

/* Define to 1 if you have the declaration of `MPI_COMBINER_F90_REAL', and to
   0 if you don't. */
#define HAVE_DECL_MPI_COMBINER_F90_REAL 1

/* Define to 1 if you have the declaration of `MPI_COMBINER_HINDEXED_INTEGER',
   and to 0 if you don't. */
#define HAVE_DECL_MPI_COMBINER_HINDEXED_INTEGER 1

/* Define to 1 if you have the declaration of `MPI_COMBINER_HVECTOR_INTEGER',
   and to 0 if you don't. */
#define HAVE_DECL_MPI_COMBINER_HVECTOR_INTEGER 1

/* Define to 1 if you have the declaration of `MPI_COMBINER_INDEXED_BLOCK',
   and to 0 if you don't. */
#define HAVE_DECL_MPI_COMBINER_INDEXED_BLOCK 1

/* Define to 1 if you have the declaration of `MPI_COMBINER_RESIZED', and to 0
   if you don't. */
#define HAVE_DECL_MPI_COMBINER_RESIZED 1

/* Define to 1 if you have the declaration of `MPI_COMBINER_STRUCT_INTEGER',
   and to 0 if you don't. */
#define HAVE_DECL_MPI_COMBINER_STRUCT_INTEGER 1

/* Define to 1 if you have the declaration of `MPI_COMBINER_SUBARRAY', and to
   0 if you don't. */
#define HAVE_DECL_MPI_COMBINER_SUBARRAY 1

/* available */
/* #undef HAVE_DECL_MPI_COMPLEX16 */

/* available */
/* #undef HAVE_DECL_MPI_COMPLEX32 */

/* available */
/* #undef HAVE_DECL_MPI_COMPLEX8 */

/* available */
#define HAVE_DECL_MPI_DOUBLE 1

/* available */
/* #undef HAVE_DECL_MPI_DOUBLE_PRECISION */

/* Define to 1 if you have the declaration of `MPI_ERR_ACCESS', and to 0 if
   you don't. */
#define HAVE_DECL_MPI_ERR_ACCESS 1

/* Define to 1 if you have the declaration of `MPI_ERR_AMODE', and to 0 if you
   don't. */
#define HAVE_DECL_MPI_ERR_AMODE 1

/* Define to 1 if you have the declaration of `MPI_ERR_BAD_FILE', and to 0 if
   you don't. */
#define HAVE_DECL_MPI_ERR_BAD_FILE 1

/* Define to 1 if you have the declaration of `MPI_ERR_FILE_EXISTS', and to 0
   if you don't. */
#define HAVE_DECL_MPI_ERR_FILE_EXISTS 1

/* Define to 1 if you have the declaration of `MPI_ERR_NOT_SAME', and to 0 if
   you don't. */
#define HAVE_DECL_MPI_ERR_NOT_SAME 1

/* Define to 1 if you have the declaration of `MPI_ERR_NO_SPACE', and to 0 if
   you don't. */
#define HAVE_DECL_MPI_ERR_NO_SPACE 1

/* Define to 1 if you have the declaration of `MPI_ERR_NO_SUCH_FILE', and to 0
   if you don't. */
#define HAVE_DECL_MPI_ERR_NO_SUCH_FILE 1

/* Define to 1 if you have the declaration of `MPI_ERR_QUOTA', and to 0 if you
   don't. */
#define HAVE_DECL_MPI_ERR_QUOTA 1

/* Define to 1 if you have the declaration of `MPI_ERR_READ_ONLY', and to 0 if
   you don't. */
#define HAVE_DECL_MPI_ERR_READ_ONLY 1

/* available */
#define HAVE_DECL_MPI_FLOAT 1

/* available */
#define HAVE_DECL_MPI_INT 1

/* available */
/* #undef HAVE_DECL_MPI_INTEGER */

/* available */
/* #undef HAVE_DECL_MPI_INTEGER1 */

/* available */
/* #undef HAVE_DECL_MPI_INTEGER16 */

/* available */
/* #undef HAVE_DECL_MPI_INTEGER2 */

/* available */
/* #undef HAVE_DECL_MPI_INTEGER4 */

/* available */
/* #undef HAVE_DECL_MPI_INTEGER8 */

/* available */
#define HAVE_DECL_MPI_LB 1

/* available */
#define HAVE_DECL_MPI_LONG 1

/* available */
#define HAVE_DECL_MPI_LONG_LONG_INT 1

/* available */
#define HAVE_DECL_MPI_OFFSET 1

/* available */
/* #undef HAVE_DECL_MPI_REAL */

/* available */
/* #undef HAVE_DECL_MPI_REAL16 */

/* available */
/* #undef HAVE_DECL_MPI_REAL4 */

/* available */
/* #undef HAVE_DECL_MPI_REAL8 */

/* available */
#define HAVE_DECL_MPI_SHORT 1

/* available */
#define HAVE_DECL_MPI_SIGNED_CHAR 1

/* available */
#define HAVE_DECL_MPI_UB 1

/* available */
#define HAVE_DECL_MPI_UNSIGNED 1

/* available */
#define HAVE_DECL_MPI_UNSIGNED_CHAR 1

/* available */
#define HAVE_DECL_MPI_UNSIGNED_LONG_LONG 1

/* available */
#define HAVE_DECL_MPI_UNSIGNED_SHORT 1

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Define if C++ macro __FUNCTION__ is defined */
#define HAVE_FUNCTION_MACRO 1

/* Define if C++ macro __func__ is defined */
#define HAVE_FUNC_MACRO 1

/* Define to 1 if the system has the type `int64'. */
/* #undef HAVE_INT64 */

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if the system has the type `longlong'. */
/* #undef HAVE_LONGLONG */

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the `MPI_Get_address' function. */
#define HAVE_MPI_GET_ADDRESS 1

/* Define to 1 if you have the `MPI_Info_dup' function. */
#define HAVE_MPI_INFO_DUP 1

/* Define to 1 if you have the `MPI_Info_free' function. */
#define HAVE_MPI_INFO_FREE 1

/* Define to 1 if you have the `MPI_Type_create_hindexed' function. */
#define HAVE_MPI_TYPE_CREATE_HINDEXED 1

/* Define to 1 if you have the `MPI_Type_create_hvector' function. */
#define HAVE_MPI_TYPE_CREATE_HVECTOR 1

/* Define to 1 if you have the `MPI_Type_create_resized' function. */
#define HAVE_MPI_TYPE_CREATE_RESIZED 1

/* Define to 1 if you have the `MPI_Type_create_struct' function. */
#define HAVE_MPI_TYPE_CREATE_STRUCT 1

/* Define to 1 if you have the `MPI_Type_create_subarray' function. */
#define HAVE_MPI_TYPE_CREATE_SUBARRAY 1

/* Define to 1 if you have the `MPI_Type_get_extent' function. */
#define HAVE_MPI_TYPE_GET_EXTENT 1

/* Define to 1 if you have the `MPI_Type_size_x' function. */
#define HAVE_MPI_TYPE_SIZE_X 1

/* Define to 1 if the system has the type `ptrdiff_t'. */
#define HAVE_PTRDIFF_T 1

/* Define to 1 if the system has the type `schar'. */
/* #undef HAVE_SCHAR */

/* Define to 1 if you have the <search.h> header file. */
/* #undef HAVE_SEARCH_H */

/* Define to 1 if stdbool.h conforms to C99. */
#define HAVE_STDBOOL_H 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the `strerror' function. */
#define HAVE_STRERROR 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the `tdelete' function. */
/* #undef HAVE_TDELETE */

/* Define to 1 if you have the `tsearch' function. */
/* #undef HAVE_TSEARCH */

/* Define to 1 if the system has the type `uchar'. */
/* #undef HAVE_UCHAR */

/* Define to 1 if the system has the type `uint'. */
#define HAVE_UINT 1

/* Define to 1 if the system has the type `uint64'. */
/* #undef HAVE_UINT64 */

/* Define to 1 if the system has the type `ulonglong'. */
/* #undef HAVE_ULONGLONG */

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to 1 if you have the `unlink' function. */
#define HAVE_UNLINK 1

/* Define to 1 if the system has the type `ushort'. */
#define HAVE_USHORT 1

/* Define to 1 if you have the `usleep' function. */
/* #undef HAVE_USLEEP */

/* Define to 1 if the system has the type `_Bool'. */
#define HAVE__BOOL 1

/* Define to the sub-directory where libtool stores uninstalled libraries. */
#define LT_OBJDIR ".libs/"

/* Type of NC_BYTE */
/* #undef NCBYTE_T */

/* Type of NC_SHORT */
/* #undef NCSHORT_T */

/* C type for Fortran double */
/* #undef NF_DOUBLEPRECISION_IS_C_ */

/* C type for Fortran INT1 */
/* #undef NF_INT1_IS_C_ */

/* Type for Fortran INT1 */
/* #undef NF_INT1_T */

/* C type for Fortran INT2 */
/* #undef NF_INT2_IS_C_ */

/* Type for Fortran INT2 */
/* #undef NF_INT2_T */

/* C type for Fortran INT8 */
/* #undef NF_INT8_IS_C_ */

/* Type for Fortran INT8 */
/* #undef NF_INT8_T */

/* C type for Fortran INT */
/* #undef NF_INT_IS_C_ */

/* C type for Fortran REAL */
/* #undef NF_REAL_IS_C_ */

/* Does system have IEEE FLOAT */
/* #undef NO_IEEE_FLOAT */

/* Name of package */
#define PACKAGE "parallel-netcdf"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "parallel-netcdf@mcs.anl.gov"

/* Define to the full name of this package. */
#define PACKAGE_NAME "parallel-netcdf"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "parallel-netcdf 1.9.0"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "parallel-netcdf"

/* Define to the home page for this package. */
#define PACKAGE_URL ""

/* Define to the version of this package. */
#define PACKAGE_VERSION "1.9.0"

/* Define if to enable malloc tracing */
/* #undef PNC_MALLOC_TRACE */

/* Define if relaxed coordinate check is enabled */
/* #undef RELAX_COORD_BOUND */

/* The size of `char', as computed by sizeof. */
#define SIZEOF_CHAR 1

/* The size of `double', as computed by sizeof. */
#define SIZEOF_DOUBLE 8

/* The size of `float', as computed by sizeof. */
#define SIZEOF_FLOAT 4

/* The size of `int', as computed by sizeof. */
#define SIZEOF_INT 4

/* The size of `long', as computed by sizeof. */
#define SIZEOF_LONG 8

/* The size of `longlong', as computed by sizeof. */
/* #undef SIZEOF_LONGLONG */

/* The size of `long long', as computed by sizeof. */
#define SIZEOF_LONG_LONG 8

/* The size of `MPI_Aint', as computed by sizeof. */
#define SIZEOF_MPI_AINT 8

/* The size of `MPI_Offset', as computed by sizeof. */
#define SIZEOF_MPI_OFFSET 8

/* The size of `off_t', as computed by sizeof. */
#define SIZEOF_OFF_T 8

/* The size of `schar', as computed by sizeof. */
/* #undef SIZEOF_SCHAR */

/* The size of `short', as computed by sizeof. */
#define SIZEOF_SHORT 2

/* The size of `signed char', as computed by sizeof. */
#define SIZEOF_SIGNED_CHAR 1

/* The size of `size_t', as computed by sizeof. */
#define SIZEOF_SIZE_T 8

/* The size of `uchar', as computed by sizeof. */
/* #undef SIZEOF_UCHAR */

/* The size of `uint', as computed by sizeof. */
#define SIZEOF_UINT 4

/* The size of `ulonglong', as computed by sizeof. */
/* #undef SIZEOF_ULONGLONG */

/* The size of `unsigned char', as computed by sizeof. */
#define SIZEOF_UNSIGNED_CHAR 1

/* The size of `unsigned int', as computed by sizeof. */
#define SIZEOF_UNSIGNED_INT 4

/* The size of `unsigned long long', as computed by sizeof. */
#define SIZEOF_UNSIGNED_LONG_LONG 8

/* The size of `unsigned short', as computed by sizeof. */
#define SIZEOF_UNSIGNED_SHORT 2

/* The size of `unsigned short int', as computed by sizeof. */
#define SIZEOF_UNSIGNED_SHORT_INT 2

/* The size of `ushort', as computed by sizeof. */
#define SIZEOF_USHORT 2

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Define if performing coverage tests */
/* #undef USE_COVERAGE */

/* Version number of package */
#define VERSION "1.9.0"

/* Define WORDS_BIGENDIAN to 1 if your processor stores words with the most
   significant byte first (like Motorola and SPARC, unlike Intel). */
#if defined AC_APPLE_UNIVERSAL_BUILD
# if defined __BIG_ENDIAN__
#  define WORDS_BIGENDIAN 1
# endif
#else
# ifndef WORDS_BIGENDIAN
/* #  undef WORDS_BIGENDIAN */
# endif
#endif

/* Enable large inode numbers on Mac OS X 10.5.  */
#ifndef _DARWIN_USE_64_BIT_INODE
# define _DARWIN_USE_64_BIT_INODE 1
#endif

/* Number of bits in a file offset, on hosts where this is settable. */
/* #undef _FILE_OFFSET_BITS */

/* Define for large files, on AIX-style hosts. */
/* #undef _LARGE_FILES */

/* Define to 1 if type `char' is unsigned and you are not using gcc.  */
#ifndef __CHAR_UNSIGNED__
/* # undef __CHAR_UNSIGNED__ */
#endif

/* Define to `__inline__' or `__inline' if that's what the C compiler
   calls it, or to nothing if 'inline' is not supported under any name.  */
#ifndef __cplusplus
/* #undef inline */
#endif

/* Define to `long int' if <sys/types.h> does not define. */
/* #undef off_t */

/* Define to `unsigned int' if <sys/types.h> does not define. */
/* #undef size_t */

/* Define to `int' if <sys/types.h> does not define. */
/* #undef ssize_t */

#include <nctypes.h>
#endif
