#ifndef vmflib_h
#define vmflib_h
#include <stdlib.h>
#include <stdio.h>

struct KVNode_t;
struct KVRef_t;
union KVNodeValue_t;
union KVRefValue_t;
typedef struct KVNode_t KVNode;
typedef struct KVRef_t KVRef;
typedef union KVNodeValue_t KVNodeValue;
typedef union KVRefValue_t KVRefValue;

union KVNodeValue_t{
	unsigned char* str;
	KVNode **children;
};

union KVRefValue_t{
	unsigned char* str;
	KVNode *asKVNode;
};
//KVNodes are for the KV Tree
struct KVNode_t {
	int flags;
	unsigned int numchildren;
	unsigned char* key;
	KVNodeValue val;
};
//KVRefs are for the KVStack
struct KVRef_t {
	unsigned char* key;
	KVRefValue val;
	int flags;
};

//KV flags
#define KV_ARRAY 1 //Is this KVNode an array?

KVNode* readKV(unsigned char* buf, unsigned int size);
void freeKV(KVNode* kv);
void printKV(KVNode* kv);
void printKVInternal(KVNode* kv, unsigned int numtabs);

#endif