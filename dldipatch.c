/*
	dldipatch aka dlditool public domain
	SPDX-License-Identifier: CC0-1.0

	According to ndsdis2 -NH9 0x00 dldi_startup_patch.o (from NDS_loader build result):
	:00000040 E3A00001 mov  r0,#0x1 ;r0=1(0x1)
	:00000044 E12FFF1E bx r14 (Jump to addr_00000000?)
	So the corresponding memory value is "\x01\x00\xa0\xe3\x1e\xff\x2f\xe1" (8 bytes).

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/stat.h>

typedef uint32_t u32;
typedef unsigned char byte;

#define magicString	0x00
#define dldiVersion	0x0c
#define driverSize	0x0d
#define fixSections	0x0e
#define allocatedSpace	0x0f
#define friendlyName 0x10

#define dataStart	0x40
#define dataEnd	0x44
#define glueStart	0x48
#define glueEnd	0x4c
#define gotStart	0x50
#define gotEnd	0x54
#define bssStart	0x58
#define bssEnd	0x5c

#define ioType	0x60
#define dldiFeatures	0x64
#define dldiStartup	0x68
#define isInserted	0x6c
#define readSectors	0x70
#define writeSectors	0x74
#define clearStatus	0x78
#define shutdown	0x7c
#define dldiData	0x80

#define fixAll	0x01
#define fixGlue	0x02
#define fixGot	0x04
#define fixBss	0x08

const byte *dldimagic=(byte*)"\xed\xa5\x8d\xbf Chishm";

#define torelative(n) (read32(pA+n)-pAdata)

unsigned int read32(const void *p){
	const unsigned char *x=(const unsigned char*)p;
	return x[0]|(x[1]<<8)|(x[2]<<16)|(x[3]<<24);
}

void write32(void *p, const unsigned int n){
	unsigned char *x = (unsigned char*)p;
	x[0] = n & 0xff, 
	x[1] = (n >> 8) & 0xff, 
	x[2] = (n >> 16) & 0xff, 
	x[3] = (n >> 24) & 0xff;
}

int dldipatch(byte *nds, const int ndslen, const int ignoresize, const byte *pD, const int dldilen) {
	byte *pA=NULL;
	byte id[5];
	byte space;
	u32 reloc, pAdata, pDdata, pDbssEnd, fix;
	int i, ittr;

	for(i = 0; i < ndslen - 0x80; i += 4) {
		if(!memcmp(nds + i, dldimagic, 12) && (read32(nds + i + dldiVersion) & 0xe0f0e0ff) == 1){
			pA = nds + i;
			printf("Section 0x%08x: ", i);
			if(*((u32*)(pD + bssEnd)) - *((u32*)(pD + dataStart)) > 1 << pA[allocatedSpace]){
				printf("Available %dB, need %dB. ", 1 << pA[allocatedSpace], *((u32*)(pD + bssEnd)) - *((u32*)(pD + dataStart)));
				if(ignoresize) {
					printf("searching interrupted.\n");
					break;
				}
				printf("continue searching.\n");
				pA = NULL;
				continue;
			}
			printf("searching done.\n");
			break;
		}
	}
	if(!pA) {
		printf("not found valid dldi section\n");
		return 1;
	}

	space = pA[allocatedSpace];

	pAdata = read32(pA + dataStart);
	if(!pAdata)
		pAdata = read32(pA + dldiStartup) - dldiData;
	memcpy(id, pA + ioType, 4);
	id[4] = 0;
	printf("Old ID=%s, Interface=0x%08x,\nName=%s\n", id, pAdata, pA + friendlyName);
	memcpy(id, pD + ioType, 4);
	id[4] = 0;
	printf("New ID=%s, Interface=0x%08x,\nName=%s\n", id, pDdata = read32(pD + dataStart), pD + friendlyName);
	printf("Relocation=0x%08x, Fix=0x%02x\n", reloc = pAdata - pDdata, fix = pD[fixSections]); //pAdata=pDdata+reloc
	printf("dldiFileSize=0x%04x, dldiMemSize=0x%04x\n", dldilen, *((u32*)(pD + bssEnd)) - *((u32*)(pD + dataStart)));

	memcpy(pA, pD, dldilen);
	pA[allocatedSpace] = space;
	for(ittr = dataStart; ittr < ioType; ittr += 4)
		write32(pA + ittr, read32(pA + ittr) + reloc);
	for(ittr = dldiStartup; ittr < dldiData; ittr += 4)
		write32(pA + ittr, read32(pA + ittr) + reloc);
	pAdata = read32(pA + dataStart);
	pDbssEnd = read32(pD + bssEnd);

	if(fix & fixAll)
		for(ittr = torelative(dataStart); ittr < torelative(dataEnd); ittr += 4)
			if(pDdata <= read32(pA + ittr) && read32(pA + ittr) < pDbssEnd)
				printf("All  0x%04x: 0x%08x -> 0x%08x\n", ittr, read32(pA + ittr), read32(pA + ittr) + reloc), write32(pA + ittr, read32(pA + ittr) + reloc);
	if(fix & fixGlue)
		for(ittr = torelative(glueStart); ittr < torelative(glueEnd); ittr += 4)
			if(pDdata <= read32(pA + ittr) && read32(pA + ittr) < pDbssEnd)
				printf("Glue 0x%04x: 0x%08x -> 0x%08x\n", ittr, read32(pA + ittr), read32(pA + ittr) + reloc), write32(pA + ittr, read32(pA + ittr) + reloc);
	if(fix & fixGot)
		for(ittr = torelative(gotStart); ittr < torelative(gotEnd); ittr += 4)
			if(pDdata <= read32(pA + ittr) && read32(pA + ittr) < pDbssEnd)
				printf("Got  0x%04x: 0x%08x -> 0x%08x\n", ittr, read32(pA + ittr), read32(pA + ittr) + reloc), write32(pA + ittr, read32(pA + ittr) + reloc);
	if(fix & fixBss)
		memset(pA + torelative(bssStart), 0, pDbssEnd - read32(pD + bssStart));

	printf("Patched successfully\n");
	return 0;
}

#undef torelative
#define torelative(n) (read32(pD+n)-pDdata)

#define TWICE(e) (e),(e)
int dldishow(const byte *pD,const int dldilen, bool extract, const char* out){
	byte id[5];
	u32 pDdata, pDbssEnd, fix;
	int ittr;

	pDdata = read32(pD + dataStart);
	pDbssEnd = read32(pD + bssEnd);
	memcpy(id, pD + ioType, 4);
	id[4] = 0;
	printf("ID=%s, Fix=0x%02x, Features=0x%02x\nName=%s\n\n", id, fix = pD[fixSections], *((u32*)(pD + dldiFeatures)), pD + friendlyName);
	printf("data = 0x%04x(%d)-0x%04x(%d)\n", 
		TWICE(*((u32*)(pD + dataStart)) - *((u32*)(pD + dataStart))), TWICE(*((u32*)(pD + dataEnd)) - *((u32*)(pD + dataStart))));
	printf("glue = 0x%04x(%d)-0x%04x(%d)\n",
		TWICE(*((u32*)(pD + glueStart)) - *((u32*)(pD + dataStart))), TWICE(*((u32*)(pD + glueEnd)) - *((u32*)(pD + dataStart))));
	printf("got  = 0x%04x(%d)-0x%04x(%d)\n",
		TWICE(*((u32*)(pD + gotStart)) - *((u32*)(pD + dataStart))), TWICE(*((u32*)(pD + gotEnd)) - *((u32*)(pD + dataStart))));
	printf("bss  = 0x%04x(%d)-0x%04x(%d)\n",
		TWICE(*((u32*)(pD + bssStart)) - *((u32*)(pD + dataStart))), TWICE(*((u32*)(pD + bssEnd)) - *((u32*)(pD + dataStart))));
	printf("\n");
	printf("dldiStartup  = 0x%04x(%d)\n", TWICE(*((u32*)(pD + dldiStartup)) - *((u32*)(pD + dataStart))));
	printf("isInserted   = 0x%04x(%d)\n", TWICE(*((u32*)(pD + isInserted)) - *((u32*)(pD + dataStart))));
	printf("readSectors  = 0x%04x(%d)\n", TWICE(*((u32*)(pD + readSectors)) - *((u32*)(pD + dataStart))));
	printf("writeSectors = 0x%04x(%d)\n", TWICE(*((u32*)(pD + writeSectors)) - *((u32*)(pD + dataStart))));
	printf("clearStatus  = 0x%04x(%d)\n", TWICE(*((u32*)(pD + clearStatus)) - *((u32*)(pD + dataStart))));
	printf("shutdown     = 0x%04x(%d)\n", TWICE(*((u32*)(pD + shutdown)) - *((u32*)(pD + dataStart))));

	if(fix & fixAll)
		for(ittr = torelative(dataStart); ittr < torelative(dataEnd); ittr += 4)
			if(pDdata <= read32(pD + ittr) && read32(pD + ittr) < pDbssEnd)
				printf("All  0x%04x: 0x%08x\n", ittr, torelative(ittr));
	if(fix & fixGlue)
		for(ittr = torelative(glueStart); ittr < torelative(glueEnd); ittr += 4)
			if(pDdata <= read32(pD + ittr) && read32(pD + ittr) < pDbssEnd)
				printf("Glue 0x%04x: 0x%08x\n", ittr, torelative(ittr));
	if(fix & fixGot)
		for(ittr = torelative(gotStart); ittr < torelative(gotEnd); ittr += 4)
			if(pDdata <= read32(pD + ittr) && read32(pD + ittr) < pDbssEnd)
				printf("Got  0x%04x: 0x%08x\n", ittr, torelative(ittr));

	if(extract) {
		FILE* of = fopen(out, "wb");
		fseek(of, 0, SEEK_SET);
		fwrite(pD,1,*((u32*)(pD+bssStart))-*((u32*)(pD+dataStart)),of);
		fclose(of);
	}
	return 0;
}

void print_help(void) {
	printf("dldipatch aka dlditool public domain v8\n\n");
	printf("Patching a homebrew using a DLDI or another homebrew's embedded DLDI:\n");
	printf("dldipatch patch dldi/homebrew [homebrew...]\n\n");
	printf("Extracting a DLDI from a homebrew's embedded DLDI:\n");
	printf("dldipatch extract homebrew dldi.dldi\n\n");
	printf("Extracting a DLDI from a homebrew's embedded DLDI:\n");
	printf("dldipatch info dldi/homebrew \n\n");
}

int main(const int argc, const char **argv) {
	int i, dldisize;
	FILE *f, *fdldi;
	struct stat st, stdldi;
	byte *p, *pdldi, *pd = NULL;

	if(argc < 4 && strncmp(argv[1], "info", 4) != 0) {
		print_help();
		return 1;
	}
	if(!(fdldi = fopen(argv[2], "rb"))) {
		printf("cannot open %s\n", argv[2]);
		return 2;
	}
	fstat(fileno(fdldi), &stdldi);
	dldisize = stdldi.st_size;
	if(!(pdldi = malloc(dldisize))) {
		fclose(fdldi);
		printf("cannot allocate %d bytes for dldi\n", (int)dldisize);
		return 3;
	}
	fread(pdldi, 1, dldisize, fdldi);
	fclose(fdldi);
	for(i = 0; i < dldisize - 0x80; i += 4) {
		if(!memcmp(pdldi + i, dldimagic, 12) && (read32(pdldi + i + dldiVersion) & 0xe0f0e0ff) == 1) {
			pd = pdldi + i;
			dldisize -= i;
			break;
		}
	}
	if(!pd) {
		printf("the dldi file is invalid\n");
		return 4;
	}
	u32 dldilen = *((u32*)(pd + bssStart)) - *((u32*)(pd + dataStart));

	// show DLDI info
	if(strncmp(argv[1], "info", 4) == 0) {
		dldishow(pd, dldilen, false, NULL);
	}

	// extract DLDI
	else if(strncmp(argv[1], "extract", 7) == 0) {
		dldishow(pd, dldilen, true, argv[3]);
	}

	// patch DLDI
	else if(strncmp(argv[1], "patch", 5) == 0) {
		for(i = 3; i < argc; i++) {
			int ignoresize = 0;
			const char *name = argv[i];
			if(*name==':') ignoresize = 1, name++;
			printf("Patching %s...\n", name);
			if(!(f = fopen(name, "rb+"))) {
				printf("cannot open %s\n", name);
				continue;
			}
			fstat(fileno(f), &st);
			if(!(p = malloc(st.st_size))) {
				printf("cannot allocate %d bytes for %s\n", (int)st.st_size, name);
				continue;
			}
			fread(p, 1, st.st_size, f);
			rewind(f);
			if(!dldipatch(p, st.st_size, ignoresize, pd, dldilen))
				fwrite(p, 1, st.st_size, f);
			fclose(f);
			free(p);
		}
	}
	// what are you even trying to do
	else {
		printf("Invalid argument: %s\n", argv[1]);
	}
	free(pdldi);
	return 0;
}
