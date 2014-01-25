#include "vmflib.h"

/*
 * This section is just random char* functions; they're designed to make traversing all this stuff decently fast and easy.
 * There are probably significant speed improvements to be found in making these better.
 */

//Skip whitespace
unsigned char nextnonwhitespace(unsigned char* buf, unsigned int* index, unsigned int size) {
	while(*index < size && (buf[*index]==' '||buf[*index]=='\n'||buf[*index]=='\t'||buf[*index]=='\r'||buf[*index]=='\0')) {
		(*index)++;
	}
	return buf[(*index)];
}
//Skip the rest of the current line
void skipline(unsigned char* buf, unsigned int* index, unsigned int size) {
	while(*index < size && buf[*index]!='\n' && buf[*index]!='\r') {
		(*index)++;
	}
	(*index)++;
}

void toNextValid(unsigned char* buf, unsigned int* index, unsigned int size) {
	nextnonwhitespace(buf,index,size);
	while(*index < size && buf[*index]=='/' && buf[(*index) + 1]=='/') { //comment
		printf("Found comment at %i\n",*index);
		skipline(buf,index,size);
		nextnonwhitespace(buf,index,size);
	}
}

//grab a pointer to the word currently at buf[*index]
//this usually returns buf+*index or buf+*index+1
//Also puts a null character at the end of the string
unsigned char* getCurrentString(unsigned char* buf, unsigned int* index) {
	unsigned int startindex = *index;
	unsigned int locallength=0;
	if(buf[*index]=='"') {
		(*index)++;
		while(true) {
			unsigned char here = buf[locallength+*index];
			if(here=='"') {
				break;
			}
			if(here=='\\') {
				locallength+=1;
			}
			locallength++;
		}
		buf[locallength+*index]='\0';
		(*index)+=locallength;
		return buf+startindex+1;
	} else {
		while(true) {
			unsigned char here = buf[locallength+*index];
			if(here==' ' || here=='\n' || here=='\t' || here=='\r') {
				break;
			}
			if(here=='\\') {
				locallength+=1;
			}
			locallength++;
		}
		buf[locallength+*index]='\0';
		(*index)+=locallength;
		return buf+startindex;
	}
}

//go back and put all the things inside the array into KVNodes, and make a KVNode for the array
unsigned int aggregateKVStack(KVRef* kvstack, unsigned int kvsp) {
	unsigned int kvbp = kvsp;
	while((kvstack[kvbp].flags & KV_ARRAY) == 0 || kvstack[kvbp].val.asKVNode != NULL) {
		kvbp--;
	}
	//kvbp now points to the last array that hasn't been converted
	unsigned int numchildren = kvsp - kvbp;
	KVNode* newnode			= (KVNode*)malloc(sizeof(KVNode));
	newnode->flags			= kvstack[kvbp].flags;
	newnode->key			= kvstack[kvbp].key;
	newnode->val.children	= (KVNode**)malloc(numchildren * sizeof(KVNode*));
	newnode->numchildren	= numchildren;
	for(unsigned int i=0;i<numchildren;i++) {
		if((kvstack[kvbp+i+1].flags & KV_ARRAY) == 0) {
			newnode->val.children[i]				= (KVNode*)malloc(sizeof(KVNode));
			newnode->val.children[i]->flags			= kvstack[kvbp+i+1].flags;
			newnode->val.children[i]->key			= kvstack[kvbp+i+1].key;
			newnode->val.children[i]->numchildren	= 0;
			newnode->val.children[i]->val.str		= kvstack[kvbp+i+1].val.str;
		} else {
			newnode->val.children[i]				= kvstack[kvbp+i+1].val.asKVNode;
		}
	}
	kvstack[kvbp].val.asKVNode = newnode;
	return kvbp;
}

// Given a KVfile read into a buffer with a specified size, return a KVNode that is the root of a tree loaded from that buffer.
// This is generally a bunch of parsing code built around pushing stuff it reads onto a stack.
// Is pretty fast, but could be faster.
KVNode* readKV(unsigned char* buf, unsigned int size) {
	unsigned int index = 0;
	unsigned int numkeys = 0;

	unsigned const int numrefs = 30000;
	KVRef kvstack[numrefs];
	unsigned int kvsp = 0;
	kvstack[0].flags		= KV_ARRAY;
	kvstack[0].key			= (unsigned char*)"root";
	kvstack[0].val.asKVNode	= NULL;

	toNextValid(buf,&index,size);
	while(index < size) {
		if(buf[index]=='}') {
			//Initialize the KVNodes and all that, and shove it all into an array
			kvsp = aggregateKVStack(kvstack,kvsp);

			//skip the brace and go on to the next key
			index++;
			toNextValid(buf,&index,size);
			continue;
		}
		unsigned char* key = getCurrentString(buf,&index);
		toNextValid(buf,&index,size);
		if(buf[index]=='{') {
			//printf("KeyValue is %s : Array\n",key);
			kvsp++;
			kvstack[kvsp].key			= key;
			kvstack[kvsp].val.asKVNode	= NULL;
			kvstack[kvsp].flags			= KV_ARRAY;
			index++;
			numkeys++;
		} else {
			unsigned char* value = getCurrentString(buf,&index);
			kvsp++;
			kvstack[kvsp].key		= key;
			kvstack[kvsp].val.str	= value;
			kvstack[kvsp].flags		= 0;
			//printf("KeyValue is %s : %s\n",key,value);
		}
		toNextValid(buf,&index,size);
	}
	printf("Number of total keys: %u\n",numkeys);
	kvsp = aggregateKVStack(kvstack,kvsp);
	if(kvsp != 0) {
		printf("Error Reading KVFile: Imbalanced braces!");
		exit(1);
	}
	//printKV(kvstack[kvsp].val.asKVNode);
	return kvstack[kvsp].val.asKVNode;
}

// Simpler call for a more elegant codebase.
void printKV(KVNode* kv) {
	printKVInternal(kv, 0);
}

// Generic tree-traversing print code
// Could be optimized somewhat by using a tab lookup table
// But seriously, a tab lookup table?
// That's just silly.
void printKVInternal(KVNode* kv, unsigned int numtabs) {
	if((kv->flags & KV_ARRAY) == 0) {
		for(unsigned int i=0;i<numtabs;i++)
			printf("\t");
		printf("\"%s\" \"%s\"\n",kv->key, kv->val.str);
	} else {
		for(unsigned int i=0;i<numtabs;i++)
			printf("\t");
		printf("\"%s\" {\n",kv->key, kv->val.str);
		for(unsigned int i=0;i<kv->numchildren;i++) {
			printKVInternal(kv->val.children[i],numtabs+1);
		}
		for(unsigned int i=0;i<numtabs;i++)
			printf("\t");
		printf("}\n");
	}
}

// Cleaning up properly is good form, so let's do it.
void freeKV(KVNode* kv) {
	if((kv->flags & KV_ARRAY) == 0) {
		free(kv);
	} else {
		for(unsigned int i=0;i<kv->numchildren;i++) {
			freeKV(kv->val.children[i]);
		}
		free(kv->val.children);
		free(kv);
	}
}