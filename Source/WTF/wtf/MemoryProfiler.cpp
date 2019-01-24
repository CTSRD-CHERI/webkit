#include "MemoryProfiler.h"

#include <iostream>
#include <fstream>

#include <malloc_np.h>
#include <unistd.h>
#include <inttypes.h>

#if __has_include(<libmw.h>)
#define HAVE_LIBMEMWALK 1
extern "C" {
#include <libmw.h>
};
#ifdef __FreeBSD__
#include <sys/user.h>
#endif
#endif

// static initialisers
std::thread MemoryProfiler::outputThread;
std::atomic_bool MemoryProfiler::shouldRun(false);
std::atomic_ulong MemoryProfiler::jsStackBytes(0);
std::atomic_ulong MemoryProfiler::mmapBytes(0);

#if HAVE_LIBMEMWALK
static void
perm_string(char *buf, uint64_t perms)
{
    buf[0] = buf[1] = buf[2]  = '-';
    buf[3] = '\0';
    if ((perms & MW_PERM_READ) == MW_PERM_READ)
        buf[0] = 'r';
    if ((perms & MW_PERM_WRITE) == MW_PERM_WRITE)
        buf[1] = 'w';
    if ((perms & MW_PERM_EXECUTE) == MW_PERM_EXECUTE)
        buf[2] = 'x';
}

static void
type_string(char *buf, uint64_t type)
{
    buf[0] = buf[1] = '-';
    buf[2] = '\0';
#ifdef __FreeBSD__
    if (type == KVME_TYPE_DEAD) {
        buf[0] = 'd';
        buf[1] = 'd';
    } else if (type == KVME_TYPE_DEFAULT) {
        buf[0] = 'd';
        buf[1] = 'f';
    } else if (type == KVME_TYPE_DEVICE) {
        buf[0] = 'd';
        buf[1] = 'v';
    } else if (type == KVME_TYPE_MGTDEVICE) {
        buf[0] = 'm';
        buf[1] = 'd';
    } else if (type == KVME_TYPE_PHYS) {
        buf[0] = 'p';
        buf[1] = 'h';
    } else if (type == KVME_TYPE_SG) {
        buf[0] = 's';
        buf[1] = 'g';
    } else if (type == KVME_TYPE_SWAP) {
        buf[0] = 's';
        buf[1] = 'w';
    } else if (type == KVME_TYPE_VNODE) {
        buf[0] = 'v';
        buf[1] = 'n';
    }
#endif
}
#endif

static uint64_t getProcStatMemUsage(std::ofstream& regionsFile) {
    uint64_t memUsage = 0;
#if HAVE_LIBMEMWALK
    struct mw_context *ctx = mw_alloc_context(getpid());
    struct mw_region region;
    char perm[4], type[3];
    while (mw_next_range(ctx, &region)) {
        perm_string(perm, region.perms);
        type_string(type, region.type);
        regionsFile << "\t\tregion with address " << std::hex << (void*)region.addr << std::dec << " has size " << region.size << " with perms " << perm << " and type " << type << "\n";
        if ((region.perms & MW_PERM_WRITE) != 0 && (region.perms & MW_PERM_EXECUTE) == 0
#ifdef __FreeBSD__
            && (region.type == KVME_TYPE_DEFAULT || region.type == KVME_TYPE_SWAP || region.type == KVME_TYPE_PHYS)
#endif
            )
            memUsage += region.size;
    }
    regionsFile << "Total mem usage of interest: " << memUsage << "\n\n";
    mw_free_context(ctx);
#endif
    return memUsage;
}

void MemoryProfiler::outputStats(void) {
    // variables to hold stats
    size_t allocated, active, metadata, resident, mapped, statsSize, epochSize;
    epochSize = sizeof(uint64_t);
    statsSize = sizeof(size_t);

    // output files
    std::ofstream statsFile, regionsFile;
    std::string statsFileName, regionsFileName;
    if (const char *env_p = getenv("DRT_MEM_PROF_STATS_OUTPUT"))
        statsFileName = env_p;
    else
        statsFileName = "stats.dat";
    if (const char *env_p = getenv("DRT_MEM_PROF_REGIONS_OUTPUT"))
        regionsFileName = env_p;
    else
        regionsFileName = "regions.dat";

    statsFile.open(statsFileName);
    regionsFile.open(regionsFileName);

    uint64_t epoch = 1;
    uint64_t timeElapsed = 0;
    uint32_t sleepInterval = 1; // in seconds

    statsFile << "time,app-mmaps,js-stack,allocated,active,metadata,resident,mapped,procstat" << "\n";

    while (shouldRun) {
        sleep(sleepInterval);
        timeElapsed += sleepInterval;

        // Update the statistics cached by mallctl.
        mallctl("epoch", &epoch, &epochSize, &epoch, epochSize);

        if (mallctl("stats.allocated", &allocated, &statsSize, NULL, 0) == 0
            && mallctl("stats.active", &active, &statsSize, NULL, 0) == 0
            && mallctl("stats.metadata", &metadata, &statsSize, NULL, 0) == 0
            && mallctl("stats.resident", &resident, &statsSize, NULL, 0) == 0
            && mallctl("stats.mapped", &mapped, &statsSize, NULL, 0) == 0) {
            regionsFile << "After " << timeElapsed << ":\n\n";
            regionsFile << "time,app-mmaps,js-stack,allocated,active,metadata,resident,mapped" << "\n";
            regionsFile << timeElapsed
                      << "," << mmapBytes
                      << "," << jsStackBytes
                      << "," << allocated
                      << "," << active
                      << "," << metadata
                      << "," << resident
                      << "," << mapped << "\n\n";
            uint64_t procstatMemUsage = getProcStatMemUsage(regionsFile);
            statsFile << timeElapsed
                      << "," << mmapBytes
                      << "," << jsStackBytes
                      << "," << allocated
                      << "," << active
                      << "," << metadata
                      << "," << resident
                      << "," << mapped
                      << "," << procstatMemUsage << "\n";
            //fprintf(stderr,
            //        "Current allocated/active/metadata/resident/mapped/app-mmap/mw: %zu/%zu/%zu/%zu/%zu/%lu/%lu\n",
            //        allocated, active, metadata, resident, mapped, mmapBytes.load(), totalMemUsage);
        }
    }

    statsFile.close();
    regionsFile.close();
}


extern "C" {
void MemoryProfiler_record_js_stack_grow(size_t n) {
    MemoryProfiler::recordJsStackGrow(n);
}
void MemoryProfiler_record_js_stack_shrink(size_t n) {
    MemoryProfiler::recordJsStackShrink(n);
}
void MemoryProfiler_record_mmap(size_t n) {
    MemoryProfiler::recordMmap(n);
}
void MemoryProfiler_record_munmap(size_t n) {
    MemoryProfiler::recordMunmap(n);
}
}