/*********************************************************************
 *
 *  Copyright (C) 2015, Northwestern University and Argonne National Laboratory
 *  See COPYRIGHT notice in top-level directory.
 *
 *********************************************************************/
/* $Id: column_wise.m4 3422 2017-09-25 05:42:52Z wkliao $ */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * This example tests a number of nonblocking API calls, each writes a single
 * column of a 2D integer array. Each process writes NX columns and any two
 * consecutive columns are of nprocs columns distance apart from each other. In
 * this case, the fileview of each process interleaves with all other processes.
 * If simply concatenating fileviews of all the nonblocking calls will result
 * in a fileview that violates the MPI-IO requirement on the fileview of which
 * flattened file offsets must be monotonically non-decreasing. PnetCDF handles
 * this case by breaking down each nonblocking call into a list of offset-length
 * pairs, merging the pairs across multiple nonblocking calls, and sorting
 * them into an increasing order. The sorted pairs are used to construct a
 * fileview that meets the monotonically non-decreasing offset requirement,
 * and thus the nonblocking requests can be serviced by a single MPI-IO call. 
 * 
 * The compile and run commands are given below, together with an ncmpidump of
 * the output file.
 *
 *    % m4 column_wise.m4 > column_wise.c
 *    % mpicc -O2 -o column_wise column_wise.c -lpnetcdf
 *    % mpiexec -l -n 4 ./column_wise /pvfs2/wkliao/testfile.nc
 *    0:  0: myOff=  0 myNX=  4
 *    1:  1: myOff=  4 myNX=  4
 *    2:  2: myOff=  8 myNX=  4
 *    3:  3: myOff= 12 myNX=  4
 *    0:  0: start=  0   0 count= 10   1
 *    1:  1: start=  0   1 count= 10   1
 *    2:  2: start=  0   2 count= 10   1
 *    3:  3: start=  0   3 count= 10   1
 *
 *    % ncmpidump /pvfs2/wkliao/testfile.nc
 *    netcdf testfile {
 *    // file format: CDF-5 (big variables)
 *    dimensions:
 *            Y = 10 ;
 *            X = 16 ;
 *    variables:
 *            int var(Y, X) ;
 *    data:
 *
 *     var =
 *      10, 11, 12, 13, 10, 11, 12, 13, 10, 11, 12, 13, 10, 11, 12, 13,
 *      10, 11, 12, 13, 10, 11, 12, 13, 10, 11, 12, 13, 10, 11, 12, 13,
 *      10, 11, 12, 13, 10, 11, 12, 13, 10, 11, 12, 13, 10, 11, 12, 13,
 *      10, 11, 12, 13, 10, 11, 12, 13, 10, 11, 12, 13, 10, 11, 12, 13,
 *      10, 11, 12, 13, 10, 11, 12, 13, 10, 11, 12, 13, 10, 11, 12, 13,
 *      10, 11, 12, 13, 10, 11, 12, 13, 10, 11, 12, 13, 10, 11, 12, 13,
 *      10, 11, 12, 13, 10, 11, 12, 13, 10, 11, 12, 13, 10, 11, 12, 13,
 *      10, 11, 12, 13, 10, 11, 12, 13, 10, 11, 12, 13, 10, 11, 12, 13,
 *      10, 11, 12, 13, 10, 11, 12, 13, 10, 11, 12, 13, 10, 11, 12, 13,
 *      10, 11, 12, 13, 10, 11, 12, 13, 10, 11, 12, 13, 10, 11, 12, 13 ;
 *    }
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifdef HAVE_CONFIG_H
#include <config.h> /* output of 'configure' */
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h> /* strcpy() */
#include <libgen.h> /* basename() */
#include <unistd.h> /* getopt() */
#include <mpi.h>
#include <pnetcdf.h>

#include <testutils.h>

#define NY 10
#define NX 4

typedef char text;





static
int test_column_wise_text(char *filename, int cdf)
{
    int i, j, nerrs=0, rank, nprocs, err, myNX, G_NX, myOff, num_reqs;
    int ncid, cmode, varid, dimid[2], *reqs, *sts;
    text **buf;
    MPI_Offset start[2], count[2];

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    cmode = NC_CLOBBER;
    if (cdf == NC_FORMAT_CDF2)
        cmode |= NC_64BIT_OFFSET;
    else if (cdf == NC_FORMAT_CDF5)
        cmode |= NC_64BIT_DATA;
    err = ncmpi_create(MPI_COMM_WORLD, filename, cmode, MPI_INFO_NULL, &ncid);
    CHECK_ERR

    /* the global array is NY * (NX * nprocs) */
    G_NX  = NX * nprocs;
    myOff = NX * rank;
    myNX  = NX;

    err = ncmpi_def_dim(ncid, "Y", NY, &dimid[0]); CHECK_ERR
    err = ncmpi_def_dim(ncid, "X", G_NX, &dimid[1]); CHECK_ERR
    err = ncmpi_def_var(ncid, "var", NC_CHAR, 2, dimid, &varid); CHECK_ERR
    err = ncmpi_enddef(ncid); CHECK_ERR

    /* First, fill the entire array with zeros, using a blocking I/O.
       Every process writes a subarray of size NY * myNX */
    buf    = (text**) malloc(myNX * sizeof(text*));
    buf[0] = (text*)  calloc(NY * myNX, sizeof(text));
    start[0] = 0;   start[1] = myOff;
    count[0] = NY;  count[1] = myNX;
    err = ncmpi_put_vara_text_all(ncid, varid, start, count, buf[0]);
    free(buf[0]);

    /* initialize the buffer with rank ID. Also make the case interesting,
       by allocatsing buffersd separately */
    for (i=0; i<myNX; i++) {
        buf[i] = (text*) malloc(NY * sizeof(text));
        for (j=0; j<NY; j++) buf[i][j] = (text)rank+10;
    }

    reqs = (int*) malloc(myNX * sizeof(int));
    sts  = (int*) malloc(myNX * sizeof(int));

    /* each proc writes myNX single columns of the 2D array */
    start[0]  = 0;   start[1] = rank;
    count[0]  = NY;  count[1] = 1;

    num_reqs = 0;
    for (i=0; i<myNX; i++) {
        err = ncmpi_iput_vara_text(ncid, varid, start, count, buf[i],
                                 &reqs[num_reqs++]); CHECK_ERR
        start[1] += nprocs;
    }

    /* try re-order the request list */
    for (i=0; i<myNX/2; i++) {
        int tmp = reqs[2*i];
        reqs[2*i] = reqs[2*i+1];
        reqs[2*i+1] = tmp;
    }

    /* test cancelling requests and see if the user write buffer is properly
     * byte-swapped back to it original form. To do this test, NY must be
     * changed to use a number > NC_BYTE_SWAP_BUFFER_SIZE/sizeof(int), say
     * 1025
     */
    err = ncmpi_cancel(ncid, num_reqs, reqs, sts); CHECK_ERR

    /* check if write buffer contents have been altered after cancelling */
    for (i=0; i<myNX; i++) {
        for (j=0; j<NY; j++) {
            if (buf[i][j] != (text)rank+10) {
                printf("Error at line %d in %s: put buffer altered buffer[%d][%d]=%c\n",
                       __LINE__,__FILE__,i,j,buf[i][j]);
                nerrs++;
            }
        }
    }

    /* post iput requests again */
    start[1] = rank;
    num_reqs = 0;
    for (i=0; i<myNX; i++) {
        err = ncmpi_iput_vara_text(ncid, varid, start, count, buf[i],
                                 &reqs[num_reqs++]); CHECK_ERR
        start[1] += nprocs;
    }

    /* try re-order the request list */
    for (i=0; i<myNX/2; i++) {
        int tmp = reqs[2*i];
        reqs[2*i] = reqs[2*i+1];
        reqs[2*i+1] = tmp;
    }

    err = ncmpi_wait_all(ncid, num_reqs, reqs, sts); CHECK_ERR

    /* check if write buffer contents have been altered after wait */
    for (i=0; i<myNX; i++) {
        for (j=0; j<NY; j++) {
            if (buf[i][j] != (text)rank+10) {
                printf("Error at line %d in %s: put buffer altered buffer[%d][%d]=%c\n",
                       __LINE__,__FILE__,i,j,buf[i][j]);
                nerrs++;
            }
        }
    }

    /* check status of all requests */
    for (i=0; i<num_reqs; i++) {
        if (reqs[i] != NC_REQ_NULL) { /* add in PnetCDF v1.7.0 */
            printf("Error at line %d in %s: request ID %d fails to be set to NC_REQ_NULL\n",__LINE__,__FILE__,i);
            nerrs++;
        }
        if (sts[i] != NC_NOERR) {
            printf("Error at line %d in %s: nonblocking write fails on request %d (%s)\n",
                   __LINE__,__FILE__,i, ncmpi_strerror(sts[i]));
            nerrs++;
        }
    }

    err = ncmpi_close(ncid); CHECK_ERR

    err = ncmpi_open(MPI_COMM_WORLD, filename, NC_NOWRITE, MPI_INFO_NULL, &ncid);
    CHECK_ERR

    err = ncmpi_inq_varid(ncid, "var", &varid); CHECK_ERR

    /* read back using the same access pattern */
    for (i=0; i<myNX; i++)
        for (j=0; j<NY; j++) buf[i][j] = (text)99;

    /* each proc reads myNX single columns of the 2D array */
    start[0]  = 0;   start[1] = rank;
    count[0]  = NY;  count[1] = 1;

    num_reqs = 0;
    for (i=0; i<myNX; i++) {
        err = ncmpi_iget_vara_text(ncid, varid, start, count, buf[i],
                                 &reqs[num_reqs++]); CHECK_ERR
        start[1] += nprocs;
    }
    /* this test is to see if cancelling free up all the internal malloc */
    err = ncmpi_cancel(ncid, num_reqs, reqs, sts); CHECK_ERR

    /* post iget requests again */
    start[1] = rank;
    num_reqs = 0;
    for (i=0; i<myNX; i++) {
        err = ncmpi_iget_vara_text(ncid, varid, start, count, buf[i],
                                 &reqs[num_reqs++]); CHECK_ERR
        start[1] += nprocs;
    }
    err = ncmpi_wait_all(ncid, num_reqs, reqs, sts); CHECK_ERR

    /* check status of all requests */
    for (i=0; i<num_reqs; i++)
        if (sts[i] != NC_NOERR) {
            printf("Error at line %d in %s: nonblocking write fails on request %d (%s)\n",
                   __LINE__,__FILE__,i, ncmpi_strerror(sts[i]));
            nerrs++;
        }

    for (i=0; i<myNX; i++) {
        for (j=0; j<NY; j++) {
            text expected = (text)rank+10;
            if (buf[i][j] != expected) {
                printf("Error at line %d in %s: expect buf[%d][%d]=%c but got %c\n",
                       __LINE__,__FILE__,i,j,expected,buf[i][j]);
                nerrs++;
            }
        }
    }

    err = ncmpi_close(ncid); CHECK_ERR

    free(sts);
    free(reqs);
    for (i=0; i<myNX; i++) free(buf[i]);
    free(buf);

    return nerrs;
}

