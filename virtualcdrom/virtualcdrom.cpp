#include "virtualcdrom.hpp"

#define DLOG IOLog

OSDefineMetaClassAndStructors(VirtualCDROM, IOCDBlockStorageDevice)
#define super IOCDBlockStorageDevice

bool VirtualCDROM::init(OSDictionary *properties)
{
    memset(&cddata, 0, sizeof(cddata));
    cdtoc = nullptr;
    lock = nullptr;
    hasCD = false;

    DLOG("VirtualCDROM::init pagesz %lld build %s %s\n", (unsigned long long) page_size, __DATE__, __TIME__ );
    
    // alloc lock
    lock = IOLockAlloc();
    if (!lock) return false;
    
    // alloc memory
    cdtoc = IOMallocPageable(CDTOC_SZ, page_size);
    bool memok = cdtoc != nullptr;
    for (int i = 0; i < MAX_CDDATA && memok; i++) {
        cddata[i] = IOMallocPageable(CDDATA_SZ, page_size);
        memok = memok && cddata[i] != nullptr;
    }
    if (!memok) return false;
    
    memset(cdtoc, 0, CDTOC_SZ);
    for (int i = 0; i < MAX_CDDATA; i++) {
        memset(cddata[i], 0, CDDATA_SZ);
    }
    
    return super::init(properties);
}
void VirtualCDROM::free()
{
    if (cdtoc) { IOFreePageable(cdtoc, CDTOC_SZ); cdtoc = nullptr; }
    for (int i = 0; i < MAX_CDDATA; i++) {
        if (cddata[i]) { IOFreePageable(cddata[i], CDDATA_SZ); cddata[i] = nullptr; }
    }
    if (lock) { IOLockFree(lock); lock = nullptr; }
    super::free();
}
bool VirtualCDROM::start(IOService * provider)
{
    if (!super::start(provider)) return false;
    registerService();
    return true;
}

IOReturn VirtualCDROM::doEjectMedia(void)
{
    hasCD = false;
    return kIOReturnSuccess;
}
IOReturn VirtualCDROM::doFormatMedia(UInt64 byteCapacity)
{
    return kIOReturnUnsupported;
}
UInt32 VirtualCDROM::doGetFormatCapacities(UInt64 * capacities, UInt32 capacitiesMaxCount) const
{
    return 0;
}
char * VirtualCDROM::getVendorString(void)
{
    static char s[] = "VirtualCDROM";
    return s;
}
char * VirtualCDROM::getProductString(void)
{
    static char s[] = "AudioCD";
    return s;
}
char * VirtualCDROM::getRevisionString(void)
{
    static char s[] = __DATE__ " " __TIME__;
    return s;
}
char * VirtualCDROM::getAdditionalDeviceInfoString(void)
{
    static char s[] = "No Additional Device Info";
    return s;
}

IOReturn VirtualCDROM::reportBlockSize(UInt64 *blockSize)
{
    *blockSize = 2352;
    return kIOReturnSuccess;
}
IOReturn VirtualCDROM::reportEjectability(bool *isEjectable)
{
    *isEjectable = true;
    return kIOReturnSuccess;
}
IOReturn VirtualCDROM::reportMaxValidBlock(UInt64 *maxBlock)
{
    *maxBlock = CDDATA_SZ / 2352 * MAX_CDDATA - 1;
    return kIOReturnSuccess;
}
IOReturn VirtualCDROM::reportMediaState(bool *mediaPresent, bool *changedState)
{
    *mediaPresent = hasCD;
    *changedState = false;
    return kIOReturnSuccess;
}
IOReturn VirtualCDROM::reportRemovability(bool *isRemovable)
{
    *isRemovable = true;
    return kIOReturnSuccess;
}
IOReturn VirtualCDROM::reportWriteProtection(bool *isWriteProtected)
{
    *isWriteProtected = true;
    return kIOReturnSuccess;
}
IOReturn VirtualCDROM::doAsyncReadWrite(IOMemoryDescriptor *buffer,
                                        UInt64 block, UInt64 nblks,
                                        IOStorageAttributes *attributes,
                                        IOStorageCompletion *completion)
{
    return kIOReturnUnsupported;
}


