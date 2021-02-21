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

#include "aleph_fbuc_dev.h"
#include "aleph_fbuc_mclass.h"
#include "kern_funcs.h"
#include "utils.h"
#include "mclass_reg.h"

#include "hw/arm/guest-services/general.h"
#include "hw/arm/guest-services/xnu_general.h"

static uint8_t aleph_fbuc_meta_class_inst[ALEPH_MCLASS_SIZE];

void *get_fbuc_mclass_inst(void)
{
    return (void *)&aleph_fbuc_meta_class_inst[0];
}

static FBUCMembers *get_fbuc_members(void *fbuc)
{
    FBUCMembers *members;
    members = (FBUCMembers *)&((uint8_t *)fbuc)[FBUC_MEMBERS_OFFSET];
    return members;
}

static void fbuc_install_external_method(void *fbuc,
                                         IOExternalMethodAction func,
                                         uint64_t index,
                                         uint32_t scic,
                                         uint32_t stic,
                                         uint32_t scoc,
                                         uint32_t stoc)
{
    if (index >= FBUC_MAX_EXT_FUNCS) {
        cancel();
    }
    FBUCMembers *members = get_fbuc_members(fbuc);
    members->fbuc_external_methods[index].function = func;
    members->fbuc_external_methods[index].checkScalarInputCount = scic;
    members->fbuc_external_methods[index].checkStructureInputSize = stic;
    members->fbuc_external_methods[index].checkScalarOutputCount = scoc;
    members->fbuc_external_methods[index].checkStructureOutputSize = stoc;
}

static uint64_t fbuc_ext_meth_get_layer_default_sur(void *target,
                                                    void *reference,
                                          IOExternalMethodArguments *arguments)
{
    //TODO: JONATHANA delete me
    IOLog("JONATHANA fbuc_ext_meth_get_layer_default_sur()\n");
    //TODO: JONATHANA maybe this contrtols the surface ID??
    //what is it?
    //behaves like a regular iphone 7
    arguments->scalarOutput[0] = 0x30e0b;
    return 0;
}

static uint64_t fbuc_ext_meth_swap_begin(void *target, void *reference,
                                         IOExternalMethodArguments *arguments)
{
    //TODO: JONATHANA delete me
    IOLog("JONATHANA fbuc_ext_meth_swap_begin()\n");
    //TODO: JONATHANA maybe this contrtols the surface ID??
    //indeed seems it is the surface id? need to verify
    arguments->scalarOutput[0] = 1;
    return 0;
}

static uint64_t fbuc_ext_meth_swap_end(void *target, void *reference,
                                       IOExternalMethodArguments *arguments)
{
    //TODO: JONATHANA delete me
    IOLog("JONATHANA fbuc_ext_meth_swap_end()\n");
    FBUCMembers *members = get_fbuc_members(target);
    //TODO: JONATHANA have to figure out how to determine the relevant
    //surface index, might need to reverse the iOS framebuffer driver
    //for this
    void *s = NULL;
    void *map = NULL;
    uint8_t *vaddr = NULL;
    uint64_t len = 0;
    for (uint64_t j = 20; j <= 20; j--) {
        s = IOSurfaceRoot_lookupSurface(members->surface_root,
                                        j, members->task);
        //TODO: JONATHANA delete me
        log_uint64("fbuc_ext_meth_swap_end(): j: ", j);
        log_uint64("fbuc_ext_meth_swap_end(): s: ", s);

        log_uint64("fbuc_ext_meth_swap_end(): members->task: ", members->task);
        log_uint64("fbuc_ext_meth_swap_end(): members->surface_root: ", members->surface_root);
        //void *s = s1;
        //TODO: JONATHANA delete me
        //if (NULL != s2) {
        //    if (NULL != s1) {
        //        IOSurface_release(s1);
        //    }
        //    s = s2;
        //}
        if (NULL == s) {
            IOLog("fbuc_ext_meth_swap_end: got NULL surface\n");
            //TODO: JOJNATHANA
            continue;
            //return 0;
            //cancel();
        }
        IOLog("JONATHANA fbuc_ext_meth_swap_end() 1\n");
        if (IOSurface_deviceLockSurface(s, 1))
        {
            IOLog("fbuc_ext_meth_swap_end: fail IOSurface_deviceLockSurface()\n");
            cancel();
        }
        IOLog("JONATHANA fbuc_ext_meth_swap_end() 2\n");
        if (IOSurface_prepare(s))
        {
            IOLog("fbuc_ext_meth_swap_end: failed IOSurface_prepare()\n");
            cancel();
        }
        IOLog("JONATHANA fbuc_ext_meth_swap_end() 3\n");
        void *mem_desc = IOSurface_getMemoryDescriptor(s);
        if (NULL == mem_desc) {
            IOLog("fbuc_ext_meth_swap_end: got 0 mem_desc\n");
            cancel();
        }
        IOLog("JONATHANA fbuc_ext_meth_swap_end() 4\n");
        len = IOMemoryDescriptor_getLength(mem_desc);
        log_uint64("JONATHANA fbuc_ext_meth_swap_end() 5: len: ", len);
        map = IOMemoryDescriptor_map(mem_desc, 0);
        if (NULL == map) {
            IOLog("fbuc_ext_meth_swap_end: got 0 map\n");
            cancel();
        }
        IOLog("JONATHANA fbuc_ext_meth_swap_end() 6\n");
        vaddr = (uint8_t *)IOMemoryMap_getVirtualAddress(map);
        if (NULL == vaddr) {
            IOLog("fbuc_ext_meth_swap_end: got 0 vaddr\n");
            cancel();
        }
        IOLog("JONATHANA fbuc_ext_meth_swap_end() 7\n");
        log_uint64("JONATHANA fbuc_ext_meth_swap_end() 7: [len/2]: ", ((uint64_t *)vaddr)[len/16]);
        uint64_t found_good_surface = 0;
        for (uint64_t i = 0; i < len/8; i ++) {
            if ((0 != ((uint64_t *)vaddr)[i]) && (18374686483949813760 != ((uint64_t *)vaddr)[i])) {
                log_uint64("JONATHANA fbuc_ext_meth_swap_end() 7: i: ", i);
                log_uint64("JONATHANA fbuc_ext_meth_swap_end() 7: [i]: ", ((uint64_t *)vaddr)[i]);
                found_good_surface = 1;
                break;
            }
        }
        if (0 != found_good_surface) {
            break;
        }
        OSObject_release(map);
        IOSurface_complete(s);
        IOSurface_deviceUnlockSurface(s, 1);
        IOSurface_release(s);
        vaddr = NULL;
    }
    if (NULL == vaddr) {
        IOLog("JONATHANA fbuc_ext_meth_swap_end() 13\n");
        return 0;
    }
    //TODO: JONATHANA think how can avoid this copy.
    //maybe find a way to display the framebuffer on the host side
    //physical segment by physical segment instead of a full cont framebuffer
    memmove(&members->fb[0], vaddr, len);

    IOLog("JONATHANA fbuc_ext_meth_swap_end() 8\n");
    OSObject_release(map);
    IOLog("JONATHANA fbuc_ext_meth_swap_end() 9\n");
    IOSurface_complete(s);
    IOLog("JONATHANA fbuc_ext_meth_swap_end() 10\n");
    IOSurface_deviceUnlockSurface(s, 1);
    IOLog("JONATHANA fbuc_ext_meth_swap_end() 11\n");
    IOSurface_release(s);
    //IOSurface_release(s1);
    //IOSurface_release(s2);
    //IOSurface_release(s3);
    //IOSurface_release(s4);
    //IOSurface_release(s5);
    IOLog("JONATHANA fbuc_ext_meth_swap_end() 12\n");
    return 0;
}