static
int test_column_wise_schar(char *filename, int cdf)
{
    int i, j, nerrs=0, rank, nprocs, err, myNX, G_NX, myOff, num_reqs;
    int ncid, cmode, varid, dimid[2], *reqs, *sts;
    schar **buf;
    MPI_Offset start[2], count[2];

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    cmode = NC_CLOBBER;
    if (cdf == NC_FORMAT_CDF2)
        cmode |= NC_64BIT_OFFSET;
    else if (cdf == NC_FORMAT_CDF5)
        cmode |= NC_64BIT_DATA;
    err = ncmpi_create(MPI_COMM_WORLD, filename, cmode, MPI_INFO_NULL, &ncid);
    CHECK_ERR

    /* the global array is NY * (NX * nprocs) */
    G_NX  = NX * nprocs;
    myOff = NX * rank;
    myNX  = NX;

    err = ncmpi_def_dim(ncid, "Y", NY, &dimid[0]); CHECK_ERR
    err = ncmpi_def_dim(ncid, "X", G_NX, &dimid[1]); CHECK_ERR
    err = ncmpi_def_var(ncid, "var", NC_BYTE, 2, dimid, &varid); CHECK_ERR
    err = ncmpi_enddef(ncid); CHECK_ERR

    /* First, fill the entire array with zeros, using a blocking I/O.
       Every process writes a subarray of size NY * myNX */
    buf    = (schar**) malloc(myNX * sizeof(schar*));
    buf[0] = (schar*)  calloc(NY * myNX, sizeof(schar));
    start[0] = 0;   start[1] = myOff;
    count[0] = NY;  count[1] = myNX;
    err = ncmpi_put_vara_schar_all(ncid, varid, start, count, buf[0]);
    free(buf[0]);

    /* initialize the buffer with rank ID. Also make the case interesting,
       by allocatsing buffersd separately */
    for (i=0; i<myNX; i++) {
        buf[i] = (schar*) malloc(NY * sizeof(schar));
        for (j=0; j<NY; j++) buf[i][j] = (schar)rank+10;
    }

    reqs = (int*) malloc(myNX * sizeof(int));
    sts  = (int*) malloc(myNX * sizeof(int));

    /* each proc writes myNX single columns of the 2D array */
    start[0]  = 0;   start[1] = rank;
    count[0]  = NY;  count[1] = 1;

    num_reqs = 0;
    for (i=0; i<myNX; i++) {
        err = ncmpi_iput_vara_schar(ncid, varid, start, count, buf[i],
                                 &reqs[num_reqs++]); CHECK_ERR
        start[1] += nprocs;
    }

    /* try re-order the request list */
    for (i=0; i<myNX/2; i++) {
        int tmp = reqs[2*i];
        reqs[2*i] = reqs[2*i+1];
        reqs[2*i+1] = tmp;
    }

    /* test cancelling requests and see if the user write buffer is properly
     * byte-swapped back to it original form. To do this test, NY must be
     * changed to use a number > NC_BYTE_SWAP_BUFFER_SIZE/sizeof(int), say
     * 1025
     */
    err = ncmpi_cancel(ncid, num_reqs, reqs, sts); CHECK_ERR

    /* check if write buffer contents have been altered after cancelling */
    for (i=0; i<myNX; i++) {
        for (j=0; j<NY; j++) {
            if (buf[i][j] != (schar)rank+10) {
                printf("Error at line %d in %s: put buffer altered buffer[%d][%d]=%hhd\n",
                       __LINE__,__FILE__,i,j,buf[i][j]);
                nerrs++;
            }
        }
    }

    /* post iput requests again */
    start[1] = rank;
    num_reqs = 0;
    for (i=0; i<myNX; i++) {
        err = ncmpi_iput_vara_schar(ncid, varid, start, count, buf[i],
                                 &reqs[num_reqs++]); CHECK_ERR
        start[1] += nprocs;
    }

    /* try re-order the request list */
    for (i=0; i<myNX/2; i++) {
        int tmp = reqs[2*i];
        reqs[2*i] = reqs[2*i+1];
        reqs[2*i+1] = tmp;
    }

    err = ncmpi_wait_all(ncid, num_reqs, reqs, sts); CHECK_ERR

    /* check if write buffer contents have been altered after wait */
    for (i=0; i<myNX; i++) {
        for (j=0; j<NY; j++) {
            if (buf[i][j] != (schar)rank+10) {
                printf("Error at line %d in %s: put buffer altered buffer[%d][%d]=%hhd\n",
                       __LINE__,__FILE__,i,j,buf[i][j]);
                nerrs++;
            }
        }
    }

    /* check status of all requests */
    for (i=0; i<num_reqs; i++) {
        if (reqs[i] != NC_REQ_NULL) { /* add in PnetCDF v1.7.0 */
            printf("Error at line %d in %s: request ID %d fails to be set to NC_REQ_NULL\n",__LINE__,__FILE__,i);
            nerrs++;
        }
        if (sts[i] != NC_NOERR) {
            printf("Error at line %d in %s: nonblocking write fails on request %d (%s)\n",
                   __LINE__,__FILE__,i, ncmpi_strerror(sts[i]));
            nerrs++;
        }
    }

    err = ncmpi_close(ncid); CHECK_ERR

    err = ncmpi_open(MPI_COMM_WORLD, filename, NC_NOWRITE, MPI_INFO_NULL, &ncid);
    CHECK_ERR

    err = ncmpi_inq_varid(ncid, "var", &varid); CHECK_ERR

    /* read back using the same access pattern */
    for (i=0; i<myNX; i++)
        for (j=0; j<NY; j++) buf[i][j] = (schar)99;

    /* each proc reads myNX single columns of the 2D array */
    start[0]  = 0;   start[1] = rank;
    count[0]  = NY;  count[1] = 1;

    num_reqs = 0;
    for (i=0; i<myNX; i++) {
        err = ncmpi_iget_vara_schar(ncid, varid, start, count, buf[i],
                                 &reqs[num_reqs++]); CHECK_ERR
        start[1] += nprocs;
    }
    /* this test is to see if cancelling free up all the internal malloc */
    err = ncmpi_cancel(ncid, num_reqs, reqs, sts); CHECK_ERR

    /* post iget requests again */
    start[1] = rank;
    num_reqs = 0;
    for (i=0; i<myNX; i++) {
        err = ncmpi_iget_vara_schar(ncid, varid, start, count, buf[i],
                                 &reqs[num_reqs++]); CHECK_ERR
        start[1] += nprocs;
    }
    err = ncmpi_wait_all(ncid, num_reqs, reqs, sts); CHECK_ERR

    /* check status of all requests */
    for (i=0; i<num_reqs; i++)
        if (sts[i] != NC_NOERR) {
            printf("Error at line %d in %s: nonblocking write fails on request %d (%s)\n",
                   __LINE__,__FILE__,i, ncmpi_strerror(sts[i]));
            nerrs++;
        }

    for (i=0; i<myNX; i++) {
        for (j=0; j<NY; j++) {
            schar expected = (schar)rank+10;
            if (buf[i][j] != expected) {
                printf("Error at line %d in %s: expect buf[%d][%d]=%hhd but got %hhd\n",
                       __LINE__,__FILE__,i,j,expected,buf[i][j]);
                nerrs++;
            }
        }
    }

    err = ncmpi_close(ncid); CHECK_ERR

    free(sts);
    free(reqs);
    for (i=0; i<myNX; i++) free(buf[i]);
    free(buf);

    return nerrs;
}

