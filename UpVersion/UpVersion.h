#ifndef upversion_h
#define upversion_h
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<limits.h>
#include<float.h>
#include<math.h>
#include"vmflib.h"
#ifndef WIN32
#include<stdbool.h>
#endif


#define HEADER_LUMPS 64
#define MAX_DISP_CORNER_NEIGHBORS 4
#define OVERLAY_BSP_FACE_COUNT 64

typedef struct lump_t {
	int fileofs;
	unsigned int filelen;
	int version;
	char fourCC[4];
} lump;



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
	int m_nReferenceID; //One of the 2 new values, this one is safe to set at 0, but it lets you do cool things otherwise.
						//Basically, it's used to let dynamic effects happen on displacements.
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

#define OVERLAY_FLAG_HIDE_IN_LOW_VIOLENCE 1

typedef struct dbrushside_21_t {
	unsigned int planenum;
	short texinfo;
	short dispinfo;
	bool bevel;
	bool thin;
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

typedef struct c4_t {
	char a;
	char b;
	char c;
	char d;
} c4;

typedef union i2c_t {
	int num;
	c4 c;
} i2c;

#endif