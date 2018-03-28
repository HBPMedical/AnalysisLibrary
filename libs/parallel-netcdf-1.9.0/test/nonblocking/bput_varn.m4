dnl This is m4 source.
dnl Process using m4 to produce 'C' language file.
dnl
dnl If you see this line, you can ignore the next one.
/* Do not edit this file. It is produced from the corresponding .m4 source */
dnl
/*
 *  Copyright (C) 2014, Northwestern University and Argonne National Laboratory
 *  See COPYRIGHT notice in top-level directory.
 */
/* $Id: bput_varn.m4 3566 2017-11-24 20:44:28Z wkliao $ */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * This example tests nonblocking buffered write varn APIs, including
 * ncmpi_bput_varn_<type>() and ncmpi_bput_varn(),
 * It first writes a sequence of requests with arbitrary array indices and
 * lengths to four variables of various NC data types, and reads back.
 *
 * The compile and run commands are given below, together with an ncmpidump of
 * the output file.
 *
 *    % m4 bput_varn.m4 > bput_varn.c
 *    % mpicc -O2 -o bput_varn bput_varn.c -lpnetcdf
 *    % mpiexec -n 4 ./bput_varn /pvfs2/wkliao/testfile.nc
 *    % ncmpidump /pvfs2/wkliao/testfile.nc
 *    netcdf testfile {
 *    dimensions:
 *             Y = 4 ;
 *             X = 10 ;
 *    variables:
 *            uint64 var0(Y, X) ;
 *            uint64 var1(Y, X) ;
 *            uint64 var2(Y, X) ;
 *            uint64 var3(Y, X) ;
 *    data:
 *
 *     var0 =
 *      13, 13, 13, 11, 11, 10, 10, 12, 11, 11,
 *      10, 12, 12, 12, 13, 11, 11, 12, 12, 12,
 *      11, 11, 12, 13, 13, 13, 10, 10, 11, 11,
 *      10, 10, 10, 12, 11, 11, 11, 13, 13, 13 ;
 *     var1 =
 *      12, 12, 12, 10, 10, 13, 13, 11, 10, 10,
 *      13, 11, 11, 11, 12, 10, 10, 11, 11, 11,
 *      10, 10, 11, 12, 12, 12, 13, 13, 10, 10,
 *      13, 13, 13, 11, 10, 10, 10, 12, 12, 12 ;

 *
 *     var2 =
 *      11, 11, 11, 13, 13, 12, 12, 10, 13, 13,
 *      12, 10, 10, 10, 11, 13, 13, 10, 10, 10,
 *      13, 13, 10, 11, 11, 11, 12, 12, 13, 13,
 *      12, 12, 12, 10, 13, 13, 13, 11, 11, 11 ;
 *
 *     var3 =
 *      10, 10, 10, 12, 12, 11, 11, 13, 12, 12,
 *      11, 13, 13, 13, 10, 12, 12, 13, 13, 13,
 *      12, 12, 13, 10, 10, 10, 11, 11, 12, 12,
 *      11, 11, 11, 13, 12, 12, 12, 10, 10, 10 ;
 *    }
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifdef HAVE_CONFIG_H
#include <config.h> /* output of 'configure' */
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h> /* strcpy() */
#include <libgen.h> /* basename() */
#include <mpi.h>
#include <pnetcdf.h>

#include <testutils.h>

#define NLOOPS 4
#define MAX_NREQS 6
#define NDIMS 2

#define NY 4
#define NX 10

typedef char text;

include(`foreach.m4')dnl
include(`utils.m4')dnl

#define ERRS(n,a) { \
    int _i; \
    for (_i=0; _i<(n); _i++) { \
        if ((a)[_i] != NC_NOERR) { \
            printf("Error at line %d in %s: err[%d] %s\n", __LINE__, __FILE__, _i, \
                   ncmpi_strerrno((a)[_i])); \
            nerrs++; \
        } \
    } \
}

