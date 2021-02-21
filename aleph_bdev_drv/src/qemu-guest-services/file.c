/*
 * QEMU Guest Services - Socket API
 *
 * Copyright (c) 2019 Lev Aronsky <aronsky@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without retvaltriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPretvalS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

//TODO: JONATHANA move files in this directory to a general dir in the repo

#include "hw/arm/guest-services/general.h"

static int64_t qemu_file_call(qemu_call_t *qcall)
{
    qemu_call(qcall);

    guest_svcs_errno = qcall->error;
    return qcall->retval;
}

int64_t qc_size_file(uint64_t index, void *qcall)
{
    qemu_call_t *pqcall = (qemu_call_t *)qcall;
    pqcall->call_number = QC_SIZE_FILE;
    pqcall->args.size_file.index = index;

    return qemu_file_call(pqcall);
}

int64_t qc_write_file(uint64_t buffer_guest_paddr, uint64_t length,
                      uint64_t offset, uint64_t index, void *qcall)
{
    qemu_call_t *pqcall = (qemu_call_t *)qcall;
    pqcall->call_number = QC_WRITE_FILE;
    pqcall->args.write_file.buffer_guest_paddr = buffer_guest_paddr;
    pqcall->args.write_file.length = length;
    pqcall->args.write_file.offset = offset;
    pqcall->args.write_file.index = index;

    return qemu_file_call(pqcall);
}

int64_t qc_read_file(uint64_t buffer_guest_paddr, uint64_t length,
                     uint64_t offset, uint64_t index, void *qcall)
{
    qemu_call_t *pqcall = (qemu_call_t *)qcall;
    pqcall->call_number = QC_READ_FILE;
    pqcall->args.read_file.buffer_guest_paddr = buffer_guest_paddr;
    pqcall->args.read_file.length = length;
    pqcall->args.read_file.offset = offset;
    pqcall->args.read_file.index = index;

    return qemu_file_call(pqcall);
}