static uint64_t fbuc_ext_meth_swap_wait(void *target, void *reference,
                                        IOExternalMethodArguments *arguments)
{
    //TODO: JONATHANA delete me
    IOLog("JONATHANA fbuc_ext_meth_swap_wait()\n");
    log_uint64("0: ", arguments->scalarInput[0]); //surface id of waited for surface?
    log_uint64("1: ", arguments->scalarInput[1]); //seems like some kind of flags?
    log_uint64("2: ", arguments->scalarInput[2]); //always 0
    //TODO: JONATHANA sometimes returns 0xe00002d5, maybe if not yet displayed?
    //observed another error return code once
    return 0;
}

static uint64_t fbuc_ext_meth_get_id(void *target, void *reference,
                                     IOExternalMethodArguments *arguments)
{
    //TODO: JONATHANA delete me
    IOLog("JONATHANA fbuc_ext_meth_get_id()\n");
    //TODO: JONATHANA reconsider the return value here?
    arguments->scalarOutput[0] = 0;
    return 0;
}

static uint64_t fbuc_ext_meth_get_disp_size(void *target, void *reference,
                                          IOExternalMethodArguments *arguments)
{
    //TODO: JONATHANA delete me
    IOLog("JONATHANA fbuc_ext_meth_get_disp_size()\n");
    //TODO: JONATHANA define in main header file
    //TODO: get this input somehow
    //behaves like a regular iphone 7
    arguments->scalarOutput[0] = 0x2ee;
    arguments->scalarOutput[1] = 0x536;
    return 0;
}

static uint64_t fbuc_ext_meth_req_power_change(void *target, void *reference,
                                          IOExternalMethodArguments *arguments)
{
    //TODO: JONATHANA delete me
    IOLog("JONATHANA fbuc_ext_meth_req_power_change()\n");
    //behaves like a regular iphone 7
    return 0;
}

static uint64_t fbuc_ext_meth_set_debug_flags(void *target, void *reference,
                                          IOExternalMethodArguments *arguments)
{
    //TODO: JONATHANA delete me
    IOLog("JONATHANA fbuc_ext_meth_set_debug_flags()\n");
    //behaves like a regular iphone 7
    arguments->scalarOutput[0] = 0xaaaaaaaaaaaaaaaa;
    return 0xe00002bc;
}

static uint64_t fbuc_ext_meth_set_gamma_table(void *target,
                                              void *reference,
                                          IOExternalMethodArguments *arguments)
{
    //TODO: JONATHANA delete me
    IOLog("JONATHANA fbuc_ext_meth_set_gamma_table()\n");
    //behaves like a regular iphone 7
    return 0;
}

static uint64_t fbuc_ext_meth_is_main_disp(void *target, void *reference,
                                          IOExternalMethodArguments *arguments)
{
    //TODO: JONATHANA delete me
    IOLog("JONATHANA fbuc_ext_meth_is_main_disp()\n");
    arguments->scalarOutput[0] = 1;
    return 0;
}

static uint64_t fbuc_ext_meth_set_display_dev(void *target, void *reference,
                                          IOExternalMethodArguments *arguments)
{
    //TODO: JONATHANA delete me
    IOLog("JONATHANA fbuc_ext_meth_set_display_dev()\n");
    //behaves like a regular iphone 7
    return 0xe00002c7;
}