static int
check_num_pending_reqs(int ncid, int expected, int lineno)
/* check if PnetCDF can reports expected number of pending requests */
{
    int err, nerrs=0, n_pendings;

    /* NULL argument test */
    err = ncmpi_inq_nreqs(ncid, NULL); CHECK_ERR

    err = ncmpi_inq_nreqs(ncid, &n_pendings); CHECK_ERR
    if (n_pendings != expected) {
        printf("Error at line %d in %s: expect %d pending requests but got %d\n",
               lineno, __FILE__, expected, n_pendings);
        nerrs++;
    }
    return nerrs;
}

static
int check_attached_buffer_usage(int ncid,
                                MPI_Offset expected_size,
                                MPI_Offset expected_usage,
                                int lineno)
/* check attached buf usage */
{
    int err, nerrs=0, rank;
    MPI_Offset usage, buf_size;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank >= 4) return nerrs;

    /* NULL argument test */
    err = ncmpi_inq_buffer_size(ncid, NULL); CHECK_ERR
    err = ncmpi_inq_buffer_usage(ncid, NULL); CHECK_ERR

    err = ncmpi_inq_buffer_size(ncid, &buf_size);
    CHECK_ERR
    if (expected_size != buf_size) {
        printf("Error at line %d in %s: expect buffer size %lld but got %lld\n",
               lineno, __FILE__,expected_size, buf_size);
        nerrs++;
    }

    err = ncmpi_inq_buffer_usage(ncid, &usage); CHECK_ERR
    if (expected_usage != usage) {
        printf("Error at line %d in %s: expect buffer usage %lld but got %lld\n",
               lineno, __FILE__,expected_usage, usage);
        nerrs++;
    }

    return nerrs;
}

/* swap two rows, a and b, of a 2D array */
static
void permute(MPI_Offset *a, MPI_Offset *b)
{
    int i;
    MPI_Offset tmp;
    for (i=0; i<NDIMS; i++) {
        tmp = a[i]; a[i] = b[i]; b[i] = tmp;
    }
}