static
int test_column_wise_uchar(char *filename, int cdf)
{
    int i, j, nerrs=0, rank, nprocs, err, myNX, G_NX, myOff, num_reqs;
    int ncid, cmode, varid, dimid[2], *reqs, *sts;
    uchar **buf;
    MPI_Offset start[2], count[2];

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    cmode = NC_CLOBBER;
    if (cdf == NC_FORMAT_CDF2)
        cmode |= NC_64BIT_OFFSET;
    else if (cdf == NC_FORMAT_CDF5)
        cmode |= NC_64BIT_DATA;
    err = ncmpi_create(MPI_COMM_WORLD, filename, cmode, MPI_INFO_NULL, &ncid);
    CHECK_ERR

    /* the global array is NY * (NX * nprocs) */
    G_NX  = NX * nprocs;
    myOff = NX * rank;
    myNX  = NX;

    err = ncmpi_def_dim(ncid, "Y", NY, &dimid[0]); CHECK_ERR
    err = ncmpi_def_dim(ncid, "X", G_NX, &dimid[1]); CHECK_ERR
    err = ncmpi_def_var(ncid, "var", NC_UBYTE, 2, dimid, &varid); CHECK_ERR
    err = ncmpi_enddef(ncid); CHECK_ERR

    /* First, fill the entire array with zeros, using a blocking I/O.
       Every process writes a subarray of size NY * myNX */
    buf    = (uchar**) malloc(myNX * sizeof(uchar*));
    buf[0] = (uchar*)  calloc(NY * myNX, sizeof(uchar));
    start[0] = 0;   start[1] = myOff;
    count[0] = NY;  count[1] = myNX;
    err = ncmpi_put_vara_uchar_all(ncid, varid, start, count, buf[0]);
    free(buf[0]);

    /* initialize the buffer with rank ID. Also make the case interesting,
       by allocatsing buffersd separately */
    for (i=0; i<myNX; i++) {
        buf[i] = (uchar*) malloc(NY * sizeof(uchar));
        for (j=0; j<NY; j++) buf[i][j] = (uchar)rank+10;
    }

    reqs = (int*) malloc(myNX * sizeof(int));
    sts  = (int*) malloc(myNX * sizeof(int));

    /* each proc writes myNX single columns of the 2D array */
    start[0]  = 0;   start[1] = rank;
    count[0]  = NY;  count[1] = 1;

    num_reqs = 0;
    for (i=0; i<myNX; i++) {
        err = ncmpi_iput_vara_uchar(ncid, varid, start, count, buf[i],
                                 &reqs[num_reqs++]); CHECK_ERR
        start[1] += nprocs;
    }

    /* try re-order the request list */
    for (i=0; i<myNX/2; i++) {
        int tmp = reqs[2*i];
        reqs[2*i] = reqs[2*i+1];
        reqs[2*i+1] = tmp;
    }

    /* test cancelling requests and see if the user write buffer is properly
     * byte-swapped back to it original form. To do this test, NY must be
     * changed to use a number > NC_BYTE_SWAP_BUFFER_SIZE/sizeof(int), say
     * 1025
     */
    err = ncmpi_cancel(ncid, num_reqs, reqs, sts); CHECK_ERR

    /* check if write buffer contents have been altered after cancelling */
    for (i=0; i<myNX; i++) {
        for (j=0; j<NY; j++) {
            if (buf[i][j] != (uchar)rank+10) {
                printf("Error at line %d in %s: put buffer altered buffer[%d][%d]=%hhu\n",
                       __LINE__,__FILE__,i,j,buf[i][j]);
                nerrs++;
            }
        }
    }

    /* post iput requests again */
    start[1] = rank;
    num_reqs = 0;
    for (i=0; i<myNX; i++) {
        err = ncmpi_iput_vara_uchar(ncid, varid, start, count, buf[i],
                                 &reqs[num_reqs++]); CHECK_ERR
        start[1] += nprocs;
    }

    /* try re-order the request list */
    for (i=0; i<myNX/2; i++) {
        int tmp = reqs[2*i];
        reqs[2*i] = reqs[2*i+1];
        reqs[2*i+1] = tmp;
    }

    err = ncmpi_wait_all(ncid, num_reqs, reqs, sts); CHECK_ERR

    /* check if write buffer contents have been altered after wait */
    for (i=0; i<myNX; i++) {
        for (j=0; j<NY; j++) {
            if (buf[i][j] != (uchar)rank+10) {
                printf("Error at line %d in %s: put buffer altered buffer[%d][%d]=%hhu\n",
                       __LINE__,__FILE__,i,j,buf[i][j]);
                nerrs++;
            }
        }
    }

    /* check status of all requests */
    for (i=0; i<num_reqs; i++) {
        if (reqs[i] != NC_REQ_NULL) { /* add in PnetCDF v1.7.0 */
            printf("Error at line %d in %s: request ID %d fails to be set to NC_REQ_NULL\n",__LINE__,__FILE__,i);
            nerrs++;
        }
        if (sts[i] != NC_NOERR) {
            printf("Error at line %d in %s: nonblocking write fails on request %d (%s)\n",
                   __LINE__,__FILE__,i, ncmpi_strerror(sts[i]));
            nerrs++;
        }
    }

    err = ncmpi_close(ncid); CHECK_ERR

    err = ncmpi_open(MPI_COMM_WORLD, filename, NC_NOWRITE, MPI_INFO_NULL, &ncid);
    CHECK_ERR

    err = ncmpi_inq_varid(ncid, "var", &varid); CHECK_ERR

    /* read back using the same access pattern */
    for (i=0; i<myNX; i++)
        for (j=0; j<NY; j++) buf[i][j] = (uchar)99;

    /* each proc reads myNX single columns of the 2D array */
    start[0]  = 0;   start[1] = rank;
    count[0]  = NY;  count[1] = 1;

    num_reqs = 0;
    for (i=0; i<myNX; i++) {
        err = ncmpi_iget_vara_uchar(ncid, varid, start, count, buf[i],
                                 &reqs[num_reqs++]); CHECK_ERR
        start[1] += nprocs;
    }
    /* this test is to see if cancelling free up all the internal malloc */
    err = ncmpi_cancel(ncid, num_reqs, reqs, sts); CHECK_ERR

    /* post iget requests again */
    start[1] = rank;
    num_reqs = 0;
    for (i=0; i<myNX; i++) {
        err = ncmpi_iget_vara_uchar(ncid, varid, start, count, buf[i],
                                 &reqs[num_reqs++]); CHECK_ERR
        start[1] += nprocs;
    }
    err = ncmpi_wait_all(ncid, num_reqs, reqs, sts); CHECK_ERR

    /* check status of all requests */
    for (i=0; i<num_reqs; i++)
        if (sts[i] != NC_NOERR) {
            printf("Error at line %d in %s: nonblocking write fails on request %d (%s)\n",
                   __LINE__,__FILE__,i, ncmpi_strerror(sts[i]));
            nerrs++;
        }

    for (i=0; i<myNX; i++) {
        for (j=0; j<NY; j++) {
            uchar expected = (uchar)rank+10;
            if (buf[i][j] != expected) {
                printf("Error at line %d in %s: expect buf[%d][%d]=%hhu but got %hhu\n",
                       __LINE__,__FILE__,i,j,expected,buf[i][j]);
                nerrs++;
            }
        }
    }

    err = ncmpi_close(ncid); CHECK_ERR

    free(sts);
    free(reqs);
    for (i=0; i<myNX; i++) free(buf[i]);
    free(buf);

    return nerrs;
}

static
int test_column_wise_short(char *filename, int cdf)
{
    int i, j, nerrs=0, rank, nprocs, err, myNX, G_NX, myOff, num_reqs;
    int ncid, cmode, varid, dimid[2], *reqs, *sts;
    short **buf;
    MPI_Offset start[2], count[2];

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    cmode = NC_CLOBBER;
    if (cdf == NC_FORMAT_CDF2)
        cmode |= NC_64BIT_OFFSET;
    else if (cdf == NC_FORMAT_CDF5)
        cmode |= NC_64BIT_DATA;
    err = ncmpi_create(MPI_COMM_WORLD, filename, cmode, MPI_INFO_NULL, &ncid);
    CHECK_ERR

    /* the global array is NY * (NX * nprocs) */
    G_NX  = NX * nprocs;
    myOff = NX * rank;
    myNX  = NX;

    err = ncmpi_def_dim(ncid, "Y", NY, &dimid[0]); CHECK_ERR
    err = ncmpi_def_dim(ncid, "X", G_NX, &dimid[1]); CHECK_ERR
    err = ncmpi_def_var(ncid, "var", NC_SHORT, 2, dimid, &varid); CHECK_ERR
    err = ncmpi_enddef(ncid); CHECK_ERR

    /* First, fill the entire array with zeros, using a blocking I/O.
       Every process writes a subarray of size NY * myNX */
    buf    = (short**) malloc(myNX * sizeof(short*));
    buf[0] = (short*)  calloc(NY * myNX, sizeof(short));
    start[0] = 0;   start[1] = myOff;
    count[0] = NY;  count[1] = myNX;
    err = ncmpi_put_vara_short_all(ncid, varid, start, count, buf[0]);
    free(buf[0]);

    /* initialize the buffer with rank ID. Also make the case interesting,
       by allocatsing buffersd separately */
    for (i=0; i<myNX; i++) {
        buf[i] = (short*) malloc(NY * sizeof(short));
        for (j=0; j<NY; j++) buf[i][j] = (short)rank+10;
    }

    reqs = (int*) malloc(myNX * sizeof(int));
    sts  = (int*) malloc(myNX * sizeof(int));

    /* each proc writes myNX single columns of the 2D array */
    start[0]  = 0;   start[1] = rank;
    count[0]  = NY;  count[1] = 1;

    num_reqs = 0;
    for (i=0; i<myNX; i++) {
        err = ncmpi_iput_vara_short(ncid, varid, start, count, buf[i],
                                 &reqs[num_reqs++]); CHECK_ERR
        start[1] += nprocs;
    }

    /* try re-order the request list */
    for (i=0; i<myNX/2; i++) {
        int tmp = reqs[2*i];
        reqs[2*i] = reqs[2*i+1];
        reqs[2*i+1] = tmp;
    }

    /* test cancelling requests and see if the user write buffer is properly
     * byte-swapped back to it original form. To do this test, NY must be
     * changed to use a number > NC_BYTE_SWAP_BUFFER_SIZE/sizeof(int), say
     * 1025
     */
    err = ncmpi_cancel(ncid, num_reqs, reqs, sts); CHECK_ERR

    /* check if write buffer contents have been altered after cancelling */
    for (i=0; i<myNX; i++) {
        for (j=0; j<NY; j++) {
            if (buf[i][j] != (short)rank+10) {
                printf("Error at line %d in %s: put buffer altered buffer[%d][%d]=%hd\n",
                       __LINE__,__FILE__,i,j,buf[i][j]);
                nerrs++;
            }
        }
    }

    /* post iput requests again */
    start[1] = rank;
    num_reqs = 0;
    for (i=0; i<myNX; i++) {
        err = ncmpi_iput_vara_short(ncid, varid, start, count, buf[i],
                                 &reqs[num_reqs++]); CHECK_ERR
        start[1] += nprocs;
    }

    /* try re-order the request list */
    for (i=0; i<myNX/2; i++) {
        int tmp = reqs[2*i];
        reqs[2*i] = reqs[2*i+1];
        reqs[2*i+1] = tmp;
    }

    err = ncmpi_wait_all(ncid, num_reqs, reqs, sts); CHECK_ERR

    /* check if write buffer contents have been altered after wait */
    for (i=0; i<myNX; i++) {
        for (j=0; j<NY; j++) {
            if (buf[i][j] != (short)rank+10) {
                printf("Error at line %d in %s: put buffer altered buffer[%d][%d]=%hd\n",
                       __LINE__,__FILE__,i,j,buf[i][j]);
                nerrs++;
            }
        }
    }

    /* check status of all requests */
    for (i=0; i<num_reqs; i++) {
        if (reqs[i] != NC_REQ_NULL) { /* add in PnetCDF v1.7.0 */
            printf("Error at line %d in %s: request ID %d fails to be set to NC_REQ_NULL\n",__LINE__,__FILE__,i);
            nerrs++;
        }
        if (sts[i] != NC_NOERR) {
            printf("Error at line %d in %s: nonblocking write fails on request %d (%s)\n",
                   __LINE__,__FILE__,i, ncmpi_strerror(sts[i]));
            nerrs++;
        }
    }

    err = ncmpi_close(ncid); CHECK_ERR

    err = ncmpi_open(MPI_COMM_WORLD, filename, NC_NOWRITE, MPI_INFO_NULL, &ncid);
    CHECK_ERR

    err = ncmpi_inq_varid(ncid, "var", &varid); CHECK_ERR

    /* read back using the same access pattern */
    for (i=0; i<myNX; i++)
        for (j=0; j<NY; j++) buf[i][j] = (short)99;

    /* each proc reads myNX single columns of the 2D array */
    start[0]  = 0;   start[1] = rank;
    count[0]  = NY;  count[1] = 1;

    num_reqs = 0;
    for (i=0; i<myNX; i++) {
        err = ncmpi_iget_vara_short(ncid, varid, start, count, buf[i],
                                 &reqs[num_reqs++]); CHECK_ERR
        start[1] += nprocs;
    }
    /* this test is to see if cancelling free up all the internal malloc */
    err = ncmpi_cancel(ncid, num_reqs, reqs, sts); CHECK_ERR

    /* post iget requests again */
    start[1] = rank;
    num_reqs = 0;
    for (i=0; i<myNX; i++) {
        err = ncmpi_iget_vara_short(ncid, varid, start, count, buf[i],
                                 &reqs[num_reqs++]); CHECK_ERR
        start[1] += nprocs;
    }
    err = ncmpi_wait_all(ncid, num_reqs, reqs, sts); CHECK_ERR

    /* check status of all requests */
    for (i=0; i<num_reqs; i++)
        if (sts[i] != NC_NOERR) {
            printf("Error at line %d in %s: nonblocking write fails on request %d (%s)\n",
                   __LINE__,__FILE__,i, ncmpi_strerror(sts[i]));
            nerrs++;
        }

    for (i=0; i<myNX; i++) {
        for (j=0; j<NY; j++) {
            short expected = (short)rank+10;
            if (buf[i][j] != expected) {
                printf("Error at line %d in %s: expect buf[%d][%d]=%hd but got %hd\n",
                       __LINE__,__FILE__,i,j,expected,buf[i][j]);
                nerrs++;
            }
        }
    }

    err = ncmpi_close(ncid); CHECK_ERR

    free(sts);
    free(reqs);
    for (i=0; i<myNX; i++) free(buf[i]);
    free(buf);

    return nerrs;
}

