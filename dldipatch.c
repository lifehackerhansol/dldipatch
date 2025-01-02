// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version by the BlocksDS project.
// SPDX-FileNotice: Modified from the BlocksDS version to provide a PC version.
//
// Copyright (c) 2006 Michael Chisholm (Chishm) and Tim Seidel (Mighty Max).

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "dldi.h"

const u32 DLDI_MAGIC_NUMBER = 0xBF8DA5ED;

void dldiRelocate(DLDI_INTERFACE *io, uint32_t targetAddress)
{
    u32 offset;
    u32 oldStart;
    u32 oldEnd;

    offset = targetAddress - io->dldiStart;

	printf("Relocation offset = 0x%08X\n", offset);

    oldStart = io->dldiStart;
    oldEnd = io->dldiEnd;

    // Correct all pointers to the offsets from the location of this interface
    io->dldiStart = io->dldiStart + offset;
    io->dldiEnd = io->dldiEnd + offset;
    io->interworkStart = io->interworkStart + offset;
    io->interworkEnd = io->interworkEnd + offset;
    io->gotStart = io->gotStart + offset;
    io->gotEnd = io->gotEnd + offset;
    io->bssStart = io->bssStart + offset;
    io->bssEnd = io->bssEnd + offset;

    io->ioInterface.startup =
        io->ioInterface.startup + offset;
    io->ioInterface.isInserted =
        io->ioInterface.isInserted + offset;
    io->ioInterface.readSectors =
        io->ioInterface.readSectors + offset;
    io->ioInterface.writeSectors =
        io->ioInterface.writeSectors + offset;
    io->ioInterface.clearStatus =
        io->ioInterface.clearStatus + offset;
    io->ioInterface.shutdown =
        io->ioInterface.shutdown + offset;

	u8 *u8_io = (u8*)io;

    // Fix all addresses with in the DLDI
    if (io->fixSectionsFlags & FIX_ALL)
    {
        for (u32 i = 0; i < (io->dldiEnd - io->dldiStart); i++)
        {
            if (oldStart <= *(u8_io + i) && *(u8_io + i) < oldEnd)
                *(u8_io + i) += offset;
        }
    }

    // Fix the interworking glue section
    if (io->fixSectionsFlags & FIX_GLUE)
    {
        for (u32 i = io->interworkStart - io->dldiStart; i < (io->interworkEnd - io->interworkStart); i++)
        {
            if (oldStart <= *(u8_io + i) && *(u8_io + i) < oldEnd)
                *(u8_io + i) += offset;
        }
    }

    // Fix the global offset table section
    if (io->fixSectionsFlags & FIX_GOT)
    {
        for (u32 i = io->gotStart - io->dldiStart; i < (io->gotEnd - io->gotStart); i++)
        {
            if (oldStart <= *(u8_io + i) && *(u8_io + i) < oldEnd)
                *(u8_io + i) += offset;
        }
    }

    // Initialise the BSS to 0
    if (io->fixSectionsFlags & FIX_BSS)
        memset((u8_io + (io->bssStart - io->dldiStart)), 0, io->bssEnd - io->bssStart);
}

DLDI_INTERFACE *dldiLoadFromFile(const char* src_path)
{
	DLDI_INTERFACE* src_dldi = NULL;
	u8 *src_binary = NULL;

	FILE *src_file = fopen(src_path, "rb");
	if (src_file == NULL)
	{
		printf("Input file does not exist.\n");
		return NULL;
	}
	fseek(src_file, 0, SEEK_END);
	int size = ftell(src_file);
	src_binary = (u8*)malloc(size);
	fseek(src_file, 0, SEEK_SET);
	fread(src_binary, 1, size, src_file);
	fclose(src_file);

	// Scan the file in 32-bit increments.
	// The DLDI *must* be 4-byte aligned, or it isn't actually usable.
	for (int i = 0; i < size; i += 4)
	{
		if (((u32*)src_binary)[i>>2] == DLDI_MAGIC_NUMBER)
		{
			u8 *src_binary_dldi_area = src_binary + i;
			int dldi_size = (1 << src_binary_dldi_area[0xD]);

			// If this is a raw DLDI binary, then the file itself may be smaller
			// than the reported DLDI size.
			if (dldi_size > size)
				dldi_size = size;
			// However, if we are looking inside a homebrew app, then we may
			// accidentally go out of bounds of the file.
			else if ((src_binary_dldi_area + dldi_size) > (src_binary + size))
				dldi_size = (src_binary + size) - (src_binary_dldi_area);

			// We do want to allocate the whole area here, though.
			src_dldi = (DLDI_INTERFACE *)malloc(1 << src_binary_dldi_area[0xD]);
			memset(src_dldi, 0, (1 << src_binary_dldi_area[0xD]));

			// Finally, copy it to our pointer.
			memcpy(src_dldi, src_binary_dldi_area, dldi_size);
			break;
		}
	}

	if (src_dldi == NULL)
	{
		printf("Input file does not have a DLDI section.\n");
		if (src_binary != NULL)
			free(src_binary);
		return NULL;
	}

	free(src_binary);
	return src_dldi;
}

