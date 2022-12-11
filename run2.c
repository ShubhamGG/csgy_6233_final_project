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

#define MIN_TIME 5
#define MAX_TIME 15

char filename[255];
size_t blksz, blkcnt;
int mfd;
uint fxor = 0;
struct stat fstatbuf;
clock_t t1, t2;
struct tms clk1, clk2;

void incorr_usage(char *msg)
{
    printf("Error: %s\n"
           "Usage: run2 <filename> <block_size>\n",
           msg);
    exit(1);
}

void process_args(int argc, char const *argv[])
{
    int opt = 1, temp;
    if (argc != 3)
        incorr_usage(ERR_INV_PARAMS);
    strcpy(filename, argv[opt++]);
    if (stat(filename, &fstatbuf) < 0)
    {
        perror(NULL);
        exit(errno);
    }
    errno = 0;
    blksz = strtol(argv[opt++], NULL, 10);
    if (errno != 0 || blksz < 1)
        incorr_usage(ERR_INV_PARAMS);
    VERBOSE printf("params:\t%s, %lu\n", filename, blksz);
    INFO printf("Input file size: %ld\n", fstatbuf.st_size);
    blkcnt = fstatbuf.st_size / blksz;
    INFO printf("Blocks to read: %lu\n", blkcnt);
}

void op2_read()
{
    uint i, rounds = 1;
    clock_t realtime = 0, usertime = 0, systime = 0;
    const double tick_to_s = (double)sysconf(_SC_CLK_TCK);
    size_t cnt = 0, totalcnt = 0;
    uint *buf = (unsigned int *)malloc(blksz);
    VERBOSE printf("read started.\n");
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
        for (i = 0; i < blksz / sizeof(int); i++)
            fxor ^= buf[i];
    }
    t2 = times(&clk2);
    if (t1 <= t2)
        realtime += t2 - t1;
    else
        realtime += LONG_MAX - t1 + t2; // handle wraparound of clocktime
    usertime += clk2.tms_utime - clk1.tms_utime;
    systime += clk2.tms_stime - clk1.tms_stime;
    if (realtime < 5 * tick_to_s)
    { // If we don't reach total time, we read through from start again
        rounds++;
        lseek(mfd, 0, SEEK_SET);
        goto read_again;
    }
    printf("Bytes Read: %ld bytes.\n", blksz * blkcnt * rounds);
    if (rounds > 1)
        printf("Number of read rounds: %u\n", rounds);
    INFO printf("XOR: %u\n", fxor);
    VERBOSE printf("XOR(hex): %x\n", fxor);
    printf("Wall Time: %f\n", realtime / tick_to_s);
    printf("CPU time: %f\n", (usertime / tick_to_s) + (systime / tick_to_s));
    printf("Performance: %f MiB/sec\n", ((blksz * blkcnt * rounds) / (realtime / tick_to_s)) / 1048576);
}

int main(int argc, char const *argv[])
{
    process_args(argc, argv);
    mfd = open(filename, O_RDONLY);
    if (mfd < 0)
    {
        perror(NULL);
        exit(errno);
    }
    op2_read();
    close(mfd);

    return 0;
}