static uint64_t fbuc_ext_meth_get_gamma_table(void *target,
                                              void *reference,
                                          IOExternalMethodArguments *arguments)
{
    //TODO: JONATHANA delete me
    IOLog("JONATHANA fbuc_ext_meth_get_gamma_table()\n");
    //behaves like a relular iphone 7
    char gtable[] = "\x00\x00\x00\x00\x04\x00\x00\x00\x08\x00\x00\x00\x0c\x00\x00\x00\x10\x00\x00\x00\x14\x00\x00\x00\x18\x00\x00\x00\x1c\x00\x00\x00 \x00\x00\x00%\x00\x00\x00)\x00\x00\x00-\x00\x00\x001\x00\x00\x005\x00\x00\x009\x00\x00\x00=\x00\x00\x00A\x00\x00\x00E\x00\x00\x00I\x00\x00\x00M\x00\x00\x00Q\x00\x00\x00U\x00\x00\x00X\x00\x00\x00\\\x00\x00\x00`\x00\x00\x00d\x00\x00\x00h\x00\x00\x00l\x00\x00\x00p\x00\x00\x00t\x00\x00\x00x\x00\x00\x00{\x00\x00\x00\x7f\x00\x00\x00\x83\x00\x00\x00\x87\x00\x00\x00\x8b\x00\x00\x00\x8e\x00\x00\x00\x92\x00\x00\x00\x96\x00\x00\x00\x9a\x00\x00\x00\x9d\x00\x00\x00\xa1\x00\x00\x00\xa5\x00\x00\x00\xa8\x00\x00\x00\xac\x00\x00\x00\xb0\x00\x00\x00\xb3\x00\x00\x00\xb7\x00\x00\x00\xbb\x00\x00\x00\xbe\x00\x00\x00\xc2\x00\x00\x00\xc6\x00\x00\x00\xc9\x00\x00\x00\xcd\x00\x00\x00\xd0\x00\x00\x00\xd4\x00\x00\x00\xd8\x00\x00\x00\xdb\x00\x00\x00\xdf\x00\x00\x00\xe2\x00\x00\x00\xe6\x00\x00\x00\xe9\x00\x00\x00\xed\x00\x00\x00\xf0\x00\x00\x00\xf4\x00\x00\x00\xf7\x00\x00\x00\xfb\x00\x00\x00\xfe\x00\x00\x00\x02\x01\x00\x00\x05\x01\x00\x00\x09\x01\x00\x00\x0c\x01\x00\x00\x0f\x01\x00\x00\x13\x01\x00\x00\x16\x01\x00\x00\x1a\x01\x00\x00\x1d\x01\x00\x00 \x01\x00\x00$\x01\x00\x00'\x01\x00\x00+\x01\x00\x00.\x01\x00\x002\x01\x00\x005\x01\x00\x009\x01\x00\x00<\x01\x00\x00@\x01\x00\x00C\x01\x00\x00G\x01\x00\x00J\x01\x00\x00N\x01\x00\x00Q\x01\x00\x00U\x01\x00\x00X\x01\x00\x00\\\x01\x00\x00`\x01\x00\x00c\x01\x00\x00g\x01\x00\x00j\x01\x00\x00n\x01\x00\x00q\x01\x00\x00u\x01\x00\x00y\x01\x00\x00|\x01\x00\x00\x80\x01\x00\x00\x83\x01\x00\x00\x87\x01\x00\x00\x8b\x01\x00\x00\x8e\x01\x00\x00\x92\x01\x00\x00\x95\x01\x00\x00\x99\x01\x00\x00\x9d\x01\x00\x00\xa0\x01\x00\x00\xa4\x01\x00\x00\xa8\x01\x00\x00\xab\x01\x00\x00\xaf\x01\x00\x00\xb3\x01\x00\x00\xb6\x01\x00\x00\xba\x01\x00\x00\xbe\x01\x00\x00\xc2\x01\x00\x00\xc5\x01\x00\x00\xc9\x01\x00\x00\xcd\x01\x00\x00\xd1\x01\x00\x00\xd4\x01\x00\x00\xd8\x01\x00\x00\xdc\x01\x00\x00\xe0\x01\x00\x00\xe3\x01\x00\x00\xe7\x01\x00\x00\xeb\x01\x00\x00\xef\x01\x00\x00\xf3\x01\x00\x00\xf7\x01\x00\x00\xfb\x01\x00\x00\xfe\x01\x00\x00\x02\x02\x00\x00\x06\x02\x00\x00\x0a\x02\x00\x00\x0e\x02\x00\x00\x12\x02\x00\x00\x16\x02\x00\x00\x1a\x02\x00\x00\x1e\x02\x00\x00\"\x02\x00\x00&\x02\x00\x00*\x02\x00\x00.\x02\x00\x002\x02\x00\x006\x02\x00\x00:\x02\x00\x00>\x02\x00\x00B\x02\x00\x00F\x02\x00\x00J\x02\x00\x00N\x02\x00\x00R\x02\x00\x00V\x02\x00\x00Z\x02\x00\x00^\x02\x00\x00b\x02\x00\x00f\x02\x00\x00j\x02\x00\x00n\x02\x00\x00r\x02\x00\x00v\x02\x00\x00z\x02\x00\x00~\x02\x00\x00\x82\x02\x00\x00\x86\x02\x00\x00\x8a\x02\x00\x00\x8e\x02\x00\x00\x92\x02\x00\x00\x96\x02\x00\x00\x9a\x02\x00\x00\x9e\x02\x00\x00\xa3\x02\x00\x00\xa7\x02\x00\x00\xab\x02\x00\x00\xaf\x02\x00\x00\xb3\x02\x00\x00\xb7\x02\x00\x00\xbb\x02\x00\x00\xbf\x02\x00\x00\xc3\x02\x00\x00\xc7\x02\x00\x00\xcc\x02\x00\x00\xd0\x02\x00\x00\xd4\x02\x00\x00\xd8\x02\x00\x00\xdc\x02\x00\x00\xe0\x02\x00\x00\xe4\x02\x00\x00\xe9\x02\x00\x00\xed\x02\x00\x00\xf1\x02\x00\x00\xf5\x02\x00\x00\xf9\x02\x00\x00\xfd\x02\x00\x00\x02\x03\x00\x00\x06\x03\x00\x00\x0a\x03\x00\x00\x0e\x03\x00\x00\x12\x03\x00\x00\x16\x03\x00\x00\x1b\x03\x00\x00\x1f\x03\x00\x00#\x03\x00\x00'\x03\x00\x00+\x03\x00\x00/\x03\x00\x004\x03\x00\x008\x03\x00\x00<\x03\x00\x00@\x03\x00\x00D\x03\x00\x00H\x03\x00\x00L\x03\x00\x00P\x03\x00\x00U\x03\x00\x00Y\x03\x00\x00]\x03\x00\x00a\x03\x00\x00e\x03\x00\x00i\x03\x00\x00m\x03\x00\x00q\x03\x00\x00u\x03\x00\x00y\x03\x00\x00}\x03\x00\x00\x82\x03\x00\x00\x86\x03\x00\x00\x8a\x03\x00\x00\x8e\x03\x00\x00\x92\x03\x00\x00\x96\x03\x00\x00\x9a\x03\x00\x00\x9e\x03\x00\x00\xa2\x03\x00\x00\xa6\x03\x00\x00\xaa\x03\x00\x00\xae\x03\x00\x00\xb2\x03\x00\x00\xb6\x03\x00\x00\xba\x03\x00\x00\xbe\x03\x00\x00\xc2\x03\x00\x00\xc6\x03\x00\x00\xcb\x03\x00\x00\xcf\x03\x00\x00\xd3\x03\x00\x00\xd7\x03\x00\x00\xdb\x03\x00\x00\xdf\x03\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00\x08\x00\x00\x00\x0c\x00\x00\x00\x11\x00\x00\x00\x15\x00\x00\x00\x19\x00\x00\x00\x1d\x00\x00\x00!\x00\x00\x00&\x00\x00\x00*\x00\x00\x00.\x00\x00\x002\x00\x00\x006\x00\x00\x00:\x00\x00\x00?\x00\x00\x00C\x00\x00\x00G\x00\x00\x00K\x00\x00\x00O\x00\x00\x00S\x00\x00\x00W\x00\x00\x00[\x00\x00\x00_\x00\x00\x00c\x00\x00\x00g\x00\x00\x00k\x00\x00\x00o\x00\x00\x00s\x00\x00\x00w\x00\x00\x00{\x00\x00\x00\x7f\x00\x00\x00\x83\x00\x00\x00\x87\x00\x00\x00\x8b\x00\x00\x00\x8f\x00\x00\x00\x93\x00\x00\x00\x97\x00\x00\x00\x9b\x00\x00\x00\x9e\x00\x00\x00\xa2\x00\x00\x00\xa6\x00\x00\x00\xaa\x00\x00\x00\xae\x00\x00\x00\xb2\x00\x00\x00\xb5\x00\x00\x00\xb9\x00\x00\x00\xbd\x00\x00\x00\xc1\x00\x00\x00\xc4\x00\x00\x00\xc8\x00\x00\x00\xcc\x00\x00\x00\xd0\x00\x00\x00\xd3\x00\x00\x00\xd7\x00\x00\x00\xdb\x00\x00\x00\xde\x00\x00\x00\xe2\x00\x00\x00\xe6\x00\x00\x00\xea\x00\x00\x00\xed\x00\x00\x00\xf1\x00\x00\x00\xf5\x00\x00\x00\xf8\x00\x00\x00\xfc\x00\x00\x00\xff\x00\x00\x00\x03\x01\x00\x00\x07\x01\x00\x00\x0a\x01\x00\x00\x0e\x01\x00\x00\x11\x01\x00\x00\x15\x01\x00\x00\x18\x01\x00\x00\x1c\x01\x00\x00\x1f\x01\x00\x00#\x01\x00\x00&\x01\x00\x00*\x01\x00\x00.\x01\x00\x001\x01\x00\x005\x01\x00\x008\x01\x00\x00<\x01\x00\x00@\x01\x00\x00C\x01\x00\x00G\x01\x00\x00K\x01\x00\x00N\x01\x00\x00R\x01\x00\x00U\x01\x00\x00Y\x01\x00\x00]\x01\x00\x00`\x01\x00\x00d\x01\x00\x00h\x01\x00\x00l\x01\x00\x00o\x01\x00\x00s\x01\x00\x00w\x01\x00\x00z\x01\x00\x00~\x01\x00\x00\x82\x01\x00\x00\x85\x01\x00\x00\x89\x01\x00\x00\x8d\x01\x00\x00\x91\x01\x00\x00\x94\x01\x00\x00\x98\x01\x00\x00\x9c\x01\x00\x00\xa0\x01\x00\x00\xa3\x01\x00\x00\xa7\x01\x00\x00\xab\x01\x00\x00\xaf\x01\x00\x00\xb2\x01\x00\x00\xb6\x01\x00\x00\xba\x01\x00\x00\xbe\x01\x00\x00\xc2\x01\x00\x00\xc5\x01\x00\x00\xc9\x01\x00\x00\xcd\x01\x00\x00\xd1\x01\x00\x00\xd5\x01\x00\x00\xd9\x01\x00\x00\xdc\x01\x00\x00\xe0\x01\x00\x00\xe4\x01\x00\x00\xe8\x01\x00\x00\xec\x01\x00\x00\xf0\x01\x00\x00\xf4\x01\x00\x00\xf8\x01\x00\x00\xfc\x01\x00\x00\xff\x01\x00\x00\x03\x02\x00\x00\x07\x02\x00\x00\x0b\x02\x00\x00\x0f\x02\x00\x00\x13\x02\x00\x00\x17\x02\x00\x00\x1b\x02\x00\x00\x1f\x02\x00\x00#\x02\x00\x00'\x02\x00\x00+\x02\x00\x00/\x02\x00\x003\x02\x00\x007\x02\x00\x00;\x02\x00\x00?\x02\x00\x00C\x02\x00\x00G\x02\x00\x00K\x02\x00\x00O\x02\x00\x00S\x02\x00\x00W\x02\x00\x00[\x02\x00\x00_\x02\x00\x00c\x02\x00\x00h\x02\x00\x00l\x02\x00\x00p\x02\x00\x00t\x02\x00\x00x\x02\x00\x00|\x02\x00\x00\x80\x02\x00\x00\x84\x02\x00\x00\x88\x02\x00\x00\x8c\x02\x00\x00\x91\x02\x00\x00\x95\x02\x00\x00\x99\x02\x00\x00\x9d\x02\x00\x00\xa1\x02\x00\x00\xa5\x02\x00\x00\xa9\x02\x00\x00\xad\x02\x00\x00\xb2\x02\x00\x00\xb6\x02\x00\x00\xba\x02\x00\x00\xbe\x02\x00\x00\xc2\x02\x00\x00\xc7\x02\x00\x00\xcb\x02\x00\x00\xcf\x02\x00\x00\xd3\x02\x00\x00\xd7\x02\x00\x00\xdc\x02\x00\x00\xe0\x02\x00\x00\xe4\x02\x00\x00\xe8\x02\x00\x00\xed\x02\x00\x00\xf1\x02\x00\x00\xf5\x02\x00\x00\xf9\x02\x00\x00\xfe\x02\x00\x00\x02\x03\x00\x00\x06\x03\x00\x00\x0b\x03\x00\x00\x0f\x03\x00\x00\x13\x03\x00\x00\x18\x03\x00\x00\x1c\x03\x00\x00 \x03\x00\x00$\x03\x00\x00)\x03\x00\x00-\x03\x00\x001\x03\x00\x006\x03\x00\x00:\x03\x00\x00>\x03\x00\x00C\x03\x00\x00G\x03\x00\x00K\x03\x00\x00P\x03\x00\x00T\x03\x00\x00X\x03\x00\x00]\x03\x00\x00a\x03\x00\x00e\x03\x00\x00j\x03\x00\x00n\x03\x00\x00r\x03\x00\x00w\x03\x00\x00{\x03\x00\x00\x7f\x03\x00\x00\x84\x03\x00\x00\x88\x03\x00\x00\x8c\x03\x00\x00\x91\x03\x00\x00\x95\x03\x00\x00\x9a\x03\x00\x00\x9e\x03\x00\x00\xa3\x03\x00\x00\xa7\x03\x00\x00\xac\x03\x00\x00\xb0\x03\x00\x00\xb5\x03\x00\x00\xb9\x03\x00\x00\xbe\x03\x00\x00\xc2\x03\x00\x00\xc7\x03\x00\x00\xcb\x03\x00\x00\xcf\x03\x00\x00\xd4\x03\x00\x00\xd8\x03\x00\x00\xdc\x03\x00\x00\xe0\x03\x00\x00\xe5\x03\x00\x00\xe9\x03\x00\x00\xed\x03\x00\x00\xf1\x03\x00\x00\xf5\x03\x00\x00\xf8\x03\x00\x00\xfc\x03\x00\x00\x00\x04\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00\x08\x00\x00\x00\x0c\x00\x00\x00\x10\x00\x00\x00\x14\x00\x00\x00\x18\x00\x00\x00\x1c\x00\x00\x00 \x00\x00\x00$\x00\x00\x00(\x00\x00\x00,\x00\x00\x000\x00\x00\x004\x00\x00\x008\x00\x00\x00<\x00\x00\x00@\x00\x00\x00D\x00\x00\x00H\x00\x00\x00L\x00\x00\x00P\x00\x00\x00T\x00\x00\x00X\x00\x00\x00[\x00\x00\x00_\x00\x00\x00c\x00\x00\x00g\x00\x00\x00k\x00\x00\x00o\x00\x00\x00s\x00\x00\x00w\x00\x00\x00z\x00\x00\x00~\x00\x00\x00\x82\x00\x00\x00\x86\x00\x00\x00\x8a\x00\x00\x00\x8d\x00\x00\x00\x91\x00\x00\x00\x95\x00\x00\x00\x99\x00\x00\x00\x9c\x00\x00\x00\xa0\x00\x00\x00\xa4\x00\x00\x00\xa8\x00\x00\x00\xab\x00\x00\x00\xaf\x00\x00\x00\xb3\x00\x00\x00\xb6\x00\x00\x00\xba\x00\x00\x00\xbe\x00\x00\x00\xc1\x00\x00\x00\xc5\x00\x00\x00\xc9\x00\x00\x00\xcc\x00\x00\x00\xd0\x00\x00\x00\xd4\x00\x00\x00\xd7\x00\x00\x00\xdb\x00\x00\x00\xde\x00\x00\x00\xe2\x00\x00\x00\xe6\x00\x00\x00\xe9\x00\x00\x00\xed\x00\x00\x00\xf0\x00\x00\x00\xf4\x00\x00\x00\xf7\x00\x00\x00\xfb\x00\x00\x00\xfe\x00\x00\x00\x02\x01\x00\x00\x05\x01\x00\x00\x09\x01\x00\x00\x0c\x01\x00\x00\x10\x01\x00\x00\x13\x01\x00\x00\x17\x01\x00\x00\x1a\x01\x00\x00\x1e\x01\x00\x00!\x01\x00\x00%\x01\x00\x00(\x01\x00\x00,\x01\x00\x00/\x01\x00\x003\x01\x00\x006\x01\x00\x00:\x01\x00\x00=\x01\x00\x00A\x01\x00\x00D\x01\x00\x00H\x01\x00\x00L\x01\x00\x00O\x01\x00\x00S\x01\x00\x00V\x01\x00\x00Z\x01\x00\x00]\x01\x00\x00a\x01\x00\x00d\x01\x00\x00h\x01\x00\x00k\x01\x00\x00o\x01\x00\x00s\x01\x00\x00v\x01\x00\x00z\x01\x00\x00}\x01\x00\x00\x81\x01\x00\x00\x85\x01\x00\x00\x88\x01\x00\x00\x8c\x01\x00\x00\x8f\x01\x00\x00\x93\x01\x00\x00\x97\x01\x00\x00\x9a\x01\x00\x00\x9e\x01\x00\x00\xa1\x01\x00\x00\xa5\x01\x00\x00\xa9\x01\x00\x00\xac\x01\x00\x00\xb0\x01\x00\x00\xb4\x01\x00\x00\xb7\x01\x00\x00\xbb\x01\x00\x00\xbf\x01\x00\x00\xc3\x01\x00\x00\xc6\x01\x00\x00\xca\x01\x00\x00\xce\x01\x00\x00\xd2\x01\x00\x00\xd5\x01\x00\x00\xd9\x01\x00\x00\xdd\x01\x00\x00\xe1\x01\x00\x00\xe5\x01\x00\x00\xe8\x01\x00\x00\xec\x01\x00\x00\xf0\x01\x00\x00\xf4\x01\x00\x00\xf8\x01\x00\x00\xfc\x01\x00\x00\x00\x02\x00\x00\x03\x02\x00\x00\x07\x02\x00\x00\x0b\x02\x00\x00\x0f\x02\x00\x00\x13\x02\x00\x00\x17\x02\x00\x00\x1b\x02\x00\x00\x1f\x02\x00\x00#\x02\x00\x00'\x02\x00\x00+\x02\x00\x00/\x02\x00\x003\x02\x00\x007\x02\x00\x00;\x02\x00\x00?\x02\x00\x00C\x02\x00\x00G\x02\x00\x00K\x02\x00\x00O\x02\x00\x00S\x02\x00\x00W\x02\x00\x00[\x02\x00\x00_\x02\x00\x00c\x02\x00\x00g\x02\x00\x00k\x02\x00\x00o\x02\x00\x00t\x02\x00\x00x\x02\x00\x00|\x02\x00\x00\x80\x02\x00\x00\x84\x02\x00\x00\x88\x02\x00\x00\x8c\x02\x00\x00\x90\x02\x00\x00\x94\x02\x00\x00\x98\x02\x00\x00\x9c\x02\x00\x00\xa1\x02\x00\x00\xa5\x02\x00\x00\xa9\x02\x00\x00\xad\x02\x00\x00\xb1\x02\x00\x00\xb5\x02\x00\x00\xb9\x02\x00\x00\xbd\x02\x00\x00\xc2\x02\x00\x00\xc6\x02\x00\x00\xca\x02\x00\x00\xce\x02\x00\x00\xd2\x02\x00\x00\xd6\x02\x00\x00\xdb\x02\x00\x00\xdf\x02\x00\x00\xe3\x02\x00\x00\xe7\x02\x00\x00\xeb\x02\x00\x00\xf0\x02\x00\x00\xf4\x02\x00\x00\xf8\x02\x00\x00\xfc\x02\x00\x00\x01\x03\x00\x00\x05\x03\x00\x00\x09\x03\x00\x00\x0d\x03\x00\x00\x12\x03\x00\x00\x16\x03\x00\x00\x1a\x03\x00\x00\x1e\x03\x00\x00#\x03\x00\x00'\x03\x00\x00+\x03\x00\x00/\x03\x00\x004\x03\x00\x008\x03\x00\x00<\x03\x00\x00@\x03\x00\x00E\x03\x00\x00I\x03\x00\x00M\x03\x00\x00Q\x03\x00\x00V\x03\x00\x00Z\x03\x00\x00^\x03\x00\x00c\x03\x00\x00g\x03\x00\x00k\x03\x00\x00o\x03\x00\x00t\x03\x00\x00x\x03\x00\x00|\x03\x00\x00\x80\x03\x00\x00\x84\x03\x00\x00\x88\x03\x00\x00\x8d\x03\x00\x00\x91\x03\x00\x00\x95\x03\x00\x00\x99\x03\x00\x00\x9d\x03\x00\x00\xa2\x03\x00\x00\xa6\x03\x00\x00\xaa\x03\x00\x00\xae\x03\x00\x00\xb3\x03\x00\x00\xb7\x03\x00\x00\xbb\x03\x00\x00\xc0\x03\x00\x00\xc4\x03\x00\x00\xc9\x03\x00\x00\xcd\x03\x00\x00\xd2\x03\x00\x00\xd7\x03\x00\x00\xdc\x03\x00\x00\xe1\x03\x00\x00\xe6\x03\x00\x00\xeb\x03\x00\x00\xf1\x03\x00\x00";
    memcpy(arguments->structureOutput, gtable, arguments->structureOutputSize);
    return 0;
}

