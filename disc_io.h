// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
// SPDX-FileNotice: Modified from the BlocksDS version to provide a PC version.
//
// Copyright (c) 2006 Michael Chisholm (Chishm) and Tim Seidel (Mighty Max).

#ifndef LIBNDS_NDS_DISC_IO_H__
#define LIBNDS_NDS_DISC_IO_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "types.h"

#define FEATURE_MEDIUM_CANREAD      0x00000001 ///< This driver can be used to read sectors.
#define FEATURE_MEDIUM_CANWRITE     0x00000002 ///< This driver can be used to write sectors.
#define FEATURE_SLOT_GBA            0x00000010 ///< This driver uses Slot-2 cartridges.
#define FEATURE_SLOT_NDS            0x00000020 ///< This driver uses Slot-1 cartridges.
#define FEATURE_ARM7_CAPABLE        0x00000100 ///< This driver can be safely used from ARM7 and ARM9. BlocksDS extension.

typedef struct DISC_INTERFACE_STRUCT
{
    /// Four-byte identifier of the device type implemented by this interface.
    uint32_t           ioType;

    /// Available device features.
    ///
    /// @see FEATURE_MEDIUM_CANREAD
    /// @see FEATURE_MEDIUM_CANWRITE
    /// @see FEATURE_SLOT_GBA
    /// @see FEATURE_SLOT_NDS
    /// @see FEATURE_ARM7_CAPABLE
    uint32_t           features;

    /// Initialize the device.
    ///
    /// @return
    ///     True on success.
    uint32_t       startup;

    /// Check if the device's removable storage, if any, is inserted.
    ///
    /// @return
    ///     True if storage is available.
    uint32_t    isInserted;

    /// Read sectors from the device.
    ///
    /// Sectors are assumed to always be 512 bytes in size. Note that some
    /// drivers only support aligned buffers.
    ///
    /// @param sector
    ///     The sector number.
    /// @param numSectors
    ///     The number of sectors.
    /// @param buffer
    ///     The destination buffer.
    ///
    /// @return
    ///     True on success.
    uint32_t   readSectors;

    /// Write sectors to the device.
    ///
    /// Sectors are assumed to always be 512 bytes in size. Note that some
    /// drivers only support aligned buffers.
    ///
    /// @param sector
    ///     The sector number.
    /// @param numSectors
    ///     The number of sectors.
    /// @param buffer
    ///     The source buffer.
    ///
    /// @return
    ///     True on success.
    uint32_t  writeSectors;

    /// Reset the device's error status after an error occured.
    ///
    /// This is not used by applications. Drivers are expected to do this
    /// automatically.
    ///
    /// @return
    ///     True on success.
    uint32_t   clearStatus;

    /// Shut down the device.
    ///
    /// @return
    ///     True on success.
    uint32_t      shutdown;
} DISC_INTERFACE;

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_NDS_DISC_IO_H__
