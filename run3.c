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

#define DEBUG 2

#define MIN_TIME 5
#define MAX_TIME 15

#define BLKSZ_START 128
#define CSV_FILENAME "output_cache.csv"

char filename[255];
int mfd;
FILE *mcsv;
uint fxor = 0;
struct stat fstatbuf;
clock_t t1, t2;
struct tms clk1, clk2;

void incorr_usage(char *msg)
{
    printf("Error: %s\n"
           "Usage: run2 <filename>\n",
           msg);
    exit(1);
}

void process_args(int argc, char const *argv[])
{
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
}

void op3_read(size_t blksz)
{
    uint i, rounds = 1;
    ulong realtime = 0;
    const double tick_to_s = (double)sysconf(_SC_CLK_TCK);
    size_t cnt = 0, blkcnt = fstatbuf.st_size/blksz;
    uint *buf = (unsigned int *)malloc(blksz);
    VERBOSE printf("read started: %lu.\n", blksz);
read_again:
    t1 = times(&clk1);
    for (cnt = 0; cnt < blkcnt; cnt++)
    {
        if (read(mfd, buf, blksz) < 0)
        {
            perror(NULL);
            exit(errno);
        }
        // xorcalc
        for (i = 0; i < blksz / sizeof(unsigned int); i++)
            fxor ^= buf[i];
    }
    t2 = times(&clk2);
    if (t1 <= t2)
        realtime += t2 - t1;
    else
        realtime += LONG_MAX - t1 + t2; // handle wraparound of clocktime
    // usertime += clk2.tms_utime - clk1.tms_utime;
    // systime += clk2.tms_stime - clk1.tms_stime;
    // printf("Bytes Read: %ld bytes.\n", blksz * blkcnt * rounds);
    if (realtime < MIN_TIME * tick_to_s)
    { // If we don't reach required min time, we read through from start again
        rounds++;
        lseek(mfd, 0, SEEK_SET);
        goto read_again;
    }
    if (rounds > 1)
        VERBOSE printf("Number of times file read: %u\n", rounds);
    INFO printf("XOR: %u\n", fxor);
    INFO printf("Wall Time: %f\n", realtime / tick_to_s);
    // printf("CPU time: %f\n", (usertime / tick_to_s) + (systime / tick_to_s));
    printf("Block: %lu\tPerf: %f MiB/sec\n", blksz, ((blksz * blkcnt * rounds) / (realtime / tick_to_s)) / 1048576);
    fprintf(mcsv, "%ld,%ld,%f\n", blksz, blksz * blkcnt * rounds, realtime / tick_to_s);
}

int main(int argc, char const *argv[])
{
    size_t blksz = BLKSZ_START;
    process_args(argc, argv);
    mfd = open(filename, O_RDONLY);
    mcsv = fopen(CSV_FILENAME, "w");
    if (mfd < 0 || mcsv == 0)
    {
        perror(NULL);
        exit(errno);
    }
    while (blksz < 3000000)
    {
        lseek(mfd, 0, SEEK_SET);
        op3_read(blksz);
        blksz *= 2;
    }
    fclose(mcsv);
    close(mfd);

    return 0;
}