static uint64_t fbuc_ext_meth_get_dot_pitch(void *target,
                                            void *reference,
                                          IOExternalMethodArguments *arguments)
{
    //TODO: JONATHANA delete me
    IOLog("JONATHANA fbuc_ext_meth_get_dot_pitch()\n");
    //behaves like a relular iphone 7
    arguments->scalarOutput[0] = 0x146;
    return 0;
}

static uint64_t fbuc_ext_meth_get_display_area(void *target,
                                               void *reference,
                                          IOExternalMethodArguments *arguments)
{
    //TODO: JONATHANA delete me
    IOLog("JONATHANA fbuc_ext_meth_get_display_area()\n");
    uint8_t *data = (uint8_t *)arguments->structureOutput;
    //TODO: JONATHANA get these numbers externally
    //behaves like a relular iphone 7
    data[0] = 0xdd;
    data[1] = 0xf1;
    data[2] = 0x82;
    data[3] = '@';
    data[4] = '@';
    data[5] = '=';
    data[6] = 0x13;
    data[7] = '@';
    return 0;
}

static uint64_t fbuc_ext_meth_en_dis_vid_power_save(void *target,
                                                    void *reference,
                                          IOExternalMethodArguments *arguments)
{
    //TODO: JONATHANA delete me
    IOLog("JONATHANA fbuc_ext_meth_en_dis_vid_power_save()\n");
    //behaves like a relular iphone 7
    return 0;
}