int dldiPrint(const char* src_path)
{
	DLDI_INTERFACE* src_dldi = dldiLoadFromFile(src_path);

	if (src_dldi == NULL)
	{
		printf("Failed to load input DLDI.\n");
		return -EINVAL;
	}

	char dldi_ioType[5] = {};
	memcpy(dldi_ioType, &(src_dldi->ioInterface.ioType), 4);
	printf(
		"ID=%s, Fix=0x%02x, Features=0x%08x\nName=%s\n\n",
		dldi_ioType,
		src_dldi->fixSectionsFlags,
		src_dldi->ioInterface.features,
		src_dldi->friendlyName
	);

	printf(
		"data = 0x%04x(%d)-0x%04x(%d)\n", 
		(u32)(src_dldi->dldiStart) - (u32)src_dldi->dldiStart,
		(u32)(src_dldi->dldiStart) - (u32)src_dldi->dldiStart,
		(u32)(src_dldi->dldiEnd) - (u32)src_dldi->dldiStart,
		(u32)(src_dldi->dldiEnd) - (u32)src_dldi->dldiStart
	);
	if (src_dldi->fixSectionsFlags & FIX_GLUE)
	{
		printf("glue = 0x%04x(%d)-0x%04x(%d)\n",
			(u32)(src_dldi->interworkStart) - (u32)src_dldi->dldiStart,
			(u32)(src_dldi->interworkStart) - (u32)src_dldi->dldiStart,
			(u32)(src_dldi->interworkEnd) - (u32)src_dldi->dldiStart,
			(u32)(src_dldi->interworkEnd) - (u32)src_dldi->dldiStart
		);
	}
	else
	{
		printf("glue = unset\n");
	}
	if (src_dldi->fixSectionsFlags & FIX_GOT)
	{
		printf(
			"got  = 0x%04x(%d)-0x%04x(%d)\n",
			(u32)(src_dldi->gotStart) - (u32)src_dldi->dldiStart,
			(u32)(src_dldi->gotStart) - (u32)src_dldi->dldiStart,
			(u32)(src_dldi->gotEnd) - (u32)src_dldi->dldiStart,
			(u32)(src_dldi->gotEnd) - (u32)src_dldi->dldiStart
		);
	}
	else
	{
		printf("got  = unset\n");
	}
	if (src_dldi->fixSectionsFlags & FIX_BSS)
	{
		printf(
			"bss  = 0x%04x(%d)-0x%04x(%d)\n",
			(u32)(src_dldi->bssStart) - (u32)src_dldi->dldiStart,
			(u32)(src_dldi->bssStart) - (u32)src_dldi->dldiStart,
			(u32)(src_dldi->bssEnd) - (u32)src_dldi->dldiStart,
			(u32)(src_dldi->bssEnd) - (u32)src_dldi->dldiStart
		);
	}
	else
	{
		printf("bss  = unset\n");
	}
	printf("\n");

	printf(
		"dldiStartup  = 0x%04x(%d)\n",
		(u32)src_dldi->ioInterface.startup - (u32)src_dldi->dldiStart,
		(u32)src_dldi->ioInterface.startup - (u32)src_dldi->dldiStart
	);
	printf(
		"isInserted   = 0x%04x(%d)\n",
		(u32)src_dldi->ioInterface.isInserted - (u32)src_dldi->dldiStart,
		(u32)src_dldi->ioInterface.isInserted - (u32)src_dldi->dldiStart
	);
	printf(
		"readSectors  = 0x%04x(%d)\n",
		(u32)src_dldi->ioInterface.readSectors - (u32)src_dldi->dldiStart,
		(u32)src_dldi->ioInterface.readSectors - (u32)src_dldi->dldiStart
	);
	printf(
		"writeSectors = 0x%04x(%d)\n",
		(u32)src_dldi->ioInterface.writeSectors - (u32)src_dldi->dldiStart,
		(u32)src_dldi->ioInterface.writeSectors - (u32)src_dldi->dldiStart
	);
	printf(
		"clearStatus  = 0x%04x(%d)\n",
		(u32)src_dldi->ioInterface.clearStatus - (u32)src_dldi->dldiStart,
		(u32)src_dldi->ioInterface.clearStatus - (u32)src_dldi->dldiStart
	);
	printf(
		"shutdown     = 0x%04x(%d)\n",
		(u32)src_dldi->ioInterface.shutdown - (u32)src_dldi->dldiStart,
		(u32)src_dldi->ioInterface.shutdown - (u32)src_dldi->dldiStart
	);

	free(src_dldi);
	return 0;
}