IOReturn  VirtualCDROM::doAsyncReadCD(IOMemoryDescriptor *buffer,
                                                   UInt32 block,UInt32 nblks,
                                                   CDSectorArea sectorArea,
                                                   CDSectorType sectorType,
                                                   IOStorageCompletion completion)
{
    // IOStorage::complete ( returnData, status, actualByteCount );
    DLOG("VirtualCDROM::doAsyncReadCD 0x%llx,%d,%d %x,%x\n", buffer->getLength(), block, nblks, sectorArea, sectorType);
    IOReturn status = kIOReturnUnsupported;
    if ((sectorArea == kCDSectorAreaUser && sectorType == kCDSectorTypeCDDA) || sectorType == kCDSectorTypeUnknown) {
        IOByteCount offset = 0;
        while (nblks > 0) {
            char buf[2352];
            UInt32 x = block / (CDDATA_SZ / 2352);
            UInt32 y = block % (CDDATA_SZ / 2352);
            if (x < MAX_CDDATA) {
                memcpy(buf, (char *) cddata[x] + y * 2352, 2352);
            } else {
                memset(buf, 0, sizeof(buf));
            }
            if (offset + 2352 > buffer->getLength()) {
                break;
            }
            buffer->writeBytes(offset, buf, sizeof(buf));
            block++;
            nblks--;
            offset += 2352;
        }
        IOStorage::complete(&completion, kIOReturnSuccess, offset);
        status = kIOReturnSuccess;
    }
    return status;
}
UInt32 VirtualCDROM::getMediaType(void)
{
    DLOG("VirtualCDROM::getMediaType\n");
    return kCDMediaTypeROM;
}
IOReturn  VirtualCDROM::readISRC(UInt8 track,CDISRC isrc)
{
    DLOG("VirtualCDROM::readISRC\n");
    return kIOReturnNotFound;
}
IOReturn  VirtualCDROM::readMCN(CDMCN mcn)
{
    DLOG("VirtualCDROM::readMCN\n");
    return kIOReturnNotFound;
}
IOReturn  VirtualCDROM::readTOC(IOMemoryDescriptor *buffer)
{
    return readTOC(buffer, kCDTOCFormatTOC, 0x01, 0x00, NULL);
}

IOReturn  VirtualCDROM::getSpeed(UInt16 * kilobytesPerSecond)
{
    DLOG("VirtualCDROM::getSpeed\n");
    return kIOReturnUnsupported;
}
IOReturn  VirtualCDROM::setSpeed(UInt16 kilobytesPerSecond)
{
    DLOG("VirtualCDROM::setSpeed\n");
    return kIOReturnUnsupported;
}

IOReturn  VirtualCDROM::readTOC(IOMemoryDescriptor *buffer,CDTOCFormat format,
                                             UInt8 msf,UInt8 trackSessionNumber,
                                             UInt16 *actualByteCount)
{
    DLOG("VirtualCDROM::readTOC 0x%llx %d,%d,%d\n", buffer->getLength(), format, msf, trackSessionNumber);
    
    IOReturn status = kIOReturnUnsupported;
    if (format == kCDTOCFormatTOC && msf == 1) {
        auto n = buffer->getLength();
        if (n > CDTOC_SZ) n = CDTOC_SZ;
        
        buffer->writeBytes(0, cdtoc, n);
        if (actualByteCount) *actualByteCount = n;
        
        status = kIOReturnSuccess;
    }
    return status;
}

IOReturn  VirtualCDROM::readDiscInfo(IOMemoryDescriptor *buffer,
                                                  UInt16 *actualByteCount)
{
    DLOG("VirtualCDROM::readDiscInfo\n");
    return kIOReturnUnsupported;
}

IOReturn  VirtualCDROM::readTrackInfo(IOMemoryDescriptor *buffer,UInt32 address,
                                                   CDTrackInfoAddressType addressType,
                                                   UInt16 *actualByteCount)
{
    DLOG("VirtualCDROM::readTrackInfo\n");
    return kIOReturnUnsupported;
}

void VirtualCDROM::unloadCD()
{
    IOLockLock(lock);
    hasCD = false;
    messageClients(kIOMessageMediaStateHasChanged, (void *) kIOMediaStateOffline);
    IOLockUnlock(lock);
}
void VirtualCDROM::loadCD()
{
    CDTOC dummy;
    memset(&dummy, 0, sizeof(dummy));
    bool loadedCD = memcmp(cdtoc, &dummy, sizeof(dummy)) != 0;
    
    IOLockLock(lock);
    if (loadedCD) {
        hasCD = true;
        messageClients(kIOMessageMediaStateHasChanged, (void *) kIOMediaStateOnline);
    } else {
        hasCD = false;
        messageClients(kIOMessageMediaStateHasChanged, (void *) kIOMediaStateOffline);
    }
    IOLockUnlock(lock);
}

#undef super

// ==================================================================================================

OSDefineMetaClassAndStructors(VirtualCDROMUserClient, IOUserClient)
#define super IOUserClient

bool VirtualCDROMUserClient::start(IOService* provider)
{
    fProvider = OSDynamicCast(VirtualCDROM, provider);
    if (fProvider == nullptr) return false;
    fProvider->unloadCD();
    return super::start(provider);
}
IOReturn VirtualCDROMUserClient::clientMemoryForType(UInt32 type, UInt32 *flags, IOMemoryDescriptor **memory)
{
    void *ptr = nullptr;
    IOByteCount sz = 0;
    if (type < MAX_CDDATA) {
        ptr = fProvider->cddata[type];
        sz = CDDATA_SZ;
    } else if (type == MAX_CDDATA) {
        ptr = fProvider->cdtoc;
        sz = CDTOC_SZ;
    }
    if (ptr) {
        *memory = IOMemoryDescriptor::withAddress(ptr, sz, kIODirectionNone);
    } else {
        *memory = nullptr;
    }
    return *memory ? kIOReturnSuccess : kIOReturnUnsupported;
}
IOReturn VirtualCDROMUserClient::clientClose(void)
{
    fProvider->loadCD();
    terminate();
    return kIOReturnSuccess;
}