static uint64_t fbuc_ext_meth_surface_is_rep(void *target,
                                             void *reference,
                                          IOExternalMethodArguments *arguments)
{
    //TODO: JONATHANA delete me
    IOLog("JONATHANA fbuc_ext_meth_surface_is_rep()\n");
    arguments->scalarOutput[0] = 1;
    return 0;
}

static uint64_t fbuc_ext_meth_set_bright_corr(void *target,
                                              void *reference,
                                          IOExternalMethodArguments *arguments)
{
    //TODO: JONATHANA delete me
    IOLog("JONATHANA fbuc_ext_meth_set_bright_corr()\n");
    log_uint64("0: ", arguments->scalarInput[0]);
    //behaves like an iphone 7
    return 0;
}

static uint64_t fbuc_ext_meth_set_matrix(void *target,
                                         void *reference,
                                         IOExternalMethodArguments *arguments)
{
    //TODO: JONATHANA delete me
    IOLog("JONATHANA fbuc_ext_meth_set_matrix()\n");
    return 0;
}

static uint64_t fbuc_ext_meth_get_color_remap_mode(void *target,
                                                   void *reference,
                                          IOExternalMethodArguments *arguments)
{
    //TODO: JONATHANA delete me
    IOLog("JONATHANA fbuc_ext_meth_get_color_remap_mode()\n");
    //behaves like an iphone 7
    arguments->scalarOutput[0] = 0;
    return 0;
}

