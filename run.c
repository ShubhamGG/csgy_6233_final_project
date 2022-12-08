#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <malloc.h>

#define DEBUG 2
#define INFO if (DEBUG >= 1)
#define VERBOSE if (DEBUG >= 2)

#define ERR_INV_PARAMS "Invalid Parameters"
#define ERR_FILE_SZ "File size insufficient"

char filename[255];
size_t blksz, blkcnt;
int mfd, mflags = 0;
const mode_t mmode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
uint fxor = 0;
struct stat fstatbuf;

void incorr_usage(char *msg)
{
    printf("Error: %s\n"
           "Usage: run <filename> [-r|-w] <block_size> <block_count>\n",
           msg);
    exit(1);
}

void process_args(int argc, char const *argv[])
{
    int opt = 1, temp;

    if (argc < 4 || argc > 5)
        incorr_usage(ERR_INV_PARAMS);
    strcpy(filename, argv[opt++]);
    if (argv[opt][0] == '-')
    {
        switch (argv[opt][1])
        {
        case 'w':
            mflags |= O_WRONLY | O_CREAT | O_TRUNC;
            break;
        case 'r':
            mflags |= O_RDONLY;
            if (stat(filename, &fstatbuf) < 0)
            {
                perror(NULL);
                exit(errno);
            }
            break;
        default:
            incorr_usage(ERR_INV_PARAMS);
        }
        opt++;
    }
    else
        mflags |= O_RDONLY;
    errno = 0;
    blksz = strtol(argv[opt++], NULL, 10);
    blkcnt = strtol(argv[opt], NULL, 10);
    if (errno != 0 || blksz < 1 || blkcnt < 1)
        incorr_usage(ERR_INV_PARAMS);
    // confirm sufficient file size
    if (mflags & O_RDONLY && fstatbuf.st_size <= blksz * blkcnt)
        incorr_usage(ERR_FILE_SZ);
    INFO printf("params:\t%s, %lu, %lu\n", filename, blksz, blkcnt);
    VERBOSE printf("optimal block size: %ld\n", fstatbuf.st_blksize);
}

void op_read()
{
    uint res = 0, *p, i;
    size_t cnt = 0;
    uint *buf = (unsigned int *)malloc(blksz);
    INFO printf("read started.\n");
    for (cnt = 0; cnt < blkcnt; cnt++)
    {
        if (read(mfd, buf, blksz) < 0)
        {
            perror(NULL);
            exit(errno);
        }
        VERBOSE printf("cnt: %lu\r", cnt);
        // xorcalc
        for (i = 0; i < blksz / sizeof(unsigned int); i++)
            fxor ^= buf[i];
    }
    VERBOSE printf("\n");
    INFO printf("read finished.\n");
    printf("XOR: %u\n", fxor);
    INFO printf("XOR(hex): %x\n", fxor);
}

void op_write()
{
    size_t cnt;
    unsigned int *buf = (unsigned int *)malloc(blksz);
    for (cnt = 0; cnt < blksz / sizeof(int); cnt++)
        buf[cnt] = rand();
    INFO printf("write started.\n");
    for (cnt = 0; cnt < blkcnt; cnt++)
    {
        if (write(mfd, buf, blksz) < 0)
        {
            perror(NULL);
            exit(errno);
        }
        VERBOSE printf("cnt: %lu\r", cnt);
    }
    INFO printf("write finished.\n");
}

int main(int argc, char const *argv[])
{
    process_args(argc, argv);
    mfd = open(filename, mflags, mmode);
    if (mfd < 0)
    {
        perror(NULL);
        exit(errno);
    }
    if (mflags & O_WRONLY)
        op_write();
    else
        op_read();
    close(mfd);

    return 0;
}