static
int test_column_wise_ushort(char *filename, int cdf)
{
    int i, j, nerrs=0, rank, nprocs, err, myNX, G_NX, myOff, num_reqs;
    int ncid, cmode, varid, dimid[2], *reqs, *sts;
    ushort **buf;
    MPI_Offset start[2], count[2];

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    cmode = NC_CLOBBER;
    if (cdf == NC_FORMAT_CDF2)
        cmode |= NC_64BIT_OFFSET;
    else if (cdf == NC_FORMAT_CDF5)
        cmode |= NC_64BIT_DATA;
    err = ncmpi_create(MPI_COMM_WORLD, filename, cmode, MPI_INFO_NULL, &ncid);
    CHECK_ERR

    /* the global array is NY * (NX * nprocs) */
    G_NX  = NX * nprocs;
    myOff = NX * rank;
    myNX  = NX;

    err = ncmpi_def_dim(ncid, "Y", NY, &dimid[0]); CHECK_ERR
    err = ncmpi_def_dim(ncid, "X", G_NX, &dimid[1]); CHECK_ERR
    err = ncmpi_def_var(ncid, "var", NC_USHORT, 2, dimid, &varid); CHECK_ERR
    err = ncmpi_enddef(ncid); CHECK_ERR

    /* First, fill the entire array with zeros, using a blocking I/O.
       Every process writes a subarray of size NY * myNX */
    buf    = (ushort**) malloc(myNX * sizeof(ushort*));
    buf[0] = (ushort*)  calloc(NY * myNX, sizeof(ushort));
    start[0] = 0;   start[1] = myOff;
    count[0] = NY;  count[1] = myNX;
    err = ncmpi_put_vara_ushort_all(ncid, varid, start, count, buf[0]);
    free(buf[0]);

    /* initialize the buffer with rank ID. Also make the case interesting,
       by allocatsing buffersd separately */
    for (i=0; i<myNX; i++) {
        buf[i] = (ushort*) malloc(NY * sizeof(ushort));
        for (j=0; j<NY; j++) buf[i][j] = (ushort)rank+10;
    }

    reqs = (int*) malloc(myNX * sizeof(int));
    sts  = (int*) malloc(myNX * sizeof(int));

    /* each proc writes myNX single columns of the 2D array */
    start[0]  = 0;   start[1] = rank;
    count[0]  = NY;  count[1] = 1;

    num_reqs = 0;
    for (i=0; i<myNX; i++) {
        err = ncmpi_iput_vara_ushort(ncid, varid, start, count, buf[i],
                                 &reqs[num_reqs++]); CHECK_ERR
        start[1] += nprocs;
    }

    /* try re-order the request list */
    for (i=0; i<myNX/2; i++) {
        int tmp = reqs[2*i];
        reqs[2*i] = reqs[2*i+1];
        reqs[2*i+1] = tmp;
    }

    /* test cancelling requests and see if the user write buffer is properly
     * byte-swapped back to it original form. To do this test, NY must be
     * changed to use a number > NC_BYTE_SWAP_BUFFER_SIZE/sizeof(int), say
     * 1025
     */
    err = ncmpi_cancel(ncid, num_reqs, reqs, sts); CHECK_ERR

    /* check if write buffer contents have been altered after cancelling */
    for (i=0; i<myNX; i++) {
        for (j=0; j<NY; j++) {
            if (buf[i][j] != (ushort)rank+10) {
                printf("Error at line %d in %s: put buffer altered buffer[%d][%d]=%hu\n",
                       __LINE__,__FILE__,i,j,buf[i][j]);
                nerrs++;
            }
        }
    }

    /* post iput requests again */
    start[1] = rank;
    num_reqs = 0;
    for (i=0; i<myNX; i++) {
        err = ncmpi_iput_vara_ushort(ncid, varid, start, count, buf[i],
                                 &reqs[num_reqs++]); CHECK_ERR
        start[1] += nprocs;
    }

    /* try re-order the request list */
    for (i=0; i<myNX/2; i++) {
        int tmp = reqs[2*i];
        reqs[2*i] = reqs[2*i+1];
        reqs[2*i+1] = tmp;
    }

    err = ncmpi_wait_all(ncid, num_reqs, reqs, sts); CHECK_ERR

    /* check if write buffer contents have been altered after wait */
    for (i=0; i<myNX; i++) {
        for (j=0; j<NY; j++) {
            if (buf[i][j] != (ushort)rank+10) {
                printf("Error at line %d in %s: put buffer altered buffer[%d][%d]=%hu\n",
                       __LINE__,__FILE__,i,j,buf[i][j]);
                nerrs++;
            }
        }
    }

    /* check status of all requests */
    for (i=0; i<num_reqs; i++) {
        if (reqs[i] != NC_REQ_NULL) { /* add in PnetCDF v1.7.0 */
            printf("Error at line %d in %s: request ID %d fails to be set to NC_REQ_NULL\n",__LINE__,__FILE__,i);
            nerrs++;
        }
        if (sts[i] != NC_NOERR) {
            printf("Error at line %d in %s: nonblocking write fails on request %d (%s)\n",
                   __LINE__,__FILE__,i, ncmpi_strerror(sts[i]));
            nerrs++;
        }
    }

    err = ncmpi_close(ncid); CHECK_ERR

    err = ncmpi_open(MPI_COMM_WORLD, filename, NC_NOWRITE, MPI_INFO_NULL, &ncid);
    CHECK_ERR

    err = ncmpi_inq_varid(ncid, "var", &varid); CHECK_ERR

    /* read back using the same access pattern */
    for (i=0; i<myNX; i++)
        for (j=0; j<NY; j++) buf[i][j] = (ushort)99;

    /* each proc reads myNX single columns of the 2D array */
    start[0]  = 0;   start[1] = rank;
    count[0]  = NY;  count[1] = 1;

    num_reqs = 0;
    for (i=0; i<myNX; i++) {
        err = ncmpi_iget_vara_ushort(ncid, varid, start, count, buf[i],
                                 &reqs[num_reqs++]); CHECK_ERR
        start[1] += nprocs;
    }
    /* this test is to see if cancelling free up all the internal malloc */
    err = ncmpi_cancel(ncid, num_reqs, reqs, sts); CHECK_ERR

    /* post iget requests again */
    start[1] = rank;
    num_reqs = 0;
    for (i=0; i<myNX; i++) {
        err = ncmpi_iget_vara_ushort(ncid, varid, start, count, buf[i],
                                 &reqs[num_reqs++]); CHECK_ERR
        start[1] += nprocs;
    }
    err = ncmpi_wait_all(ncid, num_reqs, reqs, sts); CHECK_ERR

    /* check status of all requests */
    for (i=0; i<num_reqs; i++)
        if (sts[i] != NC_NOERR) {
            printf("Error at line %d in %s: nonblocking write fails on request %d (%s)\n",
                   __LINE__,__FILE__,i, ncmpi_strerror(sts[i]));
            nerrs++;
        }

    for (i=0; i<myNX; i++) {
        for (j=0; j<NY; j++) {
            ushort expected = (ushort)rank+10;
            if (buf[i][j] != expected) {
                printf("Error at line %d in %s: expect buf[%d][%d]=%hu but got %hu\n",
                       __LINE__,__FILE__,i,j,expected,buf[i][j]);
                nerrs++;
            }
        }
    }

    err = ncmpi_close(ncid); CHECK_ERR

    free(sts);
    free(reqs);
    for (i=0; i<myNX; i++) free(buf[i]);
    free(buf);

    return nerrs;
}

