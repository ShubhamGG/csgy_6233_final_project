#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/times.h>
#include "utils.h"
#include <limits.h>

#define DEBUG 0

#define CALLNUM_START 4096
#define CALLNUM_MAX 80000000
#define CSV_READ "output_syscall_read.csv"
#define CSV_LSEEK "output_syscall_lseek.csv"

char filename[255];
int mfd;
clock_t t1, t2;
struct tms clk1, clk2;
double tick_to_s;

void incorr_usage(char *msg)
{
    printf("Error: %s\n"
           "Usage: run2 <filename>\n",
           msg);
    exit(1);
}

void process_args(int argc, char const *argv[])
{
    struct stat fstatbuf;
    if (argc != 2)
        incorr_usage(ERR_INV_PARAMS);
    strcpy(filename, argv[1]);
    if (stat(filename, &fstatbuf) < 0)
    {
        perror(NULL);
        exit(errno);
    }
    VERBOSE printf("params:\t%s\n", filename);
    INFO printf("Input file size: %ld\n", fstatbuf.st_size);
    if (fstatbuf.st_size < CALLNUM_MAX)
        incorr_usage(ERR_FILE_SZ);
}

clock_t op5_read(size_t ncall)
{
    uint i;
    clock_t realtime = 0, usertime = 0, systime = 0;
    size_t cnt = 0;
    uint *buf = (unsigned int *)malloc(sizeof(int));
    t1 = times(&clk1);
    for (cnt = 0; cnt < ncall; cnt++)
    {
        if (read(mfd, buf, 1) < 0)
        {
            perror(NULL);
            exit(errno);
        }
    }
    t2 = times(&clk2);
    if (t1 <= t2)
        realtime += t2 - t1;
    else
        realtime += LONG_MAX - t1 + t2; // handle wraparound of clocktime
    usertime += clk2.tms_utime - clk1.tms_utime;
    systime += clk2.tms_stime - clk1.tms_stime;
    VERBOSE printf("Wall Time:\t%f\n", realtime / tick_to_s);
    VERBOSE printf("CPU time:\t%f\n", (usertime / tick_to_s) + (systime / tick_to_s));
    printf("read() Calls: %lu\tsyscalls/sec: %f\tperf: %f MiB/s\n", ncall, ncall / (realtime / tick_to_s), ncall / (realtime / tick_to_s) / 1048576);
    return realtime;
}

clock_t op5_lseek(size_t ncall)
{
    uint i;
    clock_t realtime = 0, usertime = 0, systime = 0;
    size_t cnt = 0, totalcnt = 0;
    lseek(mfd, 0, SEEK_SET);
    t1 = times(&clk1);
    for (cnt = 0; cnt < ncall; cnt++)
    {
        lseek(mfd, 1, SEEK_CUR);
    }
    t2 = times(&clk2);
    if (t1 <= t2)
        realtime += t2 - t1;
    else
        realtime += LONG_MAX - t1 + t2; // handle wraparound of clocktime
    usertime += clk2.tms_utime - clk1.tms_utime;
    systime += clk2.tms_stime - clk1.tms_stime;
    VERBOSE printf("Wall Time:\t%f\n", realtime / tick_to_s);
    VERBOSE printf("CPU time:\t%f\n", (usertime / tick_to_s) + (systime / tick_to_s));
    printf("lseek() Calls: %lu\tsyscalls/sec: %f\n", ncall, ncall / (realtime / tick_to_s));
    return realtime;
}

int main(int argc, char const *argv[])
{
    FILE *csvread, *csvlseek;
    size_t ncall = CALLNUM_START;
    process_args(argc, argv);
    tick_to_s = (double)sysconf(_SC_CLK_TCK);
    mfd = open(filename, O_RDONLY);
    if (mfd < 0)
    {
        perror(NULL);
        exit(errno);
    }
    csvread = fopen(CSV_READ, "w");
    csvlseek = fopen(CSV_LSEEK, "w");
    for (ncall = CALLNUM_START; ncall < CALLNUM_MAX; ncall *= 2)
    {
        fprintf(csvread, "%lu,%f\n", ncall, op5_read(ncall) / tick_to_s);
    }
    for (ncall = CALLNUM_START; ncall < CALLNUM_MAX; ncall *= 2)
    {
        fprintf(csvlseek, "%lu,%f\n", ncall, op5_lseek(ncall) / tick_to_s);
    }

    close(mfd);
    fclose(csvlseek);
    fclose(csvread);

    return 0;
}
