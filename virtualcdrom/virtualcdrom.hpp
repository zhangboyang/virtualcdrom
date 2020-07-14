#include <mach/vm_param.h>
#include <IOKit/IOLib.h>
#include <IOKit/IOLocks.h>
#include <IOKit/storage/IOCDTypes.h>
#include <IOKit/storage/IOCDBlockStorageDevice.h>
#include <IOKit/storage/IOCDBlockStorageDriver.h>
#include <IOKit/IOUserClient.h>

class VirtualCDROM : public IOCDBlockStorageDevice {
    OSDeclareDefaultStructors(VirtualCDROM)
public:
    virtual bool init(OSDictionary * properties) override;
    virtual void free() override;
    virtual bool start(IOService * provider) override;
    
    virtual IOReturn doEjectMedia(void) override;
    virtual IOReturn doFormatMedia(UInt64 byteCapacity) override;
    virtual UInt32 doGetFormatCapacities(UInt64 * capacities,
                                         UInt32 capacitiesMaxCount) const override;
    virtual char * getVendorString(void) override;
    virtual char * getProductString(void) override;
    virtual char * getRevisionString(void) override;
    virtual char * getAdditionalDeviceInfoString(void) override;
    virtual IOReturn reportBlockSize(UInt64 *blockSize) override;
    virtual IOReturn reportEjectability(bool *isEjectable) override;
    virtual IOReturn reportMaxValidBlock(UInt64 *maxBlock) override;
    virtual IOReturn reportMediaState(bool *mediaPresent,bool *changedState) override;
    virtual IOReturn reportRemovability(bool *isRemovable) override;
    virtual IOReturn reportWriteProtection(bool *isWriteProtected) override;
    virtual IOReturn doAsyncReadWrite(IOMemoryDescriptor *buffer,
                                      UInt64 block, UInt64 nblks,
                                      IOStorageAttributes *attributes,
                                      IOStorageCompletion *completion) override;
    
    virtual IOReturn doAsyncReadCD(IOMemoryDescriptor *buffer,
                                   UInt32 block,UInt32 nblks,
                                   CDSectorArea sectorArea,
                                   CDSectorType sectorType,
                                   IOStorageCompletion completion) override;
    virtual UInt32 getMediaType(void) override;
    virtual IOReturn readISRC(UInt8 track,CDISRC isrc) override;
    virtual IOReturn readMCN(CDMCN mcn) override;
    virtual IOReturn readTOC(IOMemoryDescriptor *buffer) override;
    
    virtual IOReturn getSpeed(UInt16 * kilobytesPerSecond) override;
    
    virtual IOReturn setSpeed(UInt16 kilobytesPerSecond) override;
    
    virtual IOReturn readTOC(IOMemoryDescriptor *buffer,CDTOCFormat format,
                             UInt8 msf,UInt8 trackSessionNumber,
                             UInt16 *actualByteCount) override;
    
    virtual IOReturn readDiscInfo(IOMemoryDescriptor *buffer,
                                  UInt16 *actualByteCount) override;
    
    virtual IOReturn readTrackInfo(IOMemoryDescriptor *buffer,UInt32 address,
                                   CDTrackInfoAddressType addressType,
                                   UInt16 *actualByteCount) override;

public:
    IOLock *lock;
    
    // FIXME: race
    
    bool hasCD;
    
#define CDTOC_SZ 4096
    void *cdtoc;
    
#define MAX_CDDATA 128  // 128*2352*4096=1.23GB should enough
#define CDDATA_SZ (2352 * 4096)
    void *cddata[MAX_CDDATA]; // each memory is CDDATA_SZ bytes long

    void unloadCD();
    void loadCD();
};

class VirtualCDROMUserClient : public IOUserClient {
    OSDeclareDefaultStructors(VirtualCDROMUserClient);
public:
    virtual bool start(IOService* provider) override;
    virtual IOReturn clientMemoryForType(UInt32 type, UInt32 *flags, IOMemoryDescriptor **memory) override;
    virtual IOReturn clientClose(void) override;
    
public:
    VirtualCDROM *fProvider;
};
