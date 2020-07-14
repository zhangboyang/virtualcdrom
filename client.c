// clang -o client client.c -framework IOKit

#include <stdio.h>
#include <string.h>
#include <IOKit/IOKitLib.h>

// reference: virtualcdrom.hpp
#define CDTOC_SZ 4096
void *cdtoc;
#define MAX_CDDATA 128
#define CDDATA_SZ (2352 * 4096)
void *cddata[MAX_CDDATA];


void *doMap(io_connect_t connect, int id, int expected_sz)
{
    kern_return_t kernResult;
    mach_vm_address_t addr;
    mach_vm_size_t sz;
    void *ret;
    
    kernResult = IOConnectMapMemory(connect, id, mach_task_self(), &addr, &sz, kIOMapAnywhere);
    if (kernResult != KERN_SUCCESS) {
        fprintf(stderr, "IOConnectMapMemory returned 0x%08x\n", kernResult);
        return NULL;
    }
    
    ret = (void *) addr;
    
    if (sz != expected_sz) {
        fprintf(stderr, "mapped %d at %p sz 0x%llx != 0x%x\n", id, ret, (unsigned long long) sz, expected_sz);
        return NULL;
    }
    
    memset(ret, 0, sz);
    return ret;
}

void doLoader(io_connect_t connect)
{
    cdtoc = doMap(connect, MAX_CDDATA, CDTOC_SZ);
    for (int i = 0; i < MAX_CDDATA; i++) {
        cddata[i] = doMap(connect, i, CDDATA_SZ);
    }
    
    int n = 0;
    int r;
    
    r = fread(cdtoc, 1, CDTOC_SZ, stdin);
    if (r != CDTOC_SZ) goto done;
    for (int i = 0; i < MAX_CDDATA; i++) {
        r = fread(cddata[i], 1, CDDATA_SZ, stdin);
        n += r;
        if (r != CDDATA_SZ) goto done;
    }

done:
    fprintf(stderr, "  successfully loaded %d bytes to kernel\n", n);
}

void doService(io_service_t service)
{
    kern_return_t kernResult;
    io_connect_t connect;
    
    kernResult = IOServiceOpen(service, mach_task_self(), 0, &connect);
    if (kernResult != KERN_SUCCESS) {
        fprintf(stderr, "IOServiceOpen returned 0x%08x\n", kernResult);
        return;
    }
    
    doLoader(connect);
    
    kernResult = IOServiceClose(connect);
    if (kernResult != KERN_SUCCESS) {
        fprintf(stderr, "IOServiceClose returned 0x%08x\n\n", kernResult);
    }
}

int main()
{
    kern_return_t kernResult;
    io_service_t service;
    io_iterator_t iterator;
    
    kernResult = IOServiceGetMatchingServices(kIOMasterPortDefault, IOServiceMatching("VirtualCDROM"), &iterator);
    if (kernResult != KERN_SUCCESS) {
        fprintf(stderr, "IOServiceGetMatchingServices returned 0x%08x\n\n", kernResult);
        return -1;
    }

    service = IOIteratorNext(iterator);
    if (service != IO_OBJECT_NULL) {
        doService(service);
    } else {
        fprintf(stderr, "  No matching drivers found.\n");
        fprintf(stderr, "  Please run \"kextload /path/to/virtualcdrom.kext\" first.\n");
    }
    
    IOObjectRelease(iterator);
    
    return 0;
}