static uint64_t fbuc_ext_meth_set_parameter(void *target,
                                            void *reference,
                                          IOExternalMethodArguments *arguments)
{
    //TODO: JONATHANA delete me
    IOLog("JONATHANA fbuc_ext_meth_set_parameter()\n");
    //behaves like an iphone 7
    return 0;
}

static uint64_t fbuc_ext_meth_enable_notifications(void *target,
                                                   void *reference,
                                          IOExternalMethodArguments *arguments)
{
    //TODO: JONATHANA delete me
    IOLog("JONATHANA fbuc_ext_meth_enable_notifications()\n");
    //TODO: JONATHANA actually implement this to enable tofications
    FBUCMembers *members = get_fbuc_members(target);
    uint64_t cb = arguments->scalarInput[0];
    uint64_t ref = arguments->scalarInput[1];
    uint64_t type = arguments->scalarInput[2];
    log_uint64("JONATHANA fbuc_ext_meth_enable_notifications(): ", cb);
    log_uint64("JONATHANA fbuc_ext_meth_enable_notifications(): ", ref);
    log_uint64("JONATHANA fbuc_ext_meth_enable_notifications(): ", type);
    IOUserClient_setAsyncReference64(&members->notif_ports[type].asnyc_ref64[0],
                                     members->notif_ports[type].port,
                                     cb, ref, members->task);
    return 0;
}

