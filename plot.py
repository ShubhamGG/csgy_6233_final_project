#!/usr/bin/python3

import matplotlib.pyplot as plt
import csv

FIG_DPI = 200
fig1 = plt.figure(num='Raw Performance', dpi=FIG_DPI)
fig2 = plt.figure(num='System Calls', dpi=FIG_DPI)

# Part 3: Raw Performance X->block size(B) vs Y->perf(MiB/s)
def part3():
    blocks = []
    bytesread = []
    runtime = []
    with open("output_cache.csv", 'r') as cf:
        csvr = csv.reader(cf)
        for row in csvr:
            blocks.append(int(row[0]))
            bytesread.append(int(row[1]))
            runtime.append(float(row[2]))

    perf = map(lambda b, t: b/(t*1048576), bytesread, runtime)
    perf = list(perf)
    print("**** Part 3: Raw Performance ****")
    print("Blocks(B)\tPerf(MiB/s)")
    for xy in zip(blocks, perf):
        print("%d\t\t%f" % xy)
    blocks = [r/1048576 for r in blocks]

    plt.figure(fig1)
    plt.xlabel("Block Size (MiB)")
    plt.ylabel("Performance (MiB/s)")

    plt.plot(blocks, perf, 'r.-', label="Cached")
    # plt.xticks(blocks, fontsize=10)
    # plt.xscale('log')
    # plt.legend()
    plt.grid()
    plt.tight_layout()
    # plt.show()
    plt.savefig('./fig_part3.png')

# Part 5: System Calls | read & lseek | X->time(sec) | Y->#syscalls
def part5():
    calls = []
    readtime = []
    lseektime = []
    with open("output_syscall_read.csv", 'r') as cf:
        for row in csv.reader(cf):
            calls.append(int(row[0]))
            readtime.append(float(row[1]))
    with open("output_syscall_lseek.csv", 'r') as cf:
        for row in csv.reader(cf):
            lseektime.append(float(row[1]))

    # calls = calls[:-3]
    # readtime = readtime[:-3]
    # lseektime = lseektime[:-3]
    print("**** Part 5: SysCall Performance ****")
    print("Calls\tread()\tlseek()")
    for xy in zip(calls, readtime, lseektime):
        print("%d\t%f\t%f" % xy)
    calls = [r/1e6 for r in calls]

    plt.figure(fig2)
    plt.xlabel("Number of Calls (million)")
    plt.ylabel("Time Taken (s)")

    plt.plot(calls, readtime, 'r.-', label="read()")
    plt.plot(calls, lseektime, 'b.-', label="lseek()")

    # plt.xticks(blocks, fontsize=10)
    # plt.xscale('log')
    plt.legend()
    plt.grid()
    plt.tight_layout()
    # plt.show()
    plt.savefig('./fig_part5.png')

part3()
part5()
