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

#ifndef ALEPH_MCLASS_REG_H
#define ALEPH_MCLASS_REG_H

//just in case not sure how much is required
#define ALEPH_MCLASS_SIZE (0x1000)

typedef void (*VoidFunc)(void);
typedef void* (*AllocFunc)(void);

typedef struct {
    VoidFunc destructor1;
    VoidFunc destructor2;
    VoidFunc release1;
    VoidFunc getRetainCount;
    VoidFunc retain;
    VoidFunc release2;
    VoidFunc serialize;
    VoidFunc getMetaClass;
    VoidFunc isEqualTo;
    VoidFunc taggedRetain;
    VoidFunc taggedRelease1;
    VoidFunc taggedRelease2;

    //TODO: JONATHANA ifdef this function for iOS 14.
    //make better support for versions
    VoidFunc dispatch;

    AllocFunc alloc;
} MetaClassVTable;

typedef struct
{
    uint32_t version;
    uint32_t selector;
    uint64_t unknown1[3];

    uint64_t *scalarInput;
    uint32_t scalarInputCount;
    uint32_t align1;

    void *structureInput;
    uint32_t structureInputSize;
    uint32_t align2;

    void *structureInputDescriptor;

    uint64_t *scalarOutput;
    uint32_t scalarOutputCount;
    uint32_t align3;

    void *structureOutput;
    uint32_t structureOutputSize;
    uint32_t align4;

    void *structureOutputDescriptor;
} __attribute__((packed)) IOExternalMethodArguments;

typedef uint64_t (*IOExternalMethodAction)(void* target, void* reference,
                                        IOExternalMethodArguments *arguments);
typedef struct
{
    IOExternalMethodAction function;
    uint32_t checkScalarInputCount;
    uint32_t checkStructureInputSize;
    uint32_t checkScalarOutputCount;
    uint32_t checkStructureOutputSize;
} IOExternalMethodDispatch;

void add_to_classes_dict(char *name, void **mc);
void mclass_reg_alock_lock();
void mclass_reg_alock_unlock();
void mclass_reg_slock_lock();
void mclass_reg_slock_unlock();

//IOService virtual function indices

#define IOSERVICE_DESTRUCTOR_INDEX (1)
#define IOSERVICE_GETMCLASS_INDEX (7)

//TODO: JONATHANA iOS 12 with -1 from here
    //make better support for versions
#define IOSERVICE_START_INDEX (86)

//IOUserClient virtual function indices

//make better support for versions
//TODO: JONATHANA iOS 12 with -1 from here
#define IOUC_EXTERNAL_METHOD_INDEX (168)
#define IOUC_INIT_WITH_TASK_INDEX (171)
#define IOUC_CLIENT_CLOSE_INDEX (172)
#define IOUC_REG_NOTIF_PORT_INDEX (175)
#define IOUC_GET_NOTIFICATION_SEMAPHORE_INDEX (176)
#define IOUC_CONNECT_CLIENT_INDEX (177)
#define IOUC_CLIENT_MEM_FOR_TYPE_INDEX (178)

#endif