static uint64_t fbuc_ext_meth_change_frame_info(void *target,
                                                void *reference,
                                          IOExternalMethodArguments *arguments)
{
    //TODO: JONATHANA delete me
    IOLog("JONATHANA fbuc_ext_meth_change_frame_info()\n");
    //TODO: JONATHANA actually implement this
    //FBUCMembers *members = get_fbuc_members(target);

    //uint64_t args[2];
    //uint64_t type = arguments->scalarInput[0];
    //args[0] = type;
    //args[1] = arguments->scalarInput[1];
    //IOUserClient_sendAsyncResult64(&members->notif_ports[type].asnyc_ref64[0],
    //                               0, &args[0], 2);
    //uint64_t args[6];
    //uint64_t type = arguments->scalarInput[0];
    //args[0] = 5;
    //args[1] = 6;
    //args[2] = 7;
    //args[3] = 8;
    //args[4] = 9;
    //args[5] = 10;
    //IOUserClient_sendAsyncResult64(&members->notif_ports[type].asnyc_ref64[0],
    //                               0, &args[0], 6);
    //TODO: JONATHANA figure out if/when we need to call the callbacks
    return 0;
}

static uint64_t fbuc_ext_meth_supported_frame_info(void *target,
                                                   void *reference,
                                          IOExternalMethodArguments *arguments)
{
    //TODO: JONATHANA delete me
    IOLog("JONATHANA fbuc_ext_meth_supported_frame_info()\n");
    //Ugly trick we have to do because of how we compile this code
    switch (arguments->scalarInput[0])
    {
        case 0:
        arguments->scalarOutput[0] = 8;
        strncpy(arguments->structureOutput, "Blend_CRC",
                arguments->structureOutputSize);
        break;
        case 1:
        arguments->scalarOutput[0] = 8;
        strncpy(arguments->structureOutput, "Dither_CRC",
                arguments->structureOutputSize);
        break;
        case 2:
        arguments->scalarOutput[0] = 8;
        strncpy(arguments->structureOutput, "Presentation_time",
                arguments->structureOutputSize);
        break;
        case 3:
        arguments->scalarOutput[0] = 8;
        strncpy(arguments->structureOutput, "Presentation_delta",
                arguments->structureOutputSize);
        break;
        case 4:
        arguments->scalarOutput[0] = 8;
        strncpy(arguments->structureOutput, "Requested_presentation",
                arguments->structureOutputSize);
        break;
        case 5:
        arguments->scalarOutput[0] = 8;
        strncpy(arguments->structureOutput, "Performance_headroom",
                arguments->structureOutputSize);
        break;
        case 6:
        arguments->scalarOutput[0] = 8;
        strncpy(arguments->structureOutput, "Thermal_throttle",
                arguments->structureOutputSize);
        break;
        case 7:
        arguments->scalarOutput[0] = 8;
        strncpy(arguments->structureOutput, "Flags",
                arguments->structureOutputSize);
        break;
        default:
        IOLog("fbuc_ext_meth_supported_frame_info(): default case\n");
        cancel();
    }

    arguments->scalarOutput[0] = 8;
    return 0;
}

//virtual functions of our driver
void *fbuc_getMetaClass(void *this)
{
    return get_fbuc_mclass_inst();
}

