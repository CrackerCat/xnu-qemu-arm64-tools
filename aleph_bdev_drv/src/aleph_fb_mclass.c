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

#include <string.h>
#include <stdint.h>

#include "aleph_fb_mclass.h"
#include "kern_funcs.h"
#include "aleph_fb_dev.h"
#include "utils.h"
#include "mclass_reg.h"

#include "hw/arm/guest-services/general.h"

static void *fb_vtable[FB_VTABLE_SIZE];
static MetaClassVTable fb_meta_class_vtable;

void create_fb_vtable(void)
{
    memcpy(&fb_vtable[0],
           (char *)&IOService_vtable + 0x10,
           sizeof(fb_vtable));
    fb_vtable[IOSERVICE_GETMCLASS_INDEX] =
            &AlephFramebufferDevice_getMetaClass;
}

void *fb_alloc(void)
{
    void **obj = OSObject_new(ALEPH_FBDEV_SIZE);
    IOService_IOService(obj, get_fb_mclass_inst());
    obj[0] = &fb_vtable[0];
    OSMetaClass_instanceConstructed(get_fb_mclass_inst());
    return obj;
}

void create_fb_metaclass_vtable(void)
{
    memcpy(&fb_meta_class_vtable, (char *)&IOService_MetaClass_vtable + 0x10,
           sizeof(MetaClassVTable));
    fb_meta_class_vtable.alloc = &fb_alloc;
}

void register_fb_meta_class()
{
    mclass_reg_slock_lock();

    create_fb_vtable();
    create_fb_metaclass_vtable();

    void **mc = OSMetaClass_OSMetaClass(get_fb_mclass_inst(),
                                        FBDEV_CLASS_NAME,
                                        (void *)&IOService_gMetaClass,
                                        ALEPH_FBDEV_SIZE);
    if (NULL == mc) {
        IOLog("register_fb_meta_class(): got NULL mc\n");
        cancel();
    }
    mc[0] = &fb_meta_class_vtable;
    mclass_reg_alock_lock();
    add_to_classes_dict(FBDEV_CLASS_NAME, mc);
    mclass_reg_alock_unlock();
    mclass_reg_slock_unlock();
}
