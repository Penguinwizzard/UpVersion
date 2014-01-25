#include"UpVersion.h"

int main(int argc, char* argv[]) {
	printf("UpVersion 1.1, by Penguinwizzard\n");
	if(argc <2) {
		printf("usage: UpVersion <bspfile> [<vmffile>]\n");
		return 0;
	}

	//Load BSP File
	//Boilerplate
	FILE* bsp;
	fopen_s(&bsp,argv[1],"rb");
	if(bsp == NULL) {
		printf("BSP file not found or could not be read.\n");
		exit(1);
	}
	fseek(bsp, 0, SEEK_END);
	unsigned long fsize = (unsigned long)ftell(bsp);
	fseek(bsp, 0, SEEK_SET);

	unsigned char *buf = (unsigned char*)malloc((size_t)fsize);
	fread(buf, fsize, 1, bsp);
	fclose(bsp);

	BSPheader* header = (BSPheader*)buf;
	printf("BSP File loaded, general file info:\n");
	printf("Sanity Check: %i ",header->ident);
	if(header->ident==1347633750) {
		printf("(passed)\n");
	} else {
		printf("(failed)\n");
	}
	printf("BSP File Version: %i\n",header->version);
	printf("Map Revision: %i\n",header->mapRevision);

	//The KVNode for the vmf file, if we're given one.
	KVNode* vmf = NULL;
	unsigned char *vmfbuf = NULL;
	if(argc >= 3) {
		//Load VMF File
		//Boilerplate
		printf("Loading VMF File: WARNING, EXPERIMENTAL\n");
		FILE* vmffile;
		fopen_s(&vmffile,argv[2],"rb");
		if(vmffile == NULL) {
			printf("VMF file not found or could not be read.\n");
			exit(1);
		}
		fseek(vmffile, 0, SEEK_END);
		unsigned long vmfsize = (unsigned long)ftell(vmffile);
		fseek(vmffile, 0, SEEK_SET);

		vmfbuf = (unsigned char*)malloc(vmfsize);
		fread(vmfbuf, vmfsize, 1, vmffile);
		fclose(vmffile);
		printf("test\n");
		//Parse the VMF KV file so that we can use it later.
		vmf = readKV(vmfbuf,vmfsize);
	}
	if(header->version==21) {
		unsigned int newsize = fsize; //Two lumps have changed in size per entry; we have to reflect that here:
		newsize += (header->lumps[26].filelen/(unsigned int)sizeof(ddispinfo_21))*(sizeof(ddispinfo_23)-sizeof(ddispinfo_21));
		newsize += (header->lumps[45].filelen/(unsigned int)sizeof(doverlay_21))*(sizeof(doverlay_23)-sizeof(doverlay_21));
		unsigned char *buf2 = (unsigned char*)malloc(newsize);
		memset(buf2,0,newsize);
		memcpy(buf2,buf,sizeof(BSPheader));
		BSPheader* newheader = (BSPheader*) buf2;
		newheader->version = 23;
		int index = sizeof(BSPheader);
		int i;
		for(i=0;i<HEADER_LUMPS;i++) {
			if(i==26) { //dispinfo
				printf("\tUpdating lump 26...\n");
				newheader->lumps[i].fileofs = index;
				unsigned int count = header->lumps[i].filelen/176;
				newheader->lumps[i].filelen = count*184;
				newheader->lumps[i].version = header->lumps[i].version;
				ddispinfo_21 (*disps)[] = (ddispinfo_21 (*)[])(buf+header->lumps[i].fileofs);
				ddispinfo_23 (*newdisps)[] = (ddispinfo_23 (*)[])(buf2+newheader->lumps[i].fileofs);
				unsigned int j;
				for(j=0;j<count;j++) {
					ddispinfo_21* old = &((*disps)[j]);
					ddispinfo_23* newer = &((*newdisps)[j]);
					newer->startPos.x = old->startPos.x;
					newer->startPos.y = old->startPos.y;
					newer->startPos.z = old->startPos.z;
					newer->DispVertStart = old->DispVertStart;
					newer->DispTriStart = old->DispTriStart;
					newer->power = old->power;
					newer->minTess = old->minTess;
					newer->smoothingAngle = old->smoothingAngle;
					newer->contents = old->contents;
					newer->m_nReferenceID = 0; //Found through file analysis, never non-zero
					newer->MapFace = old->MapFace;
					newer->m_nAltTexInfo = -1; //This just means that we have no alternate texture info
					newer->LightmapAlphaStart = old->LightmapAlphaStart;
					newer->LightmapSamplePositionStart = old->LightmapSamplePositionStart;
					int k;
					memcpy(&(newer->EdgeNeighbors[0]),&(old->EdgeNeighbors[0]),4*(sizeof(CDispNeighbor) + sizeof(CDispCornerNeighbors)));
					for(k=0;k<10;k++) {
						newer->AllowedVerts[k]=old->AllowedVerts[k];
					}
				}
				index += newheader->lumps[i].filelen;
			} else if (i==35) { //game lump
				printf("\tUpdating lump 35...\n");
				//we have to update the game lump internal indicies to reflect the new position
				newheader->lumps[i].fileofs = index;
				newheader->lumps[i].filelen = header->lumps[i].filelen;
				newheader->lumps[i].version = header->lumps[i].version;
				memcpy(buf2+newheader->lumps[i].fileofs,buf+header->lumps[i].fileofs,newheader->lumps[i].filelen);
				dgamelumpheader* oldgamelumpheader = (dgamelumpheader*)(buf+header->lumps[i].fileofs);
				dgamelumpheader* newgamelumpheader = (dgamelumpheader*)(buf2+newheader->lumps[i].fileofs);
				unsigned int j;
				for(j=0;j<oldgamelumpheader->lumpCount;j++) {
					dgamelump* oldgamelump = (dgamelump*)((&oldgamelumpheader->lumps)+j*4);
					dgamelump* newgamelump = (dgamelump*)((&newgamelumpheader->lumps)+j*4);
					newgamelump->fileofs=newheader->lumps[i].fileofs+(oldgamelump->fileofs-header->lumps[i].fileofs);
				}
				index += newheader->lumps[i].filelen;
			} else if (i==45) { //overlay
				printf("\tUpdating lump 45...\n");
				newheader->lumps[i].fileofs = index;
				unsigned int count = header->lumps[i].filelen/352;
				newheader->lumps[i].filelen = count * sizeof(doverlay_23);
				newheader->lumps[i].version = header->lumps[i].version;
				doverlay_21 (*overlays)[] = (doverlay_21 (*)[])(buf+header->lumps[i].fileofs);
				doverlay_23 (*newoverlays)[] = (doverlay_23 (*)[])(buf2+newheader->lumps[i].fileofs);
				unsigned int j;
				for(j=0;j<count;j++) {
					doverlay_21* old = &((*overlays)[j]);
					doverlay_23* newer = &((*newoverlays)[j]);
					memcpy(newer,old,sizeof(doverlay_21));
					newer->flags = 0;   //We don't support the only known flag, LOW_VIOLENCE_DISABLED, just yet;
										//this is a feature that's coming though.
				}
				index += newheader->lumps[i].filelen;
			} else if (i==19) { //brushside
				printf("\tUpdating lump 19...\n");
				newheader->lumps[i].fileofs = index;
				unsigned int count = header->lumps[i].filelen/sizeof(dbrushside_21);
				newheader->lumps[i].filelen = header->lumps[i].filelen;
				newheader->lumps[i].version = header->lumps[i].version;
				memcpy(buf2+newheader->lumps[i].fileofs,buf+header->lumps[i].fileofs,newheader->lumps[i].filelen);
				dbrushside_21 (*brushsides)[] = (dbrushside_21 (*)[])(buf+header->lumps[i].fileofs);
				dbrushside_23 (*newbrushsides)[] = (dbrushside_23 (*)[])(buf2+newheader->lumps[i].fileofs);
				unsigned int j;
				for(j=0;j<count;j++) {
					dbrushside_21* old = &((*brushsides)[j]);
					dbrushside_23* newer = &((*newbrushsides)[j]);
					newer->bevel = old->bevel;
					newer->thin = old->thin;
				}
				index += newheader->lumps[i].filelen;
			} else { //generic lump copy
				newheader->lumps[i].fileofs = index;
				newheader->lumps[i].filelen = header->lumps[i].filelen;
				newheader->lumps[i].version = header->lumps[i].version;
				memcpy(buf2+newheader->lumps[i].fileofs,buf+header->lumps[i].fileofs,newheader->lumps[i].filelen);
				index += newheader->lumps[i].filelen;
			}
		}
		FILE* target;
		fopen_s(&target,argv[1],"wb");
		if(target == NULL) {
			printf("Error: Could not open BSP file for writing.\n");
			exit(1);
		}
		fwrite(buf2, newsize, 1, target);
		fclose(target);
		printf("Map version changed to 23.\n");
	} else {
		printf("No changes made.\n");
	}
	if(vmfbuf != NULL) {
		free(vmfbuf);
		freeKV(vmf);
	}
	return 0;
}