int dldiExtract(const char* src_path, const char* dst_path)
{
	int rc = dldiPrint(src_path);
	if (rc != 0)
		return rc;

	printf("\n");

	DLDI_INTERFACE* src_dldi = dldiLoadFromFile(src_path);
	u32 dldi_size = src_dldi->dldiEnd - src_dldi->dldiStart;
	FILE *dst_file = fopen(dst_path, "wb");
	if (dst_file == NULL)
	{
		printf("Failed to open output DLDI for writing: %s\n", strerror(errno));
		return errno;
	}
	fwrite(src_dldi, 1, dldi_size, dst_file);
	fflush(dst_file);
	fclose(dst_file);
	free(src_dldi);

	return 0;
}

int dldiPatch(const char* src_path, const char* dst_path)
{
	printf("Old DLDI:\n\n");
	int rc = dldiPrint(dst_path);
	if (rc != 0)
		goto patch_end;
	printf("\n");
	printf("New DLDI:\n\n");
	rc = dldiPrint(src_path);
	if (rc != 0)
		goto patch_end;

	printf("\n");

	DLDI_INTERFACE* src_dldi = dldiLoadFromFile(src_path);
	DLDI_INTERFACE* dst_dldi = dldiLoadFromFile(dst_path);
	if (dst_dldi == NULL || src_dldi == NULL)
	{
		rc = -ENOMEM;
		goto patch_end;
	}

	if (src_dldi->driverSize > dst_dldi->allocatedSize)
	{
		printf("Not enough space to patch. Input driver size: %d bytes, allocated size %d bytes\n", 1 << src_dldi->driverSize, 1 << dst_dldi->allocatedSize);
		rc = -EINVAL;
		goto patch_free;
	}

	dldiRelocate(src_dldi, dst_dldi->dldiStart);
	// restore the original allocated driver size.
	src_dldi->allocatedSize = dst_dldi->allocatedSize;

	FILE* dst_file = fopen(dst_path, "r+b");
	fseek(dst_file, 0, SEEK_END);
	int dst_size = ftell(dst_file);
	fseek(dst_file, 0, SEEK_SET);

	// Scan the file in 32-bit increments.
	// The DLDI *must* be 4-byte aligned, or it isn't actually usable.
	for (int i = 0; i < dst_size; i += 4)
	{
		u32 dldiMagic = 0;
		fseek(dst_file, i, SEEK_SET);
		fread(&dldiMagic, 1, 4, dst_file);
		if (dldiMagic == DLDI_MAGIC_NUMBER)
		{
			fseek(dst_file, i, SEEK_SET);
			fwrite(src_dldi, 1, 1 << src_dldi->driverSize, dst_file);
			fflush(dst_file);
			fclose(dst_file);
			rc = 0;
			break;
		}
	}

	printf("\n");
	printf("Patch successful\n");

patch_free:
	if (src_dldi != NULL)
		free(src_dldi);
	if (dst_dldi != NULL)
		free(dst_dldi);

patch_end:
	return rc;
}

void print_help(void)
{
	printf("dldipatch\n\n");
	printf("Patching a homebrew using a DLDI or another homebrew's embedded DLDI:\n");
	printf("dldipatch patch dldi/homebrew [homebrew...]\n\n");
	printf("Extracting a DLDI from a homebrew's embedded DLDI:\n");
	printf("dldipatch extract homebrew dldi.dldi\n\n");
	printf("Extracting a DLDI from a homebrew's embedded DLDI:\n");
	printf("dldipatch info dldi/homebrew \n\n");
}

int main(const int argc, const char **argv)
{
	if (argc < 2 || (argc < 4 && strncmp(argv[1], "info", 4) != 0))
	{
		print_help();
		return -EINVAL;
	}

	if (access(argv[2], F_OK) != 0)
	{
		printf("Input file does not exist.\n");
		return -ENOENT;
	}

	// show DLDI info
	if (strncmp(argv[1], "info", 4) == 0)
	{
		return dldiPrint(argv[2]);
	}

	// extract DLDI
	else if (strncmp(argv[1], "extract", 7) == 0)
	{
		return dldiExtract(argv[2], argv[3]);
	}

	// patch DLDI
	else if (strncmp(argv[1], "patch", 5) == 0)
	{
		return dldiPatch(argv[2], argv[3]);
	}
	// what are you even trying to do
	else
	{
		printf("Invalid argument: %s\n", argv[1]);
		return -EINVAL;
	}

	return 0;
}
