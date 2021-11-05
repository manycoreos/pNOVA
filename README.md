# pNOVA: parallel NOn-Volatile memory Accelerated log-structured file system

## Description
NOVA is a state-of-the-art NVM file system, ensures higher throughput and lower read-write latency than block-based file systems. NOVA also ensures consistency of file data and metadata through its log- structured design. NOVA adopts per-inode logging which logs metadata for every write operation. However, NOVA does not provide any degree of scalability in terms of I/O throughput when concurrent shared file I/Os are performed. This is mainly due to the coarse-grained locks on inodes to guarantee consistency of its per-inode logs, which negates the benefits of concurrent nature of NOVA and high-performance NVM devices. also NOVA does not scale Non-Uniform Memory Access(NUMA) architecture. This is because NOVA is not designed for NUMA environment.

Solving this scalability issue, pNOVA is a variant of NOVA that not only accelerates parallel writes and reads to the same file of multiple threads but also aware NUMA to show the scalability on NUMA manycore servers. The base of pNOVA is [NOVA][NOVA] v5.1 from NVSL. The technologies applied to pNOVA are as follows : 

### Fine-grained range RW lock
To solve this coarse-grained lock problem, we suggest a range-based reader-writer synchronization mechanism, which selectively blocks I/O operations only with overlapped ranges with lock holders,

### Virtualizing NVM Devices
In order to store files across multiple NUMA nodes, the non-contiguous physical address space of NVMs located at multiple nodes is virtualized into one logical address space. Applied a local first write policy to place file data and metadata preferentially on the NVM device allocated to the CPU where the thread is executing.

### Lock-Free Per-Core Data Structures
lock- free per-core data structures such as per-inode log using Global Log and Local Log to ensure the scalability by allowing multiple threads to perform write operations concurrently.

## Requirement Environment
This version is written for a 56-core server equipped with two NVMs. If you want to use it in an environment with a different number of NVMs and a different number of cores, you need to modify the following code. All files that need to be modified are below fs/nova/. If you want to emulate because you don't have actual PM equipment, look [here][PMEM].

### Setting Number of NVMs
* dm-linear.h

        #define number_of_devices 2

* dm-linear.c 

        multi_pm[0]->host = "pmem0";
        multi_pm[1]->host = "pmem1";
        
### Setting Number of Cores
* bbuid.c
        
        #ifdef PERCORE	
 	        sih->global_log = kzalloc(sizeof(struct global_log), GFP_KERNEL);
 	        /* 56 = # of CPU cores */
 	        for(i=0; i<56; i++)
 		        sih->global_log->local_log[i] = 0;
        #endif
        
* inode.h
        
        struct global_log {
 	        /* 56 = # of CPU cores */
 	        struct local_log *local_log[56];
        };

## How To Use

### Build Kernel

    1. make menuconfig
        File systems --->
            [M] NOVA: log-structured file system for non-volatile memories
    2. make -j [Number of Cores]
    3. make modules_install
    4. make install
    
Reboot to the kernel installed

### Insert Modules and Mount File system
    1. cd [pNOVA kernel Directory]
    2. insmod fs/nova/nova.ko
    3. mount -t NOVA -O init,data_cow /dev/pmem0 [mount directory]
    
## Papers about pNOVA
1. pNOVA: Optimizing Shared File I/O Operations of NVM File System on Manycore Servers, J. H. Kim, J. W. Kim, H. Kang, C. Lee, S. Park, and Y. Kim, The 10th ACM SIGOPS Asia-Pacific Workshop on Systems (APSys), August 2019.
2. A NUMA-aware NVM File System Design for Manycore Server Applications, J. Kim, Y. Kim, S. Jamil, and S. Park, The 2020 IEEE International Symposium on Modeling, Analysis and Simulation of Computer and Telecommunication Systems (MASCOTS), November 2020.
3. Parallelizing Shared File I/O Operations of NVM File System for Manycore Servers. J. Kim, Y. Kim, S. Jamil, C. Lee,  and S. Park, IEEE Access, Vol.09, pp. 24570-24585, January 2021.

[NOVA]: https://github.com/NVSL/linux-nova
[PMEM]: https://pmem.io/2016/02/22/pm-emulation.html