static
int test_column_wise_int(char *filename, int cdf)
{
    int i, j, nerrs=0, rank, nprocs, err, myNX, G_NX, myOff, num_reqs;
    int ncid, cmode, varid, dimid[2], *reqs, *sts;
    int **buf;
    MPI_Offset start[2], count[2];

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    cmode = NC_CLOBBER;
    if (cdf == NC_FORMAT_CDF2)
        cmode |= NC_64BIT_OFFSET;
    else if (cdf == NC_FORMAT_CDF5)
        cmode |= NC_64BIT_DATA;
    err = ncmpi_create(MPI_COMM_WORLD, filename, cmode, MPI_INFO_NULL, &ncid);
    CHECK_ERR

    /* the global array is NY * (NX * nprocs) */
    G_NX  = NX * nprocs;
    myOff = NX * rank;
    myNX  = NX;

    err = ncmpi_def_dim(ncid, "Y", NY, &dimid[0]); CHECK_ERR
    err = ncmpi_def_dim(ncid, "X", G_NX, &dimid[1]); CHECK_ERR
    err = ncmpi_def_var(ncid, "var", NC_INT, 2, dimid, &varid); CHECK_ERR
    err = ncmpi_enddef(ncid); CHECK_ERR

    /* First, fill the entire array with zeros, using a blocking I/O.
       Every process writes a subarray of size NY * myNX */
    buf    = (int**) malloc(myNX * sizeof(int*));
    buf[0] = (int*)  calloc(NY * myNX, sizeof(int));
    start[0] = 0;   start[1] = myOff;
    count[0] = NY;  count[1] = myNX;
    err = ncmpi_put_vara_int_all(ncid, varid, start, count, buf[0]);
    free(buf[0]);

    /* initialize the buffer with rank ID. Also make the case interesting,
       by allocatsing buffersd separately */
    for (i=0; i<myNX; i++) {
        buf[i] = (int*) malloc(NY * sizeof(int));
        for (j=0; j<NY; j++) buf[i][j] = (int)rank+10;
    }

    reqs = (int*) malloc(myNX * sizeof(int));
    sts  = (int*) malloc(myNX * sizeof(int));

    /* each proc writes myNX single columns of the 2D array */
    start[0]  = 0;   start[1] = rank;
    count[0]  = NY;  count[1] = 1;

    num_reqs = 0;
    for (i=0; i<myNX; i++) {
        err = ncmpi_iput_vara_int(ncid, varid, start, count, buf[i],
                                 &reqs[num_reqs++]); CHECK_ERR
        start[1] += nprocs;
    }

    /* try re-order the request list */
    for (i=0; i<myNX/2; i++) {
        int tmp = reqs[2*i];
        reqs[2*i] = reqs[2*i+1];
        reqs[2*i+1] = tmp;
    }

    /* test cancelling requests and see if the user write buffer is properly
     * byte-swapped back to it original form. To do this test, NY must be
     * changed to use a number > NC_BYTE_SWAP_BUFFER_SIZE/sizeof(int), say
     * 1025
     */
    err = ncmpi_cancel(ncid, num_reqs, reqs, sts); CHECK_ERR

    /* check if write buffer contents have been altered after cancelling */
    for (i=0; i<myNX; i++) {
        for (j=0; j<NY; j++) {
            if (buf[i][j] != (int)rank+10) {
                printf("Error at line %d in %s: put buffer altered buffer[%d][%d]=%d\n",
                       __LINE__,__FILE__,i,j,buf[i][j]);
                nerrs++;
            }
        }
    }

    /* post iput requests again */
    start[1] = rank;
    num_reqs = 0;
    for (i=0; i<myNX; i++) {
        err = ncmpi_iput_vara_int(ncid, varid, start, count, buf[i],
                                 &reqs[num_reqs++]); CHECK_ERR
        start[1] += nprocs;
    }

    /* try re-order the request list */
    for (i=0; i<myNX/2; i++) {
        int tmp = reqs[2*i];
        reqs[2*i] = reqs[2*i+1];
        reqs[2*i+1] = tmp;
    }

    err = ncmpi_wait_all(ncid, num_reqs, reqs, sts); CHECK_ERR

    /* check if write buffer contents have been altered after wait */
    for (i=0; i<myNX; i++) {
        for (j=0; j<NY; j++) {
            if (buf[i][j] != (int)rank+10) {
                printf("Error at line %d in %s: put buffer altered buffer[%d][%d]=%d\n",
                       __LINE__,__FILE__,i,j,buf[i][j]);
                nerrs++;
            }
        }
    }

    /* check status of all requests */
    for (i=0; i<num_reqs; i++) {
        if (reqs[i] != NC_REQ_NULL) { /* add in PnetCDF v1.7.0 */
            printf("Error at line %d in %s: request ID %d fails to be set to NC_REQ_NULL\n",__LINE__,__FILE__,i);
            nerrs++;
        }
        if (sts[i] != NC_NOERR) {
            printf("Error at line %d in %s: nonblocking write fails on request %d (%s)\n",
                   __LINE__,__FILE__,i, ncmpi_strerror(sts[i]));
            nerrs++;
        }
    }

    err = ncmpi_close(ncid); CHECK_ERR

    err = ncmpi_open(MPI_COMM_WORLD, filename, NC_NOWRITE, MPI_INFO_NULL, &ncid);
    CHECK_ERR

    err = ncmpi_inq_varid(ncid, "var", &varid); CHECK_ERR

    /* read back using the same access pattern */
    for (i=0; i<myNX; i++)
        for (j=0; j<NY; j++) buf[i][j] = (int)99;

    /* each proc reads myNX single columns of the 2D array */
    start[0]  = 0;   start[1] = rank;
    count[0]  = NY;  count[1] = 1;

    num_reqs = 0;
    for (i=0; i<myNX; i++) {
        err = ncmpi_iget_vara_int(ncid, varid, start, count, buf[i],
                                 &reqs[num_reqs++]); CHECK_ERR
        start[1] += nprocs;
    }
    /* this test is to see if cancelling free up all the internal malloc */
    err = ncmpi_cancel(ncid, num_reqs, reqs, sts); CHECK_ERR

    /* post iget requests again */
    start[1] = rank;
    num_reqs = 0;
    for (i=0; i<myNX; i++) {
        err = ncmpi_iget_vara_int(ncid, varid, start, count, buf[i],
                                 &reqs[num_reqs++]); CHECK_ERR
        start[1] += nprocs;
    }
    err = ncmpi_wait_all(ncid, num_reqs, reqs, sts); CHECK_ERR

    /* check status of all requests */
    for (i=0; i<num_reqs; i++)
        if (sts[i] != NC_NOERR) {
            printf("Error at line %d in %s: nonblocking write fails on request %d (%s)\n",
                   __LINE__,__FILE__,i, ncmpi_strerror(sts[i]));
            nerrs++;
        }

    for (i=0; i<myNX; i++) {
        for (j=0; j<NY; j++) {
            int expected = (int)rank+10;
            if (buf[i][j] != expected) {
                printf("Error at line %d in %s: expect buf[%d][%d]=%d but got %d\n",
                       __LINE__,__FILE__,i,j,expected,buf[i][j]);
                nerrs++;
            }
        }
    }

    err = ncmpi_close(ncid); CHECK_ERR

    free(sts);
    free(reqs);
    for (i=0; i<myNX; i++) free(buf[i]);
    free(buf);

    return nerrs;
}

static
int test_column_wise_uint(char *filename, int cdf)
{
    int i, j, nerrs=0, rank, nprocs, err, myNX, G_NX, myOff, num_reqs;
    int ncid, cmode, varid, dimid[2], *reqs, *sts;
    uint **buf;
    MPI_Offset start[2], count[2];

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    cmode = NC_CLOBBER;
    if (cdf == NC_FORMAT_CDF2)
        cmode |= NC_64BIT_OFFSET;
    else if (cdf == NC_FORMAT_CDF5)
        cmode |= NC_64BIT_DATA;
    err = ncmpi_create(MPI_COMM_WORLD, filename, cmode, MPI_INFO_NULL, &ncid);
    CHECK_ERR

    /* the global array is NY * (NX * nprocs) */
    G_NX  = NX * nprocs;
    myOff = NX * rank;
    myNX  = NX;

    err = ncmpi_def_dim(ncid, "Y", NY, &dimid[0]); CHECK_ERR
    err = ncmpi_def_dim(ncid, "X", G_NX, &dimid[1]); CHECK_ERR
    err = ncmpi_def_var(ncid, "var", NC_UINT, 2, dimid, &varid); CHECK_ERR
    err = ncmpi_enddef(ncid); CHECK_ERR

    /* First, fill the entire array with zeros, using a blocking I/O.
       Every process writes a subarray of size NY * myNX */
    buf    = (uint**) malloc(myNX * sizeof(uint*));
    buf[0] = (uint*)  calloc(NY * myNX, sizeof(uint));
    start[0] = 0;   start[1] = myOff;
    count[0] = NY;  count[1] = myNX;
    err = ncmpi_put_vara_uint_all(ncid, varid, start, count, buf[0]);
    free(buf[0]);

    /* initialize the buffer with rank ID. Also make the case interesting,
       by allocatsing buffersd separately */
    for (i=0; i<myNX; i++) {
        buf[i] = (uint*) malloc(NY * sizeof(uint));
        for (j=0; j<NY; j++) buf[i][j] = (uint)rank+10;
    }

    reqs = (int*) malloc(myNX * sizeof(int));
    sts  = (int*) malloc(myNX * sizeof(int));

    /* each proc writes myNX single columns of the 2D array */
    start[0]  = 0;   start[1] = rank;
    count[0]  = NY;  count[1] = 1;

    num_reqs = 0;
    for (i=0; i<myNX; i++) {
        err = ncmpi_iput_vara_uint(ncid, varid, start, count, buf[i],
                                 &reqs[num_reqs++]); CHECK_ERR
        start[1] += nprocs;
    }

    /* try re-order the request list */
    for (i=0; i<myNX/2; i++) {
        int tmp = reqs[2*i];
        reqs[2*i] = reqs[2*i+1];
        reqs[2*i+1] = tmp;
    }

    /* test cancelling requests and see if the user write buffer is properly
     * byte-swapped back to it original form. To do this test, NY must be
     * changed to use a number > NC_BYTE_SWAP_BUFFER_SIZE/sizeof(int), say
     * 1025
     */
    err = ncmpi_cancel(ncid, num_reqs, reqs, sts); CHECK_ERR

    /* check if write buffer contents have been altered after cancelling */
    for (i=0; i<myNX; i++) {
        for (j=0; j<NY; j++) {
            if (buf[i][j] != (uint)rank+10) {
                printf("Error at line %d in %s: put buffer altered buffer[%d][%d]=%u\n",
                       __LINE__,__FILE__,i,j,buf[i][j]);
                nerrs++;
            }
        }
    }

    /* post iput requests again */
    start[1] = rank;
    num_reqs = 0;
    for (i=0; i<myNX; i++) {
        err = ncmpi_iput_vara_uint(ncid, varid, start, count, buf[i],
                                 &reqs[num_reqs++]); CHECK_ERR
        start[1] += nprocs;
    }

    /* try re-order the request list */
    for (i=0; i<myNX/2; i++) {
        int tmp = reqs[2*i];
        reqs[2*i] = reqs[2*i+1];
        reqs[2*i+1] = tmp;
    }

    err = ncmpi_wait_all(ncid, num_reqs, reqs, sts); CHECK_ERR

    /* check if write buffer contents have been altered after wait */
    for (i=0; i<myNX; i++) {
        for (j=0; j<NY; j++) {
            if (buf[i][j] != (uint)rank+10) {
                printf("Error at line %d in %s: put buffer altered buffer[%d][%d]=%u\n",
                       __LINE__,__FILE__,i,j,buf[i][j]);
                nerrs++;
            }
        }
    }

    /* check status of all requests */
    for (i=0; i<num_reqs; i++) {
        if (reqs[i] != NC_REQ_NULL) { /* add in PnetCDF v1.7.0 */
            printf("Error at line %d in %s: request ID %d fails to be set to NC_REQ_NULL\n",__LINE__,__FILE__,i);
            nerrs++;
        }
        if (sts[i] != NC_NOERR) {
            printf("Error at line %d in %s: nonblocking write fails on request %d (%s)\n",
                   __LINE__,__FILE__,i, ncmpi_strerror(sts[i]));
            nerrs++;
        }
    }

    err = ncmpi_close(ncid); CHECK_ERR

    err = ncmpi_open(MPI_COMM_WORLD, filename, NC_NOWRITE, MPI_INFO_NULL, &ncid);
    CHECK_ERR

    err = ncmpi_inq_varid(ncid, "var", &varid); CHECK_ERR

    /* read back using the same access pattern */
    for (i=0; i<myNX; i++)
        for (j=0; j<NY; j++) buf[i][j] = (uint)99;

    /* each proc reads myNX single columns of the 2D array */
    start[0]  = 0;   start[1] = rank;
    count[0]  = NY;  count[1] = 1;

    num_reqs = 0;
    for (i=0; i<myNX; i++) {
        err = ncmpi_iget_vara_uint(ncid, varid, start, count, buf[i],
                                 &reqs[num_reqs++]); CHECK_ERR
        start[1] += nprocs;
    }
    /* this test is to see if cancelling free up all the internal malloc */
    err = ncmpi_cancel(ncid, num_reqs, reqs, sts); CHECK_ERR

    /* post iget requests again */
    start[1] = rank;
    num_reqs = 0;
    for (i=0; i<myNX; i++) {
        err = ncmpi_iget_vara_uint(ncid, varid, start, count, buf[i],
                                 &reqs[num_reqs++]); CHECK_ERR
        start[1] += nprocs;
    }
    err = ncmpi_wait_all(ncid, num_reqs, reqs, sts); CHECK_ERR

    /* check status of all requests */
    for (i=0; i<num_reqs; i++)
        if (sts[i] != NC_NOERR) {
            printf("Error at line %d in %s: nonblocking write fails on request %d (%s)\n",
                   __LINE__,__FILE__,i, ncmpi_strerror(sts[i]));
            nerrs++;
        }

    for (i=0; i<myNX; i++) {
        for (j=0; j<NY; j++) {
            uint expected = (uint)rank+10;
            if (buf[i][j] != expected) {
                printf("Error at line %d in %s: expect buf[%d][%d]=%u but got %u\n",
                       __LINE__,__FILE__,i,j,expected,buf[i][j]);
                nerrs++;
            }
        }
    }

    err = ncmpi_close(ncid); CHECK_ERR

    free(sts);
    free(reqs);
    for (i=0; i<myNX; i++) free(buf[i]);
    free(buf);

    return nerrs;
}