define(`TEST_BPUT_VARN',`dnl
static
int clear_file_contents_$1(int ncid, int *varid)
{
    int i, err, nerrs=0, rank;
    $1 *w_buffer = ($1*) malloc(NY*NX * sizeof($1));
    for (i=0; i<NY*NX; i++) w_buffer[i] = 99;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    for (i=0; i<4; i++) {
        err = ncmpi_put_var_$1_all(ncid, varid[i], w_buffer);
        CHECK_ERR
    }
    free(w_buffer);
    return nerrs;
}

static
int check_contents_for_fail_$1(int ncid, int *varid)
{
    /* all processes read entire variables back and check contents */
    int i, j, err, nerrs=0, nprocs;
    $1 expected[4][NY*NX] =
                            {{13, 13, 13, 11, 11, 10, 10, 12, 11, 11,
                              10, 12, 12, 12, 13, 11, 11, 12, 12, 12,
                              11, 11, 12, 13, 13, 13, 10, 10, 11, 11,
                              10, 10, 10, 12, 11, 11, 11, 13, 13, 13},
                             {12, 12, 12, 10, 10, 13, 13, 11, 10, 10,
                              13, 11, 11, 11, 12, 10, 10, 11, 11, 11,
                              10, 10, 11, 12, 12, 12, 13, 13, 10, 10,
                              13, 13, 13, 11, 10, 10, 10, 12, 12, 12},
                             {11, 11, 11, 13, 13, 12, 12, 10, 13, 13,
                              12, 10, 10, 10, 11, 13, 13, 10, 10, 10,
                              13, 13, 10, 11, 11, 11, 12, 12, 13, 13,
                              12, 12, 12, 10, 13, 13, 13, 11, 11, 11},
                             {10, 10, 10, 12, 12, 11, 11, 13, 12, 12,
                              11, 13, 13, 13, 10, 12, 12, 13, 13, 13,
                              12, 12, 13, 10, 10, 10, 11, 11, 12, 12,
                              11, 11, 11, 13, 12, 12, 12, 10, 10, 10}};

    $1 *r_buffer = ($1*) malloc(NY*NX * sizeof($1));

    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    if (nprocs > 4) MPI_Barrier(MPI_COMM_WORLD);

    for (i=0; i<4; i++) {
        for (j=0; j<NY*NX; j++) r_buffer[j] = 99;
        err = ncmpi_get_var_$1_all(ncid, varid[i], r_buffer);
        CHECK_ERR

        /* check if the contents of buf are expected */
        for (j=0; j<NY*NX; j++) {
            if (expected[i][j] >= nprocs) continue;
            if (r_buffer[j] != expected[i][j]) {
                printf("Expected read buf[%d][%d]=IFMT($1), but got IFMT($1)\n",
                       i,j,expected[i][j],r_buffer[j]);
                nerrs++;
            }
        }
    }
    free(r_buffer);
    return nerrs;
}

static int
test_bput_varn_$1(char *filename, int cdf)
{
    int i, j, k, rank, err, nerrs=0;
    int ncid, cmode, varid[NLOOPS], dimid[2], nreqs, reqs[NLOOPS], sts[NLOOPS];
    int req_lens[NLOOPS], my_nsegs[NLOOPS], num_segs[NLOOPS] = {4, 6, 5, 4};
    $1 *buffer[NLOOPS];
    MPI_Offset **starts[NLOOPS], **counts[NLOOPS];
    MPI_Offset n_starts[NLOOPS][MAX_NREQS][2] =
                                    {{{0,5}, {1,0}, {2,6}, {3,0}, {0,0}, {0,0}},
                                    {{0,3}, {0,8}, {1,5}, {2,0}, {2,8}, {3,4}},
                                    {{0,7}, {1,1}, {1,7}, {2,2}, {3,3}, {0,0}},
                                    {{0,0}, {1,4}, {2,3}, {3,7}, {0,0}, {0,0}}};
    MPI_Offset n_counts[NLOOPS][MAX_NREQS][2] =
                                    {{{1,2}, {1,1}, {1,2}, {1,3}, {0,0}, {0,0}},
                                    {{1,2}, {1,2}, {1,2}, {1,2}, {1,2}, {1,3}},
                                    {{1,1}, {1,3}, {1,3}, {1,1}, {1,1}, {0,0}},
                                    {{1,3}, {1,1}, {1,3}, {1,3}, {0,0}, {0,0}}};

    /* n_starts[0][][] n_counts[0][][] indicate the following: ("-" means skip)
              -  -  -  -  -  X  X  -  -  - 
              X  -  -  -  -  -  -  -  -  - 
              -  -  -  -  -  -  X  X  -  - 
              X  X  X  -  -  -  -  -  -  - 
       n_starts[1][][] n_counts[1][][] indicate the following pattern.
              -  -  -  X  X  -  -  -  X  X 
              -  -  -  -  -  X  X  -  -  - 
              X  X  -  -  -  -  -  -  X  X 
              -  -  -  -  X  X  X  -  -  - 
       n_starts[2][][] n_counts[2][][] indicate the following pattern.
              -  -  -  -  -  -  -  X  -  - 
              -  X  X  X  -  -  -  X  X  X 
              -  -  X  -  -  -  -  -  -  - 
              -  -  -  X  -  -  -  -  -  - 
       n_starts[3][][] n_counts[3][][] indicate the following pattern.
              X  X  X  -  -  -  -  -  -  - 
              -  -  -  -  X  -  -  -  -  - 
              -  -  -  X  X  X  -  -  -  - 
              -  -  -  -  -  -  -  X  X  X 
     */

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    /* create a new file for writing ----------------------------------------*/
    cmode = NC_CLOBBER;
    if (cdf == NC_FORMAT_CDF2)
        cmode |= NC_64BIT_OFFSET;
    else if (cdf == NC_FORMAT_CDF5)
        cmode |= NC_64BIT_DATA;
    err = ncmpi_create(MPI_COMM_WORLD, filename, cmode, MPI_INFO_NULL, &ncid);
    CHECK_ERR

    /* create a global array of size NY * NX */
    err = ncmpi_def_dim(ncid, "Y", NY, &dimid[0]); CHECK_ERR
    err = ncmpi_def_dim(ncid, "X", NX, &dimid[1]); CHECK_ERR
    err = ncmpi_def_var(ncid, "var0", NC_TYPE($1), NDIMS, dimid, &varid[0]); CHECK_ERR
    err = ncmpi_def_var(ncid, "var1", NC_TYPE($1), NDIMS, dimid, &varid[1]); CHECK_ERR
    err = ncmpi_def_var(ncid, "var2", NC_TYPE($1), NDIMS, dimid, &varid[2]); CHECK_ERR
    err = ncmpi_def_var(ncid, "var3", NC_TYPE($1), NDIMS, dimid, &varid[3]); CHECK_ERR
    err = ncmpi_enddef(ncid); CHECK_ERR

    /* allocate space for starts and counts */
    starts[0] = (MPI_Offset**) malloc(4 * 6 * sizeof(MPI_Offset*));
    counts[0] = (MPI_Offset**) malloc(4 * 6 * sizeof(MPI_Offset*));
    starts[0][0] = (MPI_Offset*) calloc(4 * 6 * NDIMS, sizeof(MPI_Offset));
    counts[0][0] = (MPI_Offset*) calloc(4 * 6 * NDIMS, sizeof(MPI_Offset));
    for (i=1; i<4; i++) {
        starts[i] = starts[i-1] + 6;
        counts[i] = counts[i-1] + 6;
        starts[i][0] = starts[i-1][0] + 6 * NDIMS;
        counts[i][0] = counts[i-1][0] + 6 * NDIMS;
    }
    for (i=0; i<4; i++) {
        for (j=1; j<6; j++) {
            starts[i][j] = starts[i][j-1] + NDIMS;
            counts[i][j] = counts[i][j-1] + NDIMS;
        }
    }

    /* set values for starts and counts */
    for (i=0; i<NLOOPS; i++) {
        int n = (i + rank) % NLOOPS;
        my_nsegs[i] = num_segs[n]; /* number of segments for this request */
        for (j=0; j<MAX_NREQS; j++) {
            for (k=0; k<NDIMS; k++) {
                starts[i][j][k] = n_starts[n][j][k];
                counts[i][j][k] = n_counts[n][j][k];
            }
        }
    }

    /* test error code: NC_ENULLABUF */
    err = ncmpi_bput_varn_$1(ncid, varid[0], 1, NULL, NULL, NULL, &reqs[0]);
    EXP_ERR(NC_ENULLABUF)

    /* only rank 0, 1, 2, and 3 do I/O:
     * each of ranks 0 to 3 write 4 nonblocking requests */
    nreqs = 4;
    if (rank >= 4) nreqs = 0;

    /* bufsize must be max of data type converted before and after */
    MPI_Offset bufsize = 0;

    /* calculate length of each varn request and allocate write buffer */
    for (i=0; i<nreqs; i++) {
        req_lens[i] = 0; /* total length this request */
        for (j=0; j<my_nsegs[i]; j++) {
            MPI_Offset req_len=1;
            for (k=0; k<NDIMS; k++)
                req_len *= counts[i][j][k];
            req_lens[i] += req_len;
        }

        /* allocate I/O buffer and initialize its contents */
        buffer[i] = ($1*) malloc(req_lens[i] * sizeof($1));
        for (j=0; j<req_lens[i]; j++) buffer[i][j] = ($1)rank+10;
        bufsize += req_lens[i];
    }
    bufsize *= sizeof($1);

    /* give PnetCDF a space to buffer the nonblocking requests */
    if (bufsize > 0) {
        err = ncmpi_buffer_attach(ncid, bufsize); CHECK_ERR
    }

    /* test error code: NC_ENULLSTART */
    err = ncmpi_bput_varn_$1(ncid, varid[0], 1, NULL, NULL, NULL, &reqs[0]);
    if (rank < 4) EXP_ERR(NC_ENULLSTART)
    else if (bufsize == 0) EXP_ERR(NC_ENULLABUF)
    else CHECK_ERR

    /* write using varn API, one bput call per variable */
    nerrs += clear_file_contents_$1(ncid, varid);
    for (i=0; i<nreqs; i++) {
        err = ncmpi_bput_varn_$1(ncid, varid[i], my_nsegs[i], starts[i],
                                 counts[i], buffer[i], &reqs[i]);
        CHECK_ERR
    }
    /* check if write buffer contents have been altered */
    for (i=0; i<nreqs; i++) {
        for (j=0; j<req_lens[i]; j++) {
            if (buffer[i][j] != ($1)rank+10) {
                printf("Error at line %d in %s: put buffer altered buffer[%d][%d]=IFMT($1)\n",
                       __LINE__,__FILE__,i,j,buffer[i][j]);
                nerrs++;
            }
        }
    }
    nerrs += check_num_pending_reqs(ncid, nreqs, __LINE__);
    nerrs += check_attached_buffer_usage(ncid, bufsize, bufsize, __LINE__);
    err = ncmpi_wait_all(ncid, nreqs, reqs, sts);
    ERRS(nreqs, sts)

    nerrs += check_attached_buffer_usage(ncid, bufsize, 0, __LINE__);

    /* all processes read entire variables back and check contents */
    nerrs += check_contents_for_fail_$1(ncid, varid);

    /* permute write order: so starts[*] are not in an increasing order:
     * swap segment 0 with segment 2 and swap segment 1 with segment 3
     */
    for (i=0; i<nreqs; i++) {
        permute(starts[i][0],starts[i][2]); permute(counts[i][0],counts[i][2]);
        permute(starts[i][1],starts[i][3]); permute(counts[i][1],counts[i][3]);
    }

    /* write using varn API, one bput call per variable */
    nerrs += clear_file_contents_$1(ncid, varid);
    for (i=0; i<nreqs; i++) {
        err = ncmpi_bput_varn_$1(ncid, varid[i], my_nsegs[i], starts[i],
                                 counts[i], buffer[i], &reqs[i]);
        CHECK_ERR
    }
    /* check if write buffer contents have been altered */
    for (i=0; i<nreqs; i++) {
        for (j=0; j<req_lens[i]; j++) {
            if (buffer[i][j] != ($1)rank+10) {
                printf("Error at line %d in %s: put buffer altered buffer[%d][%d]=IFMT($1)\n",
                       __LINE__,__FILE__,i,j,buffer[i][j]);
                nerrs++;
            }
        }
    }
    nerrs += check_num_pending_reqs(ncid, nreqs, __LINE__);
    nerrs += check_attached_buffer_usage(ncid, bufsize, bufsize, __LINE__);
    err = ncmpi_wait_all(ncid, nreqs, reqs, sts);
    ERRS(nreqs, sts)

    nerrs += check_attached_buffer_usage(ncid, bufsize, 0, __LINE__);

    /* all processes read entire variables back and check contents */
    nerrs += check_contents_for_fail_$1(ncid, varid);

    for (i=0; i<nreqs; i++) free(buffer[i]);

    /* test flexible API, using a noncontiguous buftype */
    nerrs += clear_file_contents_$1(ncid, varid);
    for (i=0; i<nreqs; i++) {
        MPI_Datatype buftype;
        MPI_Type_vector(req_lens[i], 1, 2, ITYPE2MPI($1), &buftype);
        MPI_Type_commit(&buftype);
        buffer[i] = ($1*)malloc(req_lens[i]*2*sizeof($1));
        for (j=0; j<req_lens[i]*2; j++) buffer[i][j] = ($1)rank+10;

        err = ncmpi_bput_varn(ncid, varid[i], my_nsegs[i], starts[i],
                              counts[i], buffer[i], 1, buftype, &reqs[i]);
        CHECK_ERR
        MPI_Type_free(&buftype);
    }
    /* check if write buffer contents have been altered */
    for (i=0; i<nreqs; i++) {
        for (j=0; j<req_lens[i]*2; j++) {
            if (buffer[i][j] != ($1)rank+10) {
                printf("Error at line %d in %s: put buffer altered buffer[%d][%d]=IFMT($1)\n",
                       __LINE__,__FILE__,i,j,buffer[i][j]);
                nerrs++;
            }
        }
    }
    nerrs += check_num_pending_reqs(ncid, nreqs, __LINE__);
    nerrs += check_attached_buffer_usage(ncid, bufsize, bufsize, __LINE__);
    err = ncmpi_wait_all(ncid, nreqs, reqs, sts);
    ERRS(nreqs, sts)

    /* check if write buffer contents have been altered */
    for (i=0; i<nreqs; i++) {
        for (j=0; j<req_lens[i]*2; j++) {
            if (buffer[i][j] != ($1)rank+10) {
                printf("Error at line %d in %s: put buffer altered buffer[%d][%d]=IFMT($1)\n",
                       __LINE__,__FILE__,i,j,buffer[i][j]);
                nerrs++;
            }
        }
    }
    nerrs += check_attached_buffer_usage(ncid, bufsize, 0, __LINE__);

    /* all processes read entire variables back and check contents */
    nerrs += check_contents_for_fail_$1(ncid, varid);

    /* permute back to original order */
    for (i=0; i<nreqs; i++) {
        permute(starts[i][0],starts[i][2]); permute(counts[i][0],counts[i][2]);
        permute(starts[i][1],starts[i][3]); permute(counts[i][1],counts[i][3]);
    }

    /* test flexible API, using a noncontiguous buftype, one bput call per
     * variable */
    nerrs += clear_file_contents_$1(ncid, varid);
    for (i=0; i<nreqs; i++) {
        MPI_Datatype buftype;
        MPI_Type_vector(req_lens[i], 1, 2, ITYPE2MPI($1), &buftype);
        MPI_Type_commit(&buftype);
        for (j=0; j<req_lens[i]*2; j++) buffer[i][j] = ($1)rank+10;

        err = ncmpi_bput_varn(ncid, varid[i], my_nsegs[i], starts[i],
                              counts[i], buffer[i], 1, buftype, &reqs[i]);
        CHECK_ERR
        MPI_Type_free(&buftype);
    }
    /* check if write buffer contents have been altered */
    for (i=0; i<nreqs; i++) {
        for (j=0; j<req_lens[i]*2; j++) {
            if (buffer[i][j] != ($1)rank+10) {
                printf("Error at line %d in %s: put buffer altered buffer[%d][%d]=IFMT($1)\n",
                       __LINE__,__FILE__,i,j,buffer[i][j]);
                nerrs++;
            }
        }
    }
    nerrs += check_num_pending_reqs(ncid, nreqs, __LINE__);
    nerrs += check_attached_buffer_usage(ncid, bufsize, bufsize, __LINE__);
    err = ncmpi_wait_all(ncid, nreqs, reqs, sts);
    ERRS(nreqs, sts)

    /* check if write buffer contents have been altered */
    for (i=0; i<nreqs; i++) {
        for (j=0; j<req_lens[i]*2; j++) {
            if (buffer[i][j] != ($1)rank+10) {
                printf("Error at line %d in %s: put buffer altered buffer[%d][%d]=IFMT($1)\n",
                       __LINE__,__FILE__,i,j,buffer[i][j]);
                nerrs++;
            }
        }
    }
    nerrs += check_attached_buffer_usage(ncid, bufsize, 0, __LINE__);

    /* all processes read entire variables back and check contents */
    nerrs += check_contents_for_fail_$1(ncid, varid);

    /* free the buffer space for bput */
    if (bufsize > 0) {
        err = ncmpi_buffer_detach(ncid); CHECK_ERR
    }

    /* test error code: NC_ENULLABUF */
    err = ncmpi_inq_buffer_usage(ncid, &bufsize);
    EXP_ERR(NC_ENULLABUF)

    err = ncmpi_close(ncid); CHECK_ERR

    for (i=0; i<nreqs; i++) free(buffer[i]);
    free(starts[0][0]);
    free(counts[0][0]);
    free(starts[0]);
    free(counts[0]);

    return nerrs;
}
')
TEST_BPUT_VARN(text)
TEST_BPUT_VARN(schar)
TEST_BPUT_VARN(uchar)
TEST_BPUT_VARN(short)
TEST_BPUT_VARN(ushort)
TEST_BPUT_VARN(int)
TEST_BPUT_VARN(uint)
TEST_BPUT_VARN(float)
TEST_BPUT_VARN(double)
TEST_BPUT_VARN(longlong)
TEST_BPUT_VARN(ulonglong)

int main(int argc, char** argv)
{
    char filename[256];
    int i, rank, err, nerrs=0;
    int cdf_formats[3]={NC_FORMAT_CLASSIC, NC_FORMAT_CDF2, NC_FORMAT_CDF5};

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (argc > 2) {
        if (!rank) printf("Usage: %s [filename]\n",argv[0]);
        MPI_Finalize();
        return 1;
    }
    if (argc == 2) snprintf(filename, 256, "%s", argv[1]);
    else           strcpy(filename, "testfile.nc");
    MPI_Bcast(filename, 256, MPI_CHAR, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        char *cmd_str = (char*)malloc(strlen(argv[0]) + 256);
        sprintf(cmd_str, "*** TESTING C   %s for bput_varn ", basename(argv[0]));
        printf("%-66s ------ ", cmd_str);
        free(cmd_str);
    }

    for (i=0; i<3; i++) {
        nerrs += test_bput_varn_text(filename, cdf_formats[i]);
        nerrs += test_bput_varn_schar(filename, cdf_formats[i]);
        nerrs += test_bput_varn_short(filename, cdf_formats[i]);
        nerrs += test_bput_varn_int(filename, cdf_formats[i]);
        nerrs += test_bput_varn_float(filename, cdf_formats[i]);
        nerrs += test_bput_varn_double(filename, cdf_formats[i]);
        if (cdf_formats[i] == NC_FORMAT_CDF5) {
            nerrs += test_bput_varn_uchar(filename, cdf_formats[i]);
            nerrs += test_bput_varn_ushort(filename, cdf_formats[i]);
            nerrs += test_bput_varn_uint(filename, cdf_formats[i]);
            nerrs += test_bput_varn_longlong(filename, cdf_formats[i]);
            nerrs += test_bput_varn_ulonglong(filename, cdf_formats[i]);
        }
    }

    /* check if PnetCDF freed all internal malloc */
    MPI_Offset malloc_size, sum_size;
    err = ncmpi_inq_malloc_size(&malloc_size);
    if (err == NC_NOERR) {
        MPI_Reduce(&malloc_size, &sum_size, 1, MPI_OFFSET, MPI_SUM, 0, MPI_COMM_WORLD);
        if (rank == 0 && sum_size > 0)
            printf("heap memory allocated by PnetCDF internally has %lld bytes yet to be freed\n",
                   sum_size);
    }

    MPI_Allreduce(MPI_IN_PLACE, &nerrs, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
    if (rank == 0) {
        if (nerrs) printf(FAIL_STR,nerrs);
        else       printf(PASS_STR);
    }

    MPI_Finalize();
    return (nerrs > 0);
}

