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

//TODO: JONATHANA change the name of the directory and the README
//to something general and not bdev specific

#include <string.h>
#include <stdint.h>

#include "kern_funcs.h"
#include "utils.h"
#include "aleph_block_dev.h"
#include "aleph_bdev_mclass.h"
#include "aleph_fb_dev.h"
#include "aleph_fb_mclass.h"
#include "aleph_fbuc_dev.h"
#include "aleph_fbuc_mclass.h"

#include "hw/arm/guest-services/general.h"
#include "hw/arm/guest-services/xnu_general.h"

#define NUM_BLOCK_DEVS_MAX (10)

#ifndef NUM_BLOCK_DEVS
#define NUM_BLOCK_DEVS (2)
#endif

void _start() __attribute__((section(".start")));


//make sure to hook to this entry of the driver from a place that is called
//only once whe the system boots
void _start() {


    //get address of memory thaat we will use for framebuffer, other hooks,
    //etc.. from he host
    qemu_call_t *qcall = (qemu_call_t *)qemu_call_status();
    if (0 == qcall) {
        IOLog("_start: got 0 qcall\n");
        cancel();
    }
    qcall->call_number = QC_VALUE_CB;
    qcall->args.general.id = XNU_GET_MORE_ALLOCATED_DATA;
    qemu_call(qcall);

    uint64_t more_edata_vaddr = qcall->args.general.data1;
    uint64_t more_edata_size = qcall->args.general.data2;
    if ((0 == more_edata_vaddr) || (0 == more_edata_size)) {
        IOLog("_start: got 0 more_edata_vaddr or more_edata_size\n");
        cancel();
    }

    //Map the beginning of the physical memory to static virtual memory as
    //RWX so we can have the framebuffer, other hooks and more in there..
    arm_vm_page_granular_prot(more_edata_vaddr, more_edata_size,
                              0, 0, 0, 0, 1);

    if (NUM_BLOCK_DEVS > NUM_BLOCK_DEVS_MAX) {
        IOLog("_start: error: NUM_BLOCK_DEVS > NUM_BLOCK_DEVS_MAX\n");
        cancel();
    }

    register_bdev_meta_class();

    //TODO: release this object ref
    void *match_dict = IOService_serviceMatching("AppleARMPE", NULL);
    //TODO: release this object ref
    void *service = waitForMatchingService(match_dict, -1);
    if (0 == service) {
        IOLog("_start: can't find \"AppleARMPE\" service\n");
        cancel();
    }

    char bdev_prod_name[] = "0AlephBDev";
    char bdev_vendor_name[] = "0Aleph";
    char bdev_mutex_name[] = "0AM";

    for (uint64_t i = 0; i < NUM_BLOCK_DEVS; i++) {
        bdev_prod_name[0]++;
        bdev_vendor_name[0]++;
        bdev_mutex_name[0]++;
        //TODO: release this object ref?
        create_new_aleph_bdev(bdev_prod_name, bdev_vendor_name,
                              bdev_mutex_name, i, service);

        //TODO: hack for now to make the first registered bdev disk0 instead
        //of having the system change the order
        IOSleep(1000);
        ////wait for the first disk to be loaded as disk0
        //if (0 == i) {
        //    void *match_dict_first = IOService_serviceMatching("IOMediaBSDClient", NULL);
        //    void *service_first = waitForMatchingService(match_dict_first, -1);
        //}
    }

    register_fb_meta_class();
    register_fbuc_meta_class();

    //TODO: release this object ref
    //void *match_dict_disp = IOService_nameMatching("disp0", NULL);
    void *match_dict_disp = IOService_nameMatching("AppleARMPE", NULL);
    //TODO: release this object ref
    void *service_disp = waitForMatchingService(match_dict_disp, -1);
    if (0 == service_disp) {
        IOLog("_start: can't find \"AppleARMPE\" service\n");
        cancel();
    }

    create_new_aleph_fbdev(service_disp);
}