static
int test_column_wise_float(char *filename, int cdf)
{
    int i, j, nerrs=0, rank, nprocs, err, myNX, G_NX, myOff, num_reqs;
    int ncid, cmode, varid, dimid[2], *reqs, *sts;
    float **buf;
    MPI_Offset start[2], count[2];

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    cmode = NC_CLOBBER;
    if (cdf == NC_FORMAT_CDF2)
        cmode |= NC_64BIT_OFFSET;
    else if (cdf == NC_FORMAT_CDF5)
        cmode |= NC_64BIT_DATA;
    err = ncmpi_create(MPI_COMM_WORLD, filename, cmode, MPI_INFO_NULL, &ncid);
    CHECK_ERR

    /* the global array is NY * (NX * nprocs) */
    G_NX  = NX * nprocs;
    myOff = NX * rank;
    myNX  = NX;

    err = ncmpi_def_dim(ncid, "Y", NY, &dimid[0]); CHECK_ERR
    err = ncmpi_def_dim(ncid, "X", G_NX, &dimid[1]); CHECK_ERR
    err = ncmpi_def_var(ncid, "var", NC_FLOAT, 2, dimid, &varid); CHECK_ERR
    err = ncmpi_enddef(ncid); CHECK_ERR

    /* First, fill the entire array with zeros, using a blocking I/O.
       Every process writes a subarray of size NY * myNX */
    buf    = (float**) malloc(myNX * sizeof(float*));
    buf[0] = (float*)  calloc(NY * myNX, sizeof(float));
    start[0] = 0;   start[1] = myOff;
    count[0] = NY;  count[1] = myNX;
    err = ncmpi_put_vara_float_all(ncid, varid, start, count, buf[0]);
    free(buf[0]);

    /* initialize the buffer with rank ID. Also make the case interesting,
       by allocatsing buffersd separately */
    for (i=0; i<myNX; i++) {
        buf[i] = (float*) malloc(NY * sizeof(float));
        for (j=0; j<NY; j++) buf[i][j] = (float)rank+10;
    }

    reqs = (int*) malloc(myNX * sizeof(int));
    sts  = (int*) malloc(myNX * sizeof(int));

    /* each proc writes myNX single columns of the 2D array */
    start[0]  = 0;   start[1] = rank;
    count[0]  = NY;  count[1] = 1;

    num_reqs = 0;
    for (i=0; i<myNX; i++) {
        err = ncmpi_iput_vara_float(ncid, varid, start, count, buf[i],
                                 &reqs[num_reqs++]); CHECK_ERR
        start[1] += nprocs;
    }

    /* try re-order the request list */
    for (i=0; i<myNX/2; i++) {
        int tmp = reqs[2*i];
        reqs[2*i] = reqs[2*i+1];
        reqs[2*i+1] = tmp;
    }

    /* test cancelling requests and see if the user write buffer is properly
     * byte-swapped back to it original form. To do this test, NY must be
     * changed to use a number > NC_BYTE_SWAP_BUFFER_SIZE/sizeof(int), say
     * 1025
     */
    err = ncmpi_cancel(ncid, num_reqs, reqs, sts); CHECK_ERR

    /* check if write buffer contents have been altered after cancelling */
    for (i=0; i<myNX; i++) {
        for (j=0; j<NY; j++) {
            if (buf[i][j] != (float)rank+10) {
                printf("Error at line %d in %s: put buffer altered buffer[%d][%d]=%g\n",
                       __LINE__,__FILE__,i,j,buf[i][j]);
                nerrs++;
            }
        }
    }

    /* post iput requests again */
    start[1] = rank;
    num_reqs = 0;
    for (i=0; i<myNX; i++) {
        err = ncmpi_iput_vara_float(ncid, varid, start, count, buf[i],
                                 &reqs[num_reqs++]); CHECK_ERR
        start[1] += nprocs;
    }

    /* try re-order the request list */
    for (i=0; i<myNX/2; i++) {
        int tmp = reqs[2*i];
        reqs[2*i] = reqs[2*i+1];
        reqs[2*i+1] = tmp;
    }

    err = ncmpi_wait_all(ncid, num_reqs, reqs, sts); CHECK_ERR

    /* check if write buffer contents have been altered after wait */
    for (i=0; i<myNX; i++) {
        for (j=0; j<NY; j++) {
            if (buf[i][j] != (float)rank+10) {
                printf("Error at line %d in %s: put buffer altered buffer[%d][%d]=%g\n",
                       __LINE__,__FILE__,i,j,buf[i][j]);
                nerrs++;
            }
        }
    }

    /* check status of all requests */
    for (i=0; i<num_reqs; i++) {
        if (reqs[i] != NC_REQ_NULL) { /* add in PnetCDF v1.7.0 */
            printf("Error at line %d in %s: request ID %d fails to be set to NC_REQ_NULL\n",__LINE__,__FILE__,i);
            nerrs++;
        }
        if (sts[i] != NC_NOERR) {
            printf("Error at line %d in %s: nonblocking write fails on request %d (%s)\n",
                   __LINE__,__FILE__,i, ncmpi_strerror(sts[i]));
            nerrs++;
        }
    }

    err = ncmpi_close(ncid); CHECK_ERR

    err = ncmpi_open(MPI_COMM_WORLD, filename, NC_NOWRITE, MPI_INFO_NULL, &ncid);
    CHECK_ERR

    err = ncmpi_inq_varid(ncid, "var", &varid); CHECK_ERR

    /* read back using the same access pattern */
    for (i=0; i<myNX; i++)
        for (j=0; j<NY; j++) buf[i][j] = (float)99;

    /* each proc reads myNX single columns of the 2D array */
    start[0]  = 0;   start[1] = rank;
    count[0]  = NY;  count[1] = 1;

    num_reqs = 0;
    for (i=0; i<myNX; i++) {
        err = ncmpi_iget_vara_float(ncid, varid, start, count, buf[i],
                                 &reqs[num_reqs++]); CHECK_ERR
        start[1] += nprocs;
    }
    /* this test is to see if cancelling free up all the internal malloc */
    err = ncmpi_cancel(ncid, num_reqs, reqs, sts); CHECK_ERR

    /* post iget requests again */
    start[1] = rank;
    num_reqs = 0;
    for (i=0; i<myNX; i++) {
        err = ncmpi_iget_vara_float(ncid, varid, start, count, buf[i],
                                 &reqs[num_reqs++]); CHECK_ERR
        start[1] += nprocs;
    }
    err = ncmpi_wait_all(ncid, num_reqs, reqs, sts); CHECK_ERR

    /* check status of all requests */
    for (i=0; i<num_reqs; i++)
        if (sts[i] != NC_NOERR) {
            printf("Error at line %d in %s: nonblocking write fails on request %d (%s)\n",
                   __LINE__,__FILE__,i, ncmpi_strerror(sts[i]));
            nerrs++;
        }

    for (i=0; i<myNX; i++) {
        for (j=0; j<NY; j++) {
            float expected = (float)rank+10;
            if (buf[i][j] != expected) {
                printf("Error at line %d in %s: expect buf[%d][%d]=%g but got %g\n",
                       __LINE__,__FILE__,i,j,expected,buf[i][j]);
                nerrs++;
            }
        }
    }

    err = ncmpi_close(ncid); CHECK_ERR

    free(sts);
    free(reqs);
    for (i=0; i<myNX; i++) free(buf[i]);
    free(buf);

    return nerrs;
}

