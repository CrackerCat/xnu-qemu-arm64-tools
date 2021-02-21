/*
 * Copyright (c) 2020 Jonathan Afek <jonyafek@me.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef KERN_FUNCS_H
#define KERN_FUNCS_H

enum IODirection
{
    kIODirectionNone  = 0x0,//                    same as VM_PROT_NONE
    kIODirectionIn    = 0x1,// User land 'read',  same as VM_PROT_READ
    kIODirectionOut   = 0x2,// User land 'write', same as VM_PROT_WRITE
};

//IOBlockStorageDevice::IOBlockStorageDevice(IOBlockStorageDevice *this, const OSMetaClass *a2)
extern void *_ZN20IOBlockStorageDeviceC2EPK11OSMetaClass(void *this, void *mclass);
#define IOBlockStorageDevice_IOBlockStorageDevice(a, b) \
        _ZN20IOBlockStorageDeviceC2EPK11OSMetaClass(a, b)

//IOBlockStorageDevice - vtable
extern void *_ZTV20IOBlockStorageDevice;
#define IOBlockStorageDevice_vtable _ZTV20IOBlockStorageDevice

//IOBlockStorageDevice::MetaClass - vtable
extern void *_ZTVN20IOBlockStorageDevice9MetaClassE;
#define IOBlockStorageDevice_MetaClass_vtable \
        _ZTVN20IOBlockStorageDevice9MetaClassE

//IOBlockStorageDevice::gMetaClass
extern void *_ZN20IOBlockStorageDevice10gMetaClassE;
#define IOBlockStorageDevice_gMetaClass _ZN20IOBlockStorageDevice10gMetaClassE

//IOService - vtable
extern void *_ZTV9IOService;
#define IOService_vtable _ZTV9IOService

//IOService::MetaClass - vtable
extern void *_ZTVN9IOService9MetaClassE;
#define IOService_MetaClass_vtable _ZTVN9IOService9MetaClassE

//IOService::gMetaClass
extern void *_ZN9IOService10gMetaClassE;
#define IOService_gMetaClass _ZN9IOService10gMetaClassE

//IOUserClient - vtable
extern void *_ZTV12IOUserClient;
#define IOUserClient_vtable _ZTV12IOUserClient

//IOUserClient::MetaClass - vtable
extern void *_ZTVN12IOUserClient9MetaClassE;
#define IOUserClient_MetaClass_vtable _ZTVN12IOUserClient9MetaClassE

//IOUserClient::gMetaClass
extern void *_ZN12IOUserClient10gMetaClassE;
#define IOUserClient_gMetaClass _ZN12IOUserClient10gMetaClassE

extern void *_ZL15sAllClassesDict;
#define sAllClassesDict _ZL15sAllClassesDict

extern void *sStalledClassesLock;

extern void *sAllClassesLock;

//OSMetaClass::instanceConstructed() const
void _ZNK11OSMetaClass19instanceConstructedEv(void *meta_class_inst_ptr);
#define OSMetaClass_instanceConstructed(x) \
        _ZNK11OSMetaClass19instanceConstructedEv(x)

//OSMetaClass::allocClassWithName(char const*)
void *_ZN11OSMetaClass18allocClassWithNameEPKc(char *name);
#define OSMetaClass_allocClassWithName(x) \
        _ZN11OSMetaClass18allocClassWithNameEPKc(x)

//OSObject::operator new(unsigned long)
void *_ZN8OSObjectnwEm(uint64_t size);
#define OSObject_new(x) _ZN8OSObjectnwEm(x)

//OSMetaClass::OSMetaClass(char const*, OSMetaClass const*, unsigned int)
void *_ZN11OSMetaClassC2EPKcPKS_j(void *meta_class_inst_ptr, char *class_name,
                                  void *parent_meta_class_inst_ptr,
                                  uint64_t size);
#define OSMetaClass_OSMetaClass(w, x, y, z) \
        _ZN11OSMetaClassC2EPKcPKS_j(w, x, y, z)

//OSSymbol::withCStringNoCopy(char const*)
void *_ZN8OSSymbol17withCStringNoCopyEPKc(char *str);
#define OSSymbol_withCStringNoCopy(x) _ZN8OSSymbol17withCStringNoCopyEPKc(x)

//OSDictionary::setObject(OSSymbol const*, OSMetaClassBase const*)
void _ZN12OSDictionary9setObjectEPK8OSSymbolPK15OSMetaClassBase(void *dict,
                                                                void *sym,
                                                                void *obj);
#define OSDictionary_setObject(x, y, z) \
        _ZN12OSDictionary9setObjectEPK8OSSymbolPK15OSMetaClassBase(x, y, z)

//IOService::serviceMatching(char const*, OSDictionary*)
void *_ZN9IOService15serviceMatchingEPKcP12OSDictionary(char *class_name,
                                                        void *dict);
#define IOService_serviceMatching(x, y) \
        _ZN9IOService15serviceMatchingEPKcP12OSDictionary(x, y)

//IOService::nameMatching(char const*, OSDictionary*)
void *_ZN9IOService12nameMatchingEPKcP12OSDictionary(char *name,
                                                     void *dict);
#define IOService_nameMatching(x, y) \
        _ZN9IOService12nameMatchingEPKcP12OSDictionary(x, y)

//OSDictionary * waitForMatchingService(OSDictionary *param_1,long_long param_2)
void *_ZN9IOService22waitForMatchingServiceEP12OSDictionaryy(void *dict,
                                                             uint64_t timeout);
#define waitForMatchingService(x, y) \
        _ZN9IOService22waitForMatchingServiceEP12OSDictionaryy(x, y)

//IOService::IOService(OSMetaClass*)
void _ZN9IOServiceC1EPK11OSMetaClass(void *this, void *metaclass);
#define IOService_IOService(x, y) _ZN9IOServiceC1EPK11OSMetaClass(x, y)

//IOService::start(IOService*)
uint64_t _ZN9IOService5startEPS_(void *this);
#define IOService_start(x) _ZN9IOService5startEPS_(x)

//IOUserClient::IOUserClient(OSMetaClass const*
void _ZN12IOUserClientC2EPK11OSMetaClass(void *this, void *metaclass);
#define IOUserClient_IOUserClient(x, y) \
        _ZN12IOUserClientC2EPK11OSMetaClass(x, y)

//IOUserClient::externalMethod
uint64_t
_ZN12IOUserClient14externalMethodEjP25IOExternalMethodArgumentsP24IOExternalMethodDispatchP8OSObjectPv(
                            void *this, uint32_t selector, void *args,
                            void *dispatch, void *target, void *reference);
#define IOUserClient_externalMethod(a, b, c, d, e, f) \
        _ZN12IOUserClient14externalMethodEjP25IOExternalMethodArgumentsP24IOExternalMethodDispatchP8OSObjectPv(a, b, c, d, e, f)

//IOUserClient::~IOUserClient()
void *_ZN12IOUserClientD2Ev(void *this);
#define IOUserClient_destructor(x) _ZN12IOUserClientD2Ev(x)

//OSObject::operator delete(void*, unsigned long
void _ZN8OSObjectdlEPvm(void *obj, uint64_t size);
#define OSObject_delete(x, y) _ZN8OSObjectdlEPvm(x, y)

//IOUserClient::initWithTask(task*, void*, unsigned int)
uint64_t _ZN12IOUserClient12initWithTaskEP4taskPvj(void *this, void *task,
                                                   void *sid, uint64_t type);
#define IOUserClient_initWithTask(a, b, c, d) \
    _ZN12IOUserClient12initWithTaskEP4taskPvj(a, b, c, d)

//IOUserClient::setAsyncReference64(unsigned long long*, ipc_port*, unsigned long long, unsigned, long long, task*)
void _ZN12IOUserClient19setAsyncReference64EPyP8ipc_portyyP4task(void *aref,
                                                                 void *port,
                                                                 uint64_t cb,
                                                                 uint64_t ref,
                                                                 void *task);
#define IOUserClient_setAsyncReference64(a, b, c, d, e) \
    _ZN12IOUserClient19setAsyncReference64EPyP8ipc_portyyP4task(a, b, c, d, e)

//IOUserClient::sendAsyncResult64(unsigned long long*, int, unsigned long long*, unsigned int)
void _ZN12IOUserClient17sendAsyncResult64EPyiS0_j(void *aref, uint64_t res,
                                                  uint64_t *args, uint64_t cnt);
#define IOUserClient_sendAsyncResult64(a, b, c, d) \
    _ZN12IOUserClient17sendAsyncResult64EPyiS0_j(a, b, c, d)

//IOSurfaceRoot::lookupSurface(IOSurfaceRoot *__hidden this, unsigned int, task *)
void *_ZN13IOSurfaceRoot13lookupSurfaceEjP4task(void *surface_root,
                                                uint64_t layer_index,
                                                void *task);
#define IOSurfaceRoot_lookupSurface(a, b, c) \
    _ZN13IOSurfaceRoot13lookupSurfaceEjP4task(a, b, c)

//IOSurface::deviceLockSurface(IOSurface *__hidden this, unsigned int)
uint64_t _ZN9IOSurface17deviceLockSurfaceEj(void *surface, uint64_t i);
#define IOSurface_deviceLockSurface(a, b) \
    _ZN9IOSurface17deviceLockSurfaceEj(a, b)

//IOSurface::prepare(IOSurface *__hidden this)
uint64_t _ZN9IOSurface7prepareEv(void *surface);
#define IOSurface_prepare(a) \
    _ZN9IOSurface7prepareEv(a)

//IOSurface::getMemoryDescriptor(IOSurface *__hidden this)
void *_ZNK9IOSurface19getMemoryDescriptorEv(void *surface);
#define IOSurface_getMemoryDescriptor(a) \
    _ZNK9IOSurface19getMemoryDescriptorEv(a)

//IOSurface::deviceUnlockSurface(IOSurface *__hidden this, unsigned int)
void _ZN9IOSurface19deviceUnlockSurfaceEj(void *surface, uint64_t i);
#define IOSurface_deviceUnlockSurface(a, b) \
    _ZN9IOSurface19deviceUnlockSurfaceEj(a, b)

//IOSurface::complete(IOSurface *this)
void _ZN9IOSurface8completeEv(void *surface);
#define IOSurface_complete(a) \
    _ZN9IOSurface8completeEv(a)

//IOSurface::release(IOSurface *__hidden this)
void _ZNK9IOSurface7releaseEv(void *surface);
#define IOSurface_release(a) \
    _ZNK9IOSurface7releaseEv(a)

//OSObject::release(OSObject *__hidden this)
void _ZNK8OSObject7releaseEv(void *ob);
#define OSObject_release(a) \
    _ZNK8OSObject7releaseEv(a)

//IOMemoryMap::getVirtualAddress(IOMemoryMap *__hidden this)
void *_ZN11IOMemoryMap17getVirtualAddressEv(void *map);
#define IOMemoryMap_getVirtualAddress(a) \
    _ZN11IOMemoryMap17getVirtualAddressEv(a)

//IOMemoryMap::getPhysicalSegment(IOMemoryMap *__hidden this, IOByteCount offset, IOByteCount *length, IOOptionBits options)
uint64_t _ZN11IOMemoryMap18getPhysicalSegmentEyPyj(void *map, uint32_t offset,
                                                   uint32_t *len,
                                                   uint64_t opts);
#define IOMemoryMap_getPhysicalSegment(a, b, c, d) \
    _ZN11IOMemoryMap18getPhysicalSegmentEyPyj(a, b, c, d)

//IOMemoryDescriptor::getLength(IOMemoryDescriptor *__hidden this)
uint64_t _ZNK18IOMemoryDescriptor9getLengthEv(void *mem_desc);
#define IOMemoryDescriptor_getLength(a) \
    _ZNK18IOMemoryDescriptor9getLengthEv(a)

//IOMemoryDescriptor::map(IOMemoryDescriptor *__hidden this, IOOptionBits options)
void *_ZN18IOMemoryDescriptor3mapEj(void *mem_desc, uint64_t opts);
#define IOMemoryDescriptor_map(a, b) \
    _ZN18IOMemoryDescriptor3mapEj(a, b)

//IODirection __cdecl IOMemoryDescriptor::getDirection(IOMemoryDescriptor *__hidden this)
uint64_t _ZNK18IOMemoryDescriptor12getDirectionEv(void *mem_desc);
#define IOMemoryDescriptor_getDirection(a) \
    _ZNK18IOMemoryDescriptor12getDirectionEv(a)

//bool __cdecl IOBlockStorageDevice::init(IOBlockStorageDevice *__hidden this, OSDictionary *properties)
uint64_t _ZN20IOBlockStorageDevice4initEP12OSDictionary(void *block_dev,
                                                        void *dict);
#define IOBlockStorageDevice_init(a, b) \
    _ZN20IOBlockStorageDevice4initEP12OSDictionary(a, b)

//bool __cdecl IOService::attach(IOService *__hidden this, IOService *provider)
uint64_t _ZN9IOService6attachEPS_(void *service, void *provider);
#define IOService_attach(a, b) \
    _ZN9IOService6attachEPS_(a, b)

//void __cdecl IOService::registerService(IOService *__hidden this, IOOptionBits options)
uint64_t _ZN9IOService15registerServiceEj(void *service, uint64_t options);
#define IOService_registerService(a, b) \
    _ZN9IOService15registerServiceEj(a, b)

//bool __cdecl IOService::init(IOService *__hidden this, OSDictionary *dictionary)
uint64_t _ZN9IOService4initEP12OSDictionary(void *service, void *dict);
#define IOService_init(a, b) \
    _ZN9IOService4initEP12OSDictionary(a, b)

//bool __cdecl IOService::terminate(IOService *__hidden this, IOOptionBits options)
uint64_t _ZN9IOService9terminateEj(void *service, uint64_t options);
#define IOService_terminate(a, b) \
    _ZN9IOService9terminateEj(a, b)

//bool __cdecl IORegistryEntry::setProperty(IORegistryEntry *__hidden this, const char *aKey, const char *aString)
uint64_t _ZN15IORegistryEntry11setPropertyEPKcS1_(void *entry,
                                                  const char *aKey,
                                                  const char *aString);
#define IORegistryEntry_setProperty(a, b, c) \
    _ZN15IORegistryEntry11setPropertyEPKcS1_(a, b, c)

void arm_vm_page_granular_prot(uint64_t start, uint64_t size,
                               uint64_t pa_offset, uint64_t tte_prot_XN,
                               uint64_t pte_prot_APX, uint64_t pte_prot_XN,
                               uint64_t force_page_granule);

//void _IOLog(undefined8 param_1)
void IOLog(const char* fmt, ...);

//void _IOSleep(undefined8 param_1)
void IOSleep(uint64_t millisecs);

void lck_mtx_lock(void *lck_mtx);
void lck_mtx_unlock(void *lck_mtx);
void *lck_grp_alloc_init(char *name, void *p2);
void *lck_mtx_alloc_init(void *mtx_grp, void *p2);

void kernel_thread_start(void *code, void *param, void *new_thread);

#endif