uint64_t fbuc_start(void *this)
{
    fbuc_install_external_method(this, &fbuc_ext_meth_get_layer_default_sur,
                                 3, 2, 0, 1, 0);
    fbuc_install_external_method(this, &fbuc_ext_meth_swap_begin,
                                 4, 0, 0, 1, 0);
    fbuc_install_external_method(this, &fbuc_ext_meth_swap_end,
                                 5, 0, -1, 0, 0);
    fbuc_install_external_method(this, &fbuc_ext_meth_swap_wait,
                                 6, 3, 0, 0, 0);
    fbuc_install_external_method(this, &fbuc_ext_meth_get_id, 7, 0, 0, 1, 0);
    fbuc_install_external_method(this, &fbuc_ext_meth_get_disp_size,
                                 8, 0, 0, 2, 0);
    fbuc_install_external_method(this, &fbuc_ext_meth_req_power_change,
                                 12, 1, 0, 0, 0);
    fbuc_install_external_method(this, &fbuc_ext_meth_set_debug_flags,
                                 15, 2, 0, 1, 0);
    fbuc_install_external_method(this, &fbuc_ext_meth_set_gamma_table,
                                 17, 0, 3084, 0, 0);
    fbuc_install_external_method(this, &fbuc_ext_meth_is_main_disp,
                                 18, 0, 0, 1, 0);
    fbuc_install_external_method(this, &fbuc_ext_meth_set_display_dev,
                                 22, 1, 0, 0, 0);
    fbuc_install_external_method(this, &fbuc_ext_meth_get_gamma_table,
                                 27, 0, 0, 0, 3084);
    fbuc_install_external_method(this, &fbuc_ext_meth_get_dot_pitch,
                                 28, 0, 0, 1, 0);
    fbuc_install_external_method(this, &fbuc_ext_meth_get_display_area,
                                 29, 0, 0, 0, 8);
    fbuc_install_external_method(this, &fbuc_ext_meth_en_dis_vid_power_save,
                                 33, 1, 0, 0, 0);
    fbuc_install_external_method(this, &fbuc_ext_meth_surface_is_rep,
                                 49, 1, 0, 1, 0);
    fbuc_install_external_method(this, &fbuc_ext_meth_set_bright_corr,
                                 50, 1, 0, 0, 0);
    fbuc_install_external_method(this, &fbuc_ext_meth_set_matrix,
                                 55, 1, 72, 0, 0);
    fbuc_install_external_method(this, &fbuc_ext_meth_get_color_remap_mode,
                                 57, 0, 0, 1, 0);
    fbuc_install_external_method(this, &fbuc_ext_meth_set_parameter,
                                 68, 1, 8, 0, 0);
    fbuc_install_external_method(this, &fbuc_ext_meth_enable_notifications,
                                 72, 4, 0, 0, 0);
    fbuc_install_external_method(this, &fbuc_ext_meth_change_frame_info,
                                 73, 2, 0, 0, 0);
    fbuc_install_external_method(this, &fbuc_ext_meth_supported_frame_info,
                                 74, 1, 0, 1, 64);
    FBUCMembers *members = get_fbuc_members(this);
    //TODO: release this object ref
    void *match_dict = IOService_serviceMatching("IOSurfaceRoot", NULL);
    //TODO: release this object ref
    void *service = waitForMatchingService(match_dict, -1);
    if (NULL == service) {
        IOLog("fbuc_start(): got NULL service\n");
        cancel();
    }
    members->surface_root = service;
    members->qcall_vaddr = qemu_call_status();
    if (0 == members->qcall_vaddr) {
        IOLog("fbuc_start(): got 0 members->qcall_vaddr\n");
        cancel();
    }
    qemu_call_t *qcall = (qemu_call_t *)members->qcall_vaddr;
    qcall->call_number = QC_VALUE_CB;
    qcall->args.general.id = XNU_FB_GET_VADDR_QEMU_CALL;
    qemu_call(qcall);
    if (0 == qcall->args.general.data1) {
        IOLog("fbuc_start(): got 0 qcall->args.general.data1\n");
        cancel();
    }
    members->fb = (uint8_t *)qcall->args.general.data1;
    return IOService_start(this);
}

uint64_t fbuc_externalMethod(void *this, uint32_t selector,
                             IOExternalMethodArguments *args,
                             IOExternalMethodDispatch *dispatch,
                             void *target, void *reference)
{
        //TODO: JONATHANA delete me
    log_uint64("fbuc_externalMethod(): ", selector);
    IOExternalMethodDispatch *new_dispatch;
    FBUCMembers *members = get_fbuc_members(this);


    if ((selector >= FBUC_MAX_EXT_FUNCS) ||
        (0 == members->fbuc_external_methods[selector].function)) {
        cancel();
    }
    new_dispatch = &members->fbuc_external_methods[selector];
    return IOUserClient_externalMethod(this, selector, args, new_dispatch,
                                       this, reference);
}

uint64_t fbuc_clientClose(void *this)
{
    if (!IOService_terminate(this, 0)) {
        IOLog("fbuc_clientClose(): terminate() failed\n");
    }
    return 0;
}

uint64_t fbuc_connectClient(void *this, void *user_client)
{
    IOLog("fbuc_connectClient(): unsupported vfunc\n");
    cancel();
    return 0;
}

uint64_t fbuc_getNotificationSemaphore(void *this, uint64_t type, void *sem)
{
    IOLog("fbuc_getNotificationSemaphore(): unsupported vfunc\n");
    cancel();
    return 0;
}

uint64_t fbuc_clientMemoryForType(void *this, uint64_t type, void *opts,
                                  void **mem)
{
    IOLog("fbuc_clientMemoryForType(): unsupported vfunc\n");
    cancel();
    return 0;
}

uint64_t fbuc_registerNotificationPort(void *this, void *port,
                                       uint64_t type, uint64_t ref)
{
    FBUCMembers *members = get_fbuc_members(this);
    if (type >= FBUC_MAX_NOTIF_PORTS) {
        cancel();
    }
    members->notif_ports[type].type = type;
    members->notif_ports[type].port = port;
    members->notif_ports[type].ref = ref;
    return 0;
}

uint64_t fbuc_initWithTask(void *this, void *task, void *sid, uint64_t type)
{
        //TODO: JONATHANA delete me
    IOLog("JONATHANA fbuc_initWithTask()\n");
    FBUCMembers *members = get_fbuc_members(this);
    members->task = task;
    return IOUserClient_initWithTask(this, task, sid, type);
}

uint64_t fbuc_destructor(void *this)
{
    void *obj = IOUserClient_destructor(this);
    OSObject_delete(obj, ALEPH_FBUC_SIZE);
    return 0;
}
