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

#include <stdint.h>
#include <string.h>

#include "aleph_block_dev.h"
#include "aleph_bdev_mclass.h"
#include "kern_funcs.h"
#include "utils.h"
#include "mclass_reg.h"

#include "hw/arm/guest-services/general.h"

static uint8_t aleph_bdev_meta_class_inst[ALEPH_MCLASS_SIZE];

AlephBDevMembers *get_bdev_members(void *bdev)
{
    AlephBDevMembers *members;
    members = (AlephBDevMembers *)&((uint8_t *)bdev)[ALEPH_BDEV_MEMBERS_OFFSET];
    return members;
}

void *get_bdev_mclass_inst(void)
{
    return (void *)&aleph_bdev_meta_class_inst[0];
}

//virtual functions of our driver

void *AlephBlockDevice_getMetaClass(void *this)
{
    return get_bdev_mclass_inst();
}

uint64_t AlephBlockDevice_reportRemovability(void *this, char *isRemovable)
{
    *isRemovable = 0;
    return 0;
}

uint64_t AlephBlockDevice_reportMediaState(void *this,
                                           char *mediaPresent,
                                           char *changedState)
{
    *mediaPresent = 1;
    *changedState = 0;
    return 0;
}

uint64_t AlephBlockDevice_reportBlockSize(void *this, uint64_t *param)
{
    *param = BLOCK_SIZE;
    return 0;
}

uint64_t AlephBlockDevice_reportMaxValidBlock(void *this, uint64_t *param)
{
    AlephBDevMembers *members = get_bdev_members(this);
    *param = members->block_count - 1;
    return 0;
}

//could be reportLockability or reportWriteProtection?
uint64_t AlephBlockDevice_somefunc3(void *this, char *param)
{
    *param = 0;
    return 0;
}

char *AlephBlockDevice_getVendorString(void *this)
{
    AlephBDevMembers *members = get_bdev_members(this);
    return &members->vendor_name[0];
}

char *AlephBlockDevice_getProductString(void *this)
{
    AlephBDevMembers *members = get_bdev_members(this);
    return &members->product_name[0];
}

uint64_t AlephBlockDevice_doAsyncReadWrite(void *this, void **buffer,
                                         uint64_t block, uint64_t nblks,
                                         void *attrs, void **completion)
{
    uint64_t i = 0;
    AlephBDevMembers *members = get_bdev_members(this);

    //TODO: JONATHANA this lock currently protects the global qcall obj
    //consider having one per cpuu if we want to support performance with
    //multiple CPUs
    lck_mtx_lock(members->lck_mtx);

    void **buffer_vtable = buffer[0];

    uint64_t direction = IOMemoryDescriptor_getDirection(buffer);

    uint64_t byte_count = 0;
    uint64_t offset = 0;
    uint64_t length = 0;

    void *map = IOMemoryDescriptor_map(buffer, 0);
    for (i = 0; i < nblks; i++) {
        offset = BLOCK_SIZE * (i + block);
        length = BLOCK_SIZE;
        if ((offset + length) > members->size) {
            length = members->size - offset;
        }
        if ((offset + length) >= members->size + BLOCK_SIZE) {
            IOLog("AlephBlockDevice_doAsyncReadWrite: read/write over size\n");
            cancel();
        }

        uint32_t cur_offset = i * BLOCK_SIZE;
        uint32_t seg_len = 0;
        uint32_t len_done = 0;
        uint64_t paddr = 0;
        while (len_done < length) {
            paddr = IOMemoryMap_getPhysicalSegment(map, cur_offset,
                                                   &seg_len, 0);
            if ((0 == paddr) || (0 == seg_len)) {
                IOLog("AlephBlockDevice_doAsyncReadWrite: paddr or len 0\n");
                cancel();
            }
            if (seg_len > length) {
                seg_len = length;
            }
            if (kIODirectionIn == direction) {
                qc_read_file(paddr, seg_len, offset, members->qc_file_index,
                             (void *)members->qcall_vaddr);
            } else if (kIODirectionOut == direction) {
                qc_write_file(paddr, seg_len, offset, members->qc_file_index,
                              (void *)members->qcall_vaddr);
            } else {
                IOLog("AlephBlockDevice_doAsyncReadWrite: unknown dir\n");
                cancel();
            }
            len_done += seg_len;
            cur_offset += seg_len;
            offset += seg_len;
        }
        byte_count += length;
    }
    OSObject_release(map);

    //TODO: JONATHANA this is very ineffective. We have to change he mechanism
    //so that on the host side things will be implemented with a mmap()
    //instead of read()/write(). More important though is that in case
    //mmap access page faults on the host, we have to let the guest conext
    //switch and keep running. We have to perform the action on the host from
    //here on a new host thread or async in another way and let the guest keep
    //running. Once the host is done it will deliver an IRQ to the guest
    //and we will execute the completion routine from this driver.
    if (NULL != completion) {
        FuncCompletionAction comp_act_f = (FuncCompletionAction)completion[1];
        comp_act_f((uint64_t)completion[0], (uint64_t)completion[2], 0,
                   byte_count);
    }

    lck_mtx_unlock(members->lck_mtx);
    return 0;
}

void create_new_aleph_bdev(const char *prod_name, const char *vendor_name,
                           const char *mutex_name, uint64_t bdev_file_index,
                           void *parent_service)
{
    //TODO: release this object ref?
    void *bdev = OSMetaClass_allocClassWithName(BDEV_CLASS_NAME);
    if (NULL == bdev) {
        IOLog("create_new_aleph_bdev(): NULL bdev\n");
        cancel();
    }

    if (!IOBlockStorageDevice_init(bdev, NULL)) {
        IOLog("create_new_aleph_bdev(): ::init() failed\n");
        cancel();
    }

    AlephBDevMembers *members = get_bdev_members(bdev);
    members->qc_file_index = bdev_file_index;
    strncpy(&members->product_name[0], prod_name, VENDOR_NAME_SIZE);
    strncpy(&members->vendor_name[0], vendor_name, VENDOR_NAME_SIZE);
    members->vendor_name[0] += bdev_file_index;
    strncpy(&members->mutex_name[0], mutex_name, VENDOR_NAME_SIZE);
    members->mutex_name[0] += bdev_file_index;
    members->mtx_grp = lck_grp_alloc_init(&members->mutex_name[0], NULL);
    members->lck_mtx = lck_mtx_alloc_init(members->mtx_grp, NULL);
    members->qcall_vaddr = qemu_call_status();
    if (0 == members->qcall_vaddr) {
        IOLog("create_new_aleph_bdev(): members->qcall_vaddr is 0\n");
        cancel();
    }
    members->size = qc_size_file(bdev_file_index,
                                 (void *)members->qcall_vaddr);
    if (0 == members->size) {
        IOLog("create_new_aleph_bdev(): members->size is 0\n");
        cancel();
    }

    members->block_count = (members->size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    if (0 == members->block_count) {
        IOLog("create_new_aleph_bdev(): members->block_count is 0\n");
        cancel();
    }

    if (NULL == parent_service) {
        IOLog("create_new_aleph_bdev(): NULL parent_service\n");
        cancel();
    }

    if (!IOService_attach(bdev, parent_service)) {
        IOLog("create_new_aleph_bdev(): ::attach() failed\n");
        cancel();
    }

    IOService_registerService(bdev, 0);
}
