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

#include "aleph_fb_dev.h"
#include "aleph_fb_mclass.h"
#include "aleph_fbuc_dev.h"
#include "kern_funcs.h"
#include "utils.h"
#include "mclass_reg.h"

#include "hw/arm/guest-services/general.h"

static uint8_t aleph_fb_meta_class_inst[ALEPH_MCLASS_SIZE];

void *get_fb_mclass_inst(void)
{
    return (void *)&aleph_fb_meta_class_inst[0];
}

//virtual functions of our driver
void *AlephFramebufferDevice_getMetaClass(void *this)
{
    return get_fb_mclass_inst();
}

void create_new_aleph_fbdev(void *parent_service)
{
    //TODO: release this object ref?
    void *fbdev = OSMetaClass_allocClassWithName(FBDEV_CLASS_NAME);
    if (NULL == fbdev) {
        cancel();
    }

    if (!IOService_init(fbdev, NULL)) {
        IOLog("create_new_aleph_fbdev(): ::init() failed\n");
        cancel();
    }

    if (NULL == parent_service) {
        cancel();
    }

    if (!IORegistryEntry_setProperty(fbdev, "IOUserClientClass",
                                     FBUC_CLASS_NAME)) {
        IOLog("create_new_aleph_fbdev(): ::setProperty() failed\n");
        cancel();
    }

    if (!IOService_attach(fbdev, parent_service)) {
        IOLog("create_new_aleph_fbdev(): ::attach() failed\n");
        cancel();
    }

    IOService_registerService(fbdev, 0);
}