static
int test_column_wise_double(char *filename, int cdf)
{
    int i, j, nerrs=0, rank, nprocs, err, myNX, G_NX, myOff, num_reqs;
    int ncid, cmode, varid, dimid[2], *reqs, *sts;
    double **buf;
    MPI_Offset start[2], count[2];

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    cmode = NC_CLOBBER;
    if (cdf == NC_FORMAT_CDF2)
        cmode |= NC_64BIT_OFFSET;
    else if (cdf == NC_FORMAT_CDF5)
        cmode |= NC_64BIT_DATA;
    err = ncmpi_create(MPI_COMM_WORLD, filename, cmode, MPI_INFO_NULL, &ncid);
    CHECK_ERR

    /* the global array is NY * (NX * nprocs) */
    G_NX  = NX * nprocs;
    myOff = NX * rank;
    myNX  = NX;

    err = ncmpi_def_dim(ncid, "Y", NY, &dimid[0]); CHECK_ERR
    err = ncmpi_def_dim(ncid, "X", G_NX, &dimid[1]); CHECK_ERR
    err = ncmpi_def_var(ncid, "var", NC_DOUBLE, 2, dimid, &varid); CHECK_ERR
    err = ncmpi_enddef(ncid); CHECK_ERR

    /* First, fill the entire array with zeros, using a blocking I/O.
       Every process writes a subarray of size NY * myNX */
    buf    = (double**) malloc(myNX * sizeof(double*));
    buf[0] = (double*)  calloc(NY * myNX, sizeof(double));
    start[0] = 0;   start[1] = myOff;
    count[0] = NY;  count[1] = myNX;
    err = ncmpi_put_vara_double_all(ncid, varid, start, count, buf[0]);
    free(buf[0]);

    /* initialize the buffer with rank ID. Also make the case interesting,
       by allocatsing buffersd separately */
    for (i=0; i<myNX; i++) {
        buf[i] = (double*) malloc(NY * sizeof(double));
        for (j=0; j<NY; j++) buf[i][j] = (double)rank+10;
    }

    reqs = (int*) malloc(myNX * sizeof(int));
    sts  = (int*) malloc(myNX * sizeof(int));

    /* each proc writes myNX single columns of the 2D array */
    start[0]  = 0;   start[1] = rank;
    count[0]  = NY;  count[1] = 1;

    num_reqs = 0;
    for (i=0; i<myNX; i++) {
        err = ncmpi_iput_vara_double(ncid, varid, start, count, buf[i],
                                 &reqs[num_reqs++]); CHECK_ERR
        start[1] += nprocs;
    }

    /* try re-order the request list */
    for (i=0; i<myNX/2; i++) {
        int tmp = reqs[2*i];
        reqs[2*i] = reqs[2*i+1];
        reqs[2*i+1] = tmp;
    }

    /* test cancelling requests and see if the user write buffer is properly
     * byte-swapped back to it original form. To do this test, NY must be
     * changed to use a number > NC_BYTE_SWAP_BUFFER_SIZE/sizeof(int), say
     * 1025
     */
    err = ncmpi_cancel(ncid, num_reqs, reqs, sts); CHECK_ERR

    /* check if write buffer contents have been altered after cancelling */
    for (i=0; i<myNX; i++) {
        for (j=0; j<NY; j++) {
            if (buf[i][j] != (double)rank+10) {
                printf("Error at line %d in %s: put buffer altered buffer[%d][%d]=%g\n",
                       __LINE__,__FILE__,i,j,buf[i][j]);
                nerrs++;
            }
        }
    }

    /* post iput requests again */
    start[1] = rank;
    num_reqs = 0;
    for (i=0; i<myNX; i++) {
        err = ncmpi_iput_vara_double(ncid, varid, start, count, buf[i],
                                 &reqs[num_reqs++]); CHECK_ERR
        start[1] += nprocs;
    }

    /* try re-order the request list */
    for (i=0; i<myNX/2; i++) {
        int tmp = reqs[2*i];
        reqs[2*i] = reqs[2*i+1];
        reqs[2*i+1] = tmp;
    }

    err = ncmpi_wait_all(ncid, num_reqs, reqs, sts); CHECK_ERR

    /* check if write buffer contents have been altered after wait */
    for (i=0; i<myNX; i++) {
        for (j=0; j<NY; j++) {
            if (buf[i][j] != (double)rank+10) {
                printf("Error at line %d in %s: put buffer altered buffer[%d][%d]=%g\n",
                       __LINE__,__FILE__,i,j,buf[i][j]);
                nerrs++;
            }
        }
    }

    /* check status of all requests */
    for (i=0; i<num_reqs; i++) {
        if (reqs[i] != NC_REQ_NULL) { /* add in PnetCDF v1.7.0 */
            printf("Error at line %d in %s: request ID %d fails to be set to NC_REQ_NULL\n",__LINE__,__FILE__,i);
            nerrs++;
        }
        if (sts[i] != NC_NOERR) {
            printf("Error at line %d in %s: nonblocking write fails on request %d (%s)\n",
                   __LINE__,__FILE__,i, ncmpi_strerror(sts[i]));
            nerrs++;
        }
    }

    err = ncmpi_close(ncid); CHECK_ERR

    err = ncmpi_open(MPI_COMM_WORLD, filename, NC_NOWRITE, MPI_INFO_NULL, &ncid);
    CHECK_ERR

    err = ncmpi_inq_varid(ncid, "var", &varid); CHECK_ERR

    /* read back using the same access pattern */
    for (i=0; i<myNX; i++)
        for (j=0; j<NY; j++) buf[i][j] = (double)99;

    /* each proc reads myNX single columns of the 2D array */
    start[0]  = 0;   start[1] = rank;
    count[0]  = NY;  count[1] = 1;

    num_reqs = 0;
    for (i=0; i<myNX; i++) {
        err = ncmpi_iget_vara_double(ncid, varid, start, count, buf[i],
                                 &reqs[num_reqs++]); CHECK_ERR
        start[1] += nprocs;
    }
    /* this test is to see if cancelling free up all the internal malloc */
    err = ncmpi_cancel(ncid, num_reqs, reqs, sts); CHECK_ERR

    /* post iget requests again */
    start[1] = rank;
    num_reqs = 0;
    for (i=0; i<myNX; i++) {
        err = ncmpi_iget_vara_double(ncid, varid, start, count, buf[i],
                                 &reqs[num_reqs++]); CHECK_ERR
        start[1] += nprocs;
    }
    err = ncmpi_wait_all(ncid, num_reqs, reqs, sts); CHECK_ERR

    /* check status of all requests */
    for (i=0; i<num_reqs; i++)
        if (sts[i] != NC_NOERR) {
            printf("Error at line %d in %s: nonblocking write fails on request %d (%s)\n",
                   __LINE__,__FILE__,i, ncmpi_strerror(sts[i]));
            nerrs++;
        }

    for (i=0; i<myNX; i++) {
        for (j=0; j<NY; j++) {
            double expected = (double)rank+10;
            if (buf[i][j] != expected) {
                printf("Error at line %d in %s: expect buf[%d][%d]=%g but got %g\n",
                       __LINE__,__FILE__,i,j,expected,buf[i][j]);
                nerrs++;
            }
        }
    }

    err = ncmpi_close(ncid); CHECK_ERR

    free(sts);
    free(reqs);
    for (i=0; i<myNX; i++) free(buf[i]);
    free(buf);

    return nerrs;
}

static
int test_column_wise_longlong(char *filename, int cdf)
{
    int i, j, nerrs=0, rank, nprocs, err, myNX, G_NX, myOff, num_reqs;
    int ncid, cmode, varid, dimid[2], *reqs, *sts;
    longlong **buf;
    MPI_Offset start[2], count[2];

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    cmode = NC_CLOBBER;
    if (cdf == NC_FORMAT_CDF2)
        cmode |= NC_64BIT_OFFSET;
    else if (cdf == NC_FORMAT_CDF5)
        cmode |= NC_64BIT_DATA;
    err = ncmpi_create(MPI_COMM_WORLD, filename, cmode, MPI_INFO_NULL, &ncid);
    CHECK_ERR

    /* the global array is NY * (NX * nprocs) */
    G_NX  = NX * nprocs;
    myOff = NX * rank;
    myNX  = NX;

    err = ncmpi_def_dim(ncid, "Y", NY, &dimid[0]); CHECK_ERR
    err = ncmpi_def_dim(ncid, "X", G_NX, &dimid[1]); CHECK_ERR
    err = ncmpi_def_var(ncid, "var", NC_INT64, 2, dimid, &varid); CHECK_ERR
    err = ncmpi_enddef(ncid); CHECK_ERR

    /* First, fill the entire array with zeros, using a blocking I/O.
       Every process writes a subarray of size NY * myNX */
    buf    = (longlong**) malloc(myNX * sizeof(longlong*));
    buf[0] = (longlong*)  calloc(NY * myNX, sizeof(longlong));
    start[0] = 0;   start[1] = myOff;
    count[0] = NY;  count[1] = myNX;
    err = ncmpi_put_vara_longlong_all(ncid, varid, start, count, buf[0]);
    free(buf[0]);

    /* initialize the buffer with rank ID. Also make the case interesting,
       by allocatsing buffersd separately */
    for (i=0; i<myNX; i++) {
        buf[i] = (longlong*) malloc(NY * sizeof(longlong));
        for (j=0; j<NY; j++) buf[i][j] = (longlong)rank+10;
    }

    reqs = (int*) malloc(myNX * sizeof(int));
    sts  = (int*) malloc(myNX * sizeof(int));

    /* each proc writes myNX single columns of the 2D array */
    start[0]  = 0;   start[1] = rank;
    count[0]  = NY;  count[1] = 1;

    num_reqs = 0;
    for (i=0; i<myNX; i++) {
        err = ncmpi_iput_vara_longlong(ncid, varid, start, count, buf[i],
                                 &reqs[num_reqs++]); CHECK_ERR
        start[1] += nprocs;
    }

    /* try re-order the request list */
    for (i=0; i<myNX/2; i++) {
        int tmp = reqs[2*i];
        reqs[2*i] = reqs[2*i+1];
        reqs[2*i+1] = tmp;
    }

    /* test cancelling requests and see if the user write buffer is properly
     * byte-swapped back to it original form. To do this test, NY must be
     * changed to use a number > NC_BYTE_SWAP_BUFFER_SIZE/sizeof(int), say
     * 1025
     */
    err = ncmpi_cancel(ncid, num_reqs, reqs, sts); CHECK_ERR

    /* check if write buffer contents have been altered after cancelling */
    for (i=0; i<myNX; i++) {
        for (j=0; j<NY; j++) {
            if (buf[i][j] != (longlong)rank+10) {
                printf("Error at line %d in %s: put buffer altered buffer[%d][%d]=%lld\n",
                       __LINE__,__FILE__,i,j,buf[i][j]);
                nerrs++;
            }
        }
    }

    /* post iput requests again */
    start[1] = rank;
    num_reqs = 0;
    for (i=0; i<myNX; i++) {
        err = ncmpi_iput_vara_longlong(ncid, varid, start, count, buf[i],
                                 &reqs[num_reqs++]); CHECK_ERR
        start[1] += nprocs;
    }

    /* try re-order the request list */
    for (i=0; i<myNX/2; i++) {
        int tmp = reqs[2*i];
        reqs[2*i] = reqs[2*i+1];
        reqs[2*i+1] = tmp;
    }

    err = ncmpi_wait_all(ncid, num_reqs, reqs, sts); CHECK_ERR

    /* check if write buffer contents have been altered after wait */
    for (i=0; i<myNX; i++) {
        for (j=0; j<NY; j++) {
            if (buf[i][j] != (longlong)rank+10) {
                printf("Error at line %d in %s: put buffer altered buffer[%d][%d]=%lld\n",
                       __LINE__,__FILE__,i,j,buf[i][j]);
                nerrs++;
            }
        }
    }

    /* check status of all requests */
    for (i=0; i<num_reqs; i++) {
        if (reqs[i] != NC_REQ_NULL) { /* add in PnetCDF v1.7.0 */
            printf("Error at line %d in %s: request ID %d fails to be set to NC_REQ_NULL\n",__LINE__,__FILE__,i);
            nerrs++;
        }
        if (sts[i] != NC_NOERR) {
            printf("Error at line %d in %s: nonblocking write fails on request %d (%s)\n",
                   __LINE__,__FILE__,i, ncmpi_strerror(sts[i]));
            nerrs++;
        }
    }

    err = ncmpi_close(ncid); CHECK_ERR

    err = ncmpi_open(MPI_COMM_WORLD, filename, NC_NOWRITE, MPI_INFO_NULL, &ncid);
    CHECK_ERR

    err = ncmpi_inq_varid(ncid, "var", &varid); CHECK_ERR

    /* read back using the same access pattern */
    for (i=0; i<myNX; i++)
        for (j=0; j<NY; j++) buf[i][j] = (longlong)99;

    /* each proc reads myNX single columns of the 2D array */
    start[0]  = 0;   start[1] = rank;
    count[0]  = NY;  count[1] = 1;

    num_reqs = 0;
    for (i=0; i<myNX; i++) {
        err = ncmpi_iget_vara_longlong(ncid, varid, start, count, buf[i],
                                 &reqs[num_reqs++]); CHECK_ERR
        start[1] += nprocs;
    }
    /* this test is to see if cancelling free up all the internal malloc */
    err = ncmpi_cancel(ncid, num_reqs, reqs, sts); CHECK_ERR

    /* post iget requests again */
    start[1] = rank;
    num_reqs = 0;
    for (i=0; i<myNX; i++) {
        err = ncmpi_iget_vara_longlong(ncid, varid, start, count, buf[i],
                                 &reqs[num_reqs++]); CHECK_ERR
        start[1] += nprocs;
    }
    err = ncmpi_wait_all(ncid, num_reqs, reqs, sts); CHECK_ERR

    /* check status of all requests */
    for (i=0; i<num_reqs; i++)
        if (sts[i] != NC_NOERR) {
            printf("Error at line %d in %s: nonblocking write fails on request %d (%s)\n",
                   __LINE__,__FILE__,i, ncmpi_strerror(sts[i]));
            nerrs++;
        }

    for (i=0; i<myNX; i++) {
        for (j=0; j<NY; j++) {
            longlong expected = (longlong)rank+10;
            if (buf[i][j] != expected) {
                printf("Error at line %d in %s: expect buf[%d][%d]=%lld but got %lld\n",
                       __LINE__,__FILE__,i,j,expected,buf[i][j]);
                nerrs++;
            }
        }
    }

    err = ncmpi_close(ncid); CHECK_ERR

    free(sts);
    free(reqs);
    for (i=0; i<myNX; i++) free(buf[i]);
    free(buf);

    return nerrs;
}

