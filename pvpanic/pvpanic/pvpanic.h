/*
 * Copyright (C) 2015-2017 Red Hat, Inc.
 *
 * Written By: Gal Hammer <ghammer@redhat.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met :
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and / or other materials provided with the distribution.
 * 3. Neither the names of the copyright holders nor the names of their contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <ntddk.h>
#include <wdf.h>

#include "trace.h"

// The bit of supported PV event.
#define PVPANIC_F_PANICKED      0

// The PV event value.
#define PVPANIC_PANICKED        (1 << PVPANIC_F_PANICKED)

// Name of the symbolic link object exposed in the guest.
// The file name visible to user space is "\\.\PVPanicDevice".
#define PVPANIC_DOS_DEVICE_NAME L"\\DosDevices\\Global\\PVPanicDevice"

// IOCTLs supported by the symbolic link object.
#define IOCTL_GET_CRASH_DUMP_HEADER CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)

typedef struct _DEVICE_CONTEXT {

    // HW Resources.
    PVOID               IoBaseAddress;
    ULONG               IoRange;
    BOOLEAN             MappedPort;

    // IOCTL request queue.
    WDFQUEUE            IoctlQueue;

} DEVICE_CONTEXT, *PDEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_CONTEXT, GetDeviceContext);

#define PVPANIC_DRIVER_MEMORY_TAG (ULONG)'npVP'

// Referenced in MSDN but not declared in SDK/WDK headers.
#define DUMP_TYPE_FULL 1

//
// Bug check callback registration functions.
//

BOOLEAN PVPanicRegisterBugCheckCallback(IN PVOID PortAddress);
BOOLEAN PVPanicDeregisterBugCheckCallback();

//
// WDFDRIVER events.
//

DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD PVPanicEvtDeviceAdd;
EVT_WDF_OBJECT_CONTEXT_CLEANUP PVPanicEvtDriverContextCleanup;

EVT_WDF_DEVICE_PREPARE_HARDWARE PVPanicEvtDevicePrepareHardware;
EVT_WDF_DEVICE_RELEASE_HARDWARE PVPanicEvtDeviceReleaseHardware;
EVT_WDF_DEVICE_D0_ENTRY PVPanicEvtDeviceD0Entry;
EVT_WDF_DEVICE_D0_EXIT PVPanicEvtDeviceD0Exit;

EVT_WDF_DEVICE_FILE_CREATE PVPanicEvtDeviceFileCreate;
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL PVPanicEvtQueueDeviceControl;
