// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
// SPDX-FileNotice: Modified from the BlocksDS version to provide a PC version.
//
// Copyright (c) 2006 Michael Chisholm (Chishm)

#ifndef LIBNDS_NDS_ARM9_DLDI_H__
#define LIBNDS_NDS_ARM9_DLDI_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "dldi_asm.h"
#include "disc_io.h"

#define DLDI_MAGIC_STRING_LEN   8
#define DLDI_FRIENDLY_NAME_LEN  48

/// DLDI I/O driver interface.
typedef struct DLDI_INTERFACE
{
    /// Magic number, equal to 0xBF8DA5ED.
    ///
    /// @see dldiIsValid
    u32 magicNumber;

    /// Magic string, equal to " Chishm\0".
    ///
    /// @see dldiIsValid
    char magicString[DLDI_MAGIC_STRING_LEN];

    /// Version number.
    u8 versionNumber;

    /// Log-2 of the driver's size, in bytes.
    u8 driverSize;

    /// Flags which determine the sections that may have addresses to be fixed.
    ///
    /// @see FIX_ALL
    /// @see FIX_GLUE
    /// @see FIX_GOT
    /// @see FIX_BSS
    u8 fixSectionsFlags;

    /// Log-2 of the available maximum driver size, in bytes.
    u8 allocatedSize;

    /// User-friendly driver name.
    char friendlyName[DLDI_FRIENDLY_NAME_LEN];

    // Pointers to sections that need address fixing
    uint32_t dldiStart; ///< Start of the DLDI driver's text/data section.
    uint32_t dldiEnd; ///< End of the DLDI driver's text/data section.
    uint32_t interworkStart; ///< Start of the DLDI driver's ARM interwork section.
    uint32_t interworkEnd; ///< End of the DLDI driver's ARM interwork section.
    uint32_t gotStart; ///< Start of the DLDI driver's Global Offset Table section.
    uint32_t gotEnd; ///< End of the DLDI driver's Global Offset Table section.
    uint32_t bssStart; ///< Start of the DLDI driver's BSS section.
    uint32_t bssEnd; ///< End of the DLDI driver's BSS section.

    /// File system interface flags and functions.
    DISC_INTERFACE ioInterface;
} DLDI_INTERFACE;

/// Load a DLDI driver from a file and set up the bus permissions.
///
/// This is not directly usable as a filesystem driver.
/// @return a pointer to the loaded DLDI_INTERFACE. This allocates the number of bytes specified in driverSize.
DLDI_INTERFACE *dldiLoadFromFile(const char *path);

/// Free the memory used by the DLDI driver.
///
/// Remember to shut down the driver itself first:
///
/// ```
/// loadedDldi->ioInterface.shutdown();
/// dldiFree(loadedDldi);
/// ```
void dldiFree(DLDI_INTERFACE *dldi);

#ifdef __cplusplus
}
#endif

#endif // LIBNDS_NDS_ARM9_DLDI_H__