static
int test_column_wise_ulonglong(char *filename, int cdf)
{
    int i, j, nerrs=0, rank, nprocs, err, myNX, G_NX, myOff, num_reqs;
    int ncid, cmode, varid, dimid[2], *reqs, *sts;
    ulonglong **buf;
    MPI_Offset start[2], count[2];

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    cmode = NC_CLOBBER;
    if (cdf == NC_FORMAT_CDF2)
        cmode |= NC_64BIT_OFFSET;
    else if (cdf == NC_FORMAT_CDF5)
        cmode |= NC_64BIT_DATA;
    err = ncmpi_create(MPI_COMM_WORLD, filename, cmode, MPI_INFO_NULL, &ncid);
    CHECK_ERR

    /* the global array is NY * (NX * nprocs) */
    G_NX  = NX * nprocs;
    myOff = NX * rank;
    myNX  = NX;

    err = ncmpi_def_dim(ncid, "Y", NY, &dimid[0]); CHECK_ERR
    err = ncmpi_def_dim(ncid, "X", G_NX, &dimid[1]); CHECK_ERR
    err = ncmpi_def_var(ncid, "var", NC_UINT64, 2, dimid, &varid); CHECK_ERR
    err = ncmpi_enddef(ncid); CHECK_ERR

    /* First, fill the entire array with zeros, using a blocking I/O.
       Every process writes a subarray of size NY * myNX */
    buf    = (ulonglong**) malloc(myNX * sizeof(ulonglong*));
    buf[0] = (ulonglong*)  calloc(NY * myNX, sizeof(ulonglong));
    start[0] = 0;   start[1] = myOff;
    count[0] = NY;  count[1] = myNX;
    err = ncmpi_put_vara_ulonglong_all(ncid, varid, start, count, buf[0]);
    free(buf[0]);

    /* initialize the buffer with rank ID. Also make the case interesting,
       by allocatsing buffersd separately */
    for (i=0; i<myNX; i++) {
        buf[i] = (ulonglong*) malloc(NY * sizeof(ulonglong));
        for (j=0; j<NY; j++) buf[i][j] = (ulonglong)rank+10;
    }

    reqs = (int*) malloc(myNX * sizeof(int));
    sts  = (int*) malloc(myNX * sizeof(int));

    /* each proc writes myNX single columns of the 2D array */
    start[0]  = 0;   start[1] = rank;
    count[0]  = NY;  count[1] = 1;

    num_reqs = 0;
    for (i=0; i<myNX; i++) {
        err = ncmpi_iput_vara_ulonglong(ncid, varid, start, count, buf[i],
                                 &reqs[num_reqs++]); CHECK_ERR
        start[1] += nprocs;
    }

    /* try re-order the request list */
    for (i=0; i<myNX/2; i++) {
        int tmp = reqs[2*i];
        reqs[2*i] = reqs[2*i+1];
        reqs[2*i+1] = tmp;
    }

    /* test cancelling requests and see if the user write buffer is properly
     * byte-swapped back to it original form. To do this test, NY must be
     * changed to use a number > NC_BYTE_SWAP_BUFFER_SIZE/sizeof(int), say
     * 1025
     */
    err = ncmpi_cancel(ncid, num_reqs, reqs, sts); CHECK_ERR

    /* check if write buffer contents have been altered after cancelling */
    for (i=0; i<myNX; i++) {
        for (j=0; j<NY; j++) {
            if (buf[i][j] != (ulonglong)rank+10) {
                printf("Error at line %d in %s: put buffer altered buffer[%d][%d]=%llu\n",
                       __LINE__,__FILE__,i,j,buf[i][j]);
                nerrs++;
            }
        }
    }

    /* post iput requests again */
    start[1] = rank;
    num_reqs = 0;
    for (i=0; i<myNX; i++) {
        err = ncmpi_iput_vara_ulonglong(ncid, varid, start, count, buf[i],
                                 &reqs[num_reqs++]); CHECK_ERR
        start[1] += nprocs;
    }

    /* try re-order the request list */
    for (i=0; i<myNX/2; i++) {
        int tmp = reqs[2*i];
        reqs[2*i] = reqs[2*i+1];
        reqs[2*i+1] = tmp;
    }

    err = ncmpi_wait_all(ncid, num_reqs, reqs, sts); CHECK_ERR

    /* check if write buffer contents have been altered after wait */
    for (i=0; i<myNX; i++) {
        for (j=0; j<NY; j++) {
            if (buf[i][j] != (ulonglong)rank+10) {
                printf("Error at line %d in %s: put buffer altered buffer[%d][%d]=%llu\n",
                       __LINE__,__FILE__,i,j,buf[i][j]);
                nerrs++;
            }
        }
    }

    /* check status of all requests */
    for (i=0; i<num_reqs; i++) {
        if (reqs[i] != NC_REQ_NULL) { /* add in PnetCDF v1.7.0 */
            printf("Error at line %d in %s: request ID %d fails to be set to NC_REQ_NULL\n",__LINE__,__FILE__,i);
            nerrs++;
        }
        if (sts[i] != NC_NOERR) {
            printf("Error at line %d in %s: nonblocking write fails on request %d (%s)\n",
                   __LINE__,__FILE__,i, ncmpi_strerror(sts[i]));
            nerrs++;
        }
    }

    err = ncmpi_close(ncid); CHECK_ERR

    err = ncmpi_open(MPI_COMM_WORLD, filename, NC_NOWRITE, MPI_INFO_NULL, &ncid);
    CHECK_ERR

    err = ncmpi_inq_varid(ncid, "var", &varid); CHECK_ERR

    /* read back using the same access pattern */
    for (i=0; i<myNX; i++)
        for (j=0; j<NY; j++) buf[i][j] = (ulonglong)99;

    /* each proc reads myNX single columns of the 2D array */
    start[0]  = 0;   start[1] = rank;
    count[0]  = NY;  count[1] = 1;

    num_reqs = 0;
    for (i=0; i<myNX; i++) {
        err = ncmpi_iget_vara_ulonglong(ncid, varid, start, count, buf[i],
                                 &reqs[num_reqs++]); CHECK_ERR
        start[1] += nprocs;
    }
    /* this test is to see if cancelling free up all the internal malloc */
    err = ncmpi_cancel(ncid, num_reqs, reqs, sts); CHECK_ERR

    /* post iget requests again */
    start[1] = rank;
    num_reqs = 0;
    for (i=0; i<myNX; i++) {
        err = ncmpi_iget_vara_ulonglong(ncid, varid, start, count, buf[i],
                                 &reqs[num_reqs++]); CHECK_ERR
        start[1] += nprocs;
    }
    err = ncmpi_wait_all(ncid, num_reqs, reqs, sts); CHECK_ERR

    /* check status of all requests */
    for (i=0; i<num_reqs; i++)
        if (sts[i] != NC_NOERR) {
            printf("Error at line %d in %s: nonblocking write fails on request %d (%s)\n",
                   __LINE__,__FILE__,i, ncmpi_strerror(sts[i]));
            nerrs++;
        }

    for (i=0; i<myNX; i++) {
        for (j=0; j<NY; j++) {
            ulonglong expected = (ulonglong)rank+10;
            if (buf[i][j] != expected) {
                printf("Error at line %d in %s: expect buf[%d][%d]=%llu but got %llu\n",
                       __LINE__,__FILE__,i,j,expected,buf[i][j]);
                nerrs++;
            }
        }
    }

    err = ncmpi_close(ncid); CHECK_ERR

    free(sts);
    free(reqs);
    for (i=0; i<myNX; i++) free(buf[i]);
    free(buf);

    return nerrs;
}


int main(int argc, char** argv)
{
    extern int optind;
    char filename[256];
    int i, nerrs=0, rank, err;
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
        sprintf(cmd_str, "*** TESTING C   %s for iput/iget interleaved access ", basename(argv[0]));
        printf("%-66s ------ ", cmd_str);
        free(cmd_str);
    }

    for (i=0; i<3; i++) {
        nerrs += test_column_wise_text(filename, cdf_formats[i]);
        nerrs += test_column_wise_schar(filename, cdf_formats[i]);
        nerrs += test_column_wise_short(filename, cdf_formats[i]);
        nerrs += test_column_wise_int(filename, cdf_formats[i]);
        nerrs += test_column_wise_float(filename, cdf_formats[i]);
        nerrs += test_column_wise_double(filename, cdf_formats[i]);
        if (cdf_formats[i] == NC_FORMAT_CDF5) {
            nerrs += test_column_wise_uchar(filename, cdf_formats[i]);
            nerrs += test_column_wise_ushort(filename, cdf_formats[i]);
            nerrs += test_column_wise_uint(filename, cdf_formats[i]);
            nerrs += test_column_wise_longlong(filename, cdf_formats[i]);
            nerrs += test_column_wise_ulonglong(filename, cdf_formats[i]);
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

