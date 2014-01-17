#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#ifndef WIN32
#include<stdbool.h>
#endif

#define HEADER_LUMPS 64
#define MAX_DISP_CORNER_NEIGHBORS 4
#define OVERLAY_BSP_FACE_COUNT 64

typedef struct lump_t {
	int fileofs;
	int filelen;
	int version;
	char fourCC[4];
} lump;

typedef struct vector_t {
	float x;
	float y;
	float z;
} vector;

typedef struct CDispSubNeighbor_t {
	unsigned short Neighbor;
	unsigned char NeighborOrientation;
	unsigned char Span;
	unsigned char NeighborSpan;
} CDispSubNeighbor;

typedef struct CDispNeighbor_t {
	CDispSubNeighbor SubNeighbors[2];
} CDispNeighbor;

typedef struct CDispCornerNeighbors_t {
	unsigned short Neighbors[MAX_DISP_CORNER_NEIGHBORS];
	unsigned char nNeighbors;
} CDispCornerNeighbors;

typedef struct ddispinfo_21_t {
	vector startPos;
	int DispVertStart;
	int DispTriStart;
	int power;
	int minTess;
	float smoothingAngle;
	int contents;
	unsigned short MapFace;
	int LightmapAlphaStart;
	int LightmapSamplePositionStart;
	CDispNeighbor EdgeNeighbors[4];
	CDispCornerNeighbors CornerNeighbors[4];
	unsigned int AllowedVerts[10];
} ddispinfo_21;

typedef struct ddispinfo_23_t {
	vector startPos;
	int DispVertStart;
	int DispTriStart;
	int power;
	int minTess;
	float smoothingAngle;
	int contents;
	int m_nReferenceID; //One of the 2 new values, this one is apparently always 0
	unsigned short MapFace;
	int m_nAltTexInfo; //The other new value. If there's no alternate texinfo, it's -1
	int LightmapAlphaStart;
	int LightmapSamplePositionStart;
	CDispNeighbor EdgeNeighbors[4];
	CDispCornerNeighbors CornerNeighbors[4];
	unsigned int AllowedVerts[10];
} ddispinfo_23;

typedef struct doverlay_21_t {
	int Id;
	short TexInfo;
	unsigned short FaceCountAndRenderOrder;
	int Ofaces [OVERLAY_BSP_FACE_COUNT];
	float U[2];
	float V[2];
	vector UVPoints[4];
	vector Origin;
	vector BasisNormal;
} doverlay_21;

typedef struct doverlay_23_t {
	int Id;
	short TexInfo;
	unsigned short FaceCountAndRenderOrder;
	int Ofaces [OVERLAY_BSP_FACE_COUNT];
	float U[2];
	float V[2];
	vector UVPoints[4];
	vector Origin;
	vector BasisNormal;
	unsigned int flags;
} doverlay_23;

#define OVERLAY_FLAG_NOT_IN_LOW_VIOLENCE 1

typedef struct dbrushside_21_t {
	unsigned int planenum;
	short texinfo;
	short dispinfo;
	short bevel;
} dbrushside_21;

typedef struct dbrushside_23_t {
	unsigned int planenum;
	short texinfo;
	short dispinfo;
	bool bevel;
	bool thin;
} dbrushside_23;

typedef struct BSPheader_t
{
	int ident;
	int version;
	lump lumps[HEADER_LUMPS];
	int mapRevision;
} BSPheader;

typedef struct dgamelump_t {
	int id;
	unsigned short flags;
	unsigned short version;
	int fileofs;
	int filelen;
} dgamelump;

typedef struct dgamelumpheader_t {
	unsigned int lumpCount;
	dgamelump* lumps;
} dgamelumpheader;

int main(int argc, char* argv[]) {
	printf("UpVersion 1.0, by Penguinwizzard\n");
	if(argc <2) {
		printf("usage: UpVersion <bspfile>\n");
		return 0;
	}
	FILE* bsp;
	fopen_s(&bsp,argv[1],"rb");
	if(bsp == NULL) {
		printf("File not found or could not be read.\n");
		exit(1);
	}
	fseek(bsp, 0, SEEK_END);
	long fsize = ftell(bsp);
	fseek(bsp, 0, SEEK_SET);

	unsigned char *buf = (unsigned char*)malloc(fsize);
	fread(buf, fsize, 1, bsp);
	fclose(bsp);
	BSPheader* header = (BSPheader*)buf;
	printf("File loaded, general file info:\n");
	printf("Sanity Check: %i ",header->ident);
	if(header->ident==1347633750) {
		printf("(passed)\n");
	} else {
		printf("(failed)\n");
	}
	printf("BSP File Version: %i\n",header->version);
	printf("Map Revision: %i\n",header->mapRevision);
	if(header->version==21) {
		unsigned int newsize = fsize;
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
					newer->bevel = (old->bevel == 1);
					newer->thin = false;
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
		fwrite(buf2, newsize, 1, target);
		fclose(target);
		printf("Map version changed to 23.\n");
	} else {
		printf("No changes made.\n");
	}
	return 0;
}
