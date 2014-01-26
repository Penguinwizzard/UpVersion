#include"UpVersion.h"

unsigned long load_file(unsigned char** buf, char* fname, char* opts, char* errmsg) {
	FILE* file;
	fopen_s(&file,fname,opts);
	if(file == NULL) {
		printf("%s",errmsg);
		exit(1);
	}
	fseek(file, 0, SEEK_END);
	unsigned long fsize = (unsigned long)ftell(file);
	fseek(file, 0, SEEK_SET);

	(*buf) = (unsigned char*)malloc((size_t)fsize);
	fread(*buf, fsize, 1, file);
	fclose(file);
	return fsize;
}

int main(int argc, char* argv[]) {
	printf("UpVersion 1.1, by Penguinwizzard\n");
	if(argc <2) {
		printf("usage: UpVersion <bspfile> [<vmffile>]\n");
		return 0;
	}

	//Load BSP File
	//Boilerplate
	unsigned char *buf = NULL;
	unsigned long fsize = load_file(&buf, argv[1],"rb","BSP file not found or could not be read.\n");

	BSPheader* header = (BSPheader*)buf;
	printf("BSP File loaded, general file info:\n");
	printf("Sanity Check: %i ",header->ident);
	if(header->ident==1347633750) { //VBSP, in int form because we're lazy (and this serves as an endianness check)
		printf("(passed)\n");
	} else {
		printf("(failed)\n");
		exit(1); //Not going to try to do anything to the file if it doesn't pass the sanity check, since we might do serious damage.
	}
	printf("BSP File Version: %i\n",header->version);
	printf("Map Revision: %i\n",header->mapRevision);

	//The KVNode for the vmf file, if we're given one.
	KVNode* vmf = NULL;
	unsigned char *vmfbuf = NULL;
	if(argc >= 3) {
		//Load VMF File
		printf("Loading VMF File: WARNING, EXPERIMENTAL\n");
		unsigned long vmfsize = load_file(&vmfbuf,argv[2],"rb","VMF file not found or could not be read.\n");
		printf("VMF Size: %u\n",vmfsize);
		//Parse the VMF KV file so that we can use it later.
		vmf = readKV(vmfbuf,vmfsize);
	}

	//Build list of overlays
	if(vmf != NULL) {
		unsigned int count=0;
		KVNode* overlay = NULL;
		while(overlay = nextByProperty(vmf,overlay,"classname","info_overlay")) {
			count++;
		}
		printf("%u overlays found\n",count);
		
	}
	//Build GNV
	if(vmf != NULL) {
		unsigned int count=0;
		//Check that it's a properly formed bsp
		KVNode* world = nextByKey(vmf,NULL,"world");
		if(world == NULL) {
			printf("VMF File does not have world node, exiting...");
			printKV(vmf);
			exit(1);
		}
		//We're going to do this by marking every TOOLS/TOOLSSKIP surface point as an invalid cell.
		//To do this, we're first going to do a dry run to figure out how many of these points there are.
		KVNode* solid = NULL;
		while(solid = nextByKey(world,solid,"solid")) {
			KVNode* side = NULL;
			while(side = nextByKey(solid,side,"side")) {
				if(hasProperty(side,"material","TOOLS/TOOLSSKIP")) {
					count++;
				}
			}
		}
		printf("%u toolsskip surfaces found\n",count);
		// 3 points per surface
		vector* points = (vector*)malloc(count*3*sizeof(vector));
		unsigned int pi = 0;
		//Now for the parsing pass
		float minx=FLT_MAX;
		float maxx=FLT_MIN;
		float miny=FLT_MAX;
		float maxy=FLT_MIN;
		//We're going to go through and parse all the points into a nice big array.
		solid = NULL;
		while(solid = nextByKey(world,solid,"solid")) {
			KVNode* side = NULL;
			while(side = nextByKey(solid,side,"side")) {
				if(hasProperty(side,"material","TOOLS/TOOLSSKIP")) {
					const unsigned char* val = getValue(side,"plane");
					if(val==NULL) //Really shouldn't happen
						continue;
					float a,b,c,d,e,f,g,h,i;
					sscanf((const char*)val," ( %f %f %f ) ( %f %f %f ) ( %f %f %f ) ",&a,&b,&c,&d,&e,&f,&g,&h,&i);
					if(a < minx) minx = a;
					if(a > maxx) maxx = a;
					if(b < miny) miny = b;
					if(b > maxy) maxy = b;
					points[pi  ].x = a;
					points[pi  ].y = b;
					points[pi++].z = c;
					if(d < minx) minx = d;
					if(d > maxx) maxx = d;
					if(e < miny) miny = e;
					if(e > maxy) maxy = e;
					points[pi  ].x = d;
					points[pi  ].y = e;
					points[pi++].z = f;
					if(g < minx) minx = g;
					if(g > maxx) maxx = g;
					if(h < miny) miny = h;
					if(h > maxy) maxy = h;
					points[pi  ].x = g;
					points[pi  ].y = h;
					points[pi++].z = i;
				}
			}
		}
		//Now we make the navigation image, and so on
		int cellmaxx = floorf(maxx/64);
		int cellminx = floorf(minx/64);
		int cellmaxy = floorf(maxy/64);
		int cellminy = floorf(miny/64);
		int width = cellmaxx-cellminx+1;
		int height = cellmaxy-cellminy+1;
		printf("GNV Info:\n");
		printf("%i %i %i %i %i %i\n",cellminx, cellmaxx, cellminy, cellmaxy, width, height);
		bool *pbm = (bool*)malloc(height*width*sizeof(bool));
		memset(pbm,0,height*width*sizeof(bool));
		for(unsigned int i=0;i<count*3;i++) {
			pbm[(((int)floorf(points[i].x/64))-cellminx)+(cellmaxy - (int)floorf(points[i].y/64))*width] = true;
		}
		for(int h=0;h<height;h++) {
			for(int w=0;w<width;w++) {
				printf("%c ",(pbm[w+h*width])?'1':'0');
			}
			printf("\n");
		}
		free(pbm);
		free(points);
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
				printf("%i %i\n",count,header->lumps[i].filelen);
				for(j=0;j<count;j++) {
					doverlay_21* old = &((*overlays)[j]);
					doverlay_23* newer = &((*newoverlays)[j]);
					memcpy(newer,old,sizeof(doverlay_21));
					newer->flags = 0;   //We don't support the only known flag, LOW_VIOLENCE_DISABLED, just yet;
										//this is a feature that's coming though.
					printf("%i %f %f %f\n",old->Id, newer->Origin.x, newer->Origin.y, newer->Origin.z);
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
		printf("Cleaning up...\n");
		freeKV(vmf);
		free(vmfbuf);
	}
	return 0;
}
