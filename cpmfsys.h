#include <stdint.h> 
#include <stdlib.h> 
#include "diskSimulator.h"
#include  <stdbool.h> 
#include <string.h> 
#include <stdio.h> 

#define EXTENT_SIZE 32
#define BLOCKS_PER_EXTENT 16 
#define debugging false

typedef struct dirStruct { 
uint8_t status; // 0xe5 = unused, 0-16 = user number
char  name[9]; // no need to support attributes in msb  of bytes 0,1,2,7
char  extension[4]; // no need to support attributes in msb  of bytes 0,1,2
uint8_t XL; // see below for these 4  bytes' meaning
uint8_t BC; 
uint8_t XH; 
uint8_t RC;
uint8_t blocks[BLOCKS_PER_EXTENT]; // array of disk sectors used
} DirStructType;

typedef uint8_t Extent[32];
DirStructType *mkDirStruct(int index,uint8_t *e); 
void writeDirStruct(DirStructType *d, uint8_t index, uint8_t *e); 
void makeFreeList(); 
void printFreeList();
int findExtentWithName(char *name, uint8_t *block0); 
bool checkLegalName(char *name); 
void cpmDir(); 
int cpmRename(char *oldName, char * newName); 
int  cpmDelete(char * name); 
//int  cpmCopy(char *oldName, char *newName); 
//int  cpmOpen( char *fileName, char mode); 
//int cpmClose(int filePointer); 
//int cpmRead(int pointer, uint8_t *buffer, int size);
//int cpmWrite(int pointer, uint8_t *buffer, int size);  

