#include <stdint.h> 
#include <stdlib.h> 
#include <stdbool.h> 
#include <string.h> 
#include <stdio.h>
#include "diskSimulator.h"
#include "cpmfsys.h"

//Global variables
uint8_t buffer1[BLOCK_SIZE];
bool FreeList[256]; 

//function to allocate memory for a DirStructType (see above), and populate it, given a
//pointer to a buffer of memory holding the contents of disk block 0 (e), and an integer index
// which tells which extent from block zero (extent numbers start with 0) to use to make the
// DirStructType value to return.
struct dirStruct *mkDirStruct(int index,uint8_t *e){
  int row, col;
  int buf_index = 0;
  blockRead(e,index); //Read blocks from the disk
  struct dirStruct *d = malloc(32*sizeof(DirStructType)); //Allocate memory for Directory structure.
  for(row=0;row<32;row++){
	for(col=0;col<32;col++){
	if(col == 0){
		(d+row)->status = *(e+buf_index); //Transfer status
	}
	else if(col>=1 && col<=8){
		(d+row)->name[col-1] = (char)*(e+buf_index); //Transfer file name
		(d+row)->name[9] = '\0';
	}
	else if(col>=9 && col<=11){
		(d+row)->extension[col-9] = (char)*(e+buf_index); //Transfer extension
		(d+row)->extension[3] = '\0';
	}
	else if(col==12){
		(d+row)->XL = *(e+buf_index); //Transfer XL
	}
	else if(col==13){
		(d+row)->BC = *(e+buf_index); //Transfer BC  
	}
	else if(col==14){
		(d+row)->XH = *(e+buf_index); //Transfer XH
	}
	else if(col==15){
		(d+row)->RC = *(e+buf_index); //Transfer RC
	}
	else if(col>=16 && col<=31){
		(d+row)->blocks[col-16] = *(e+buf_index); //Transfer blocks and memory map
	}
	buf_index++; //Increase buffer index
	}
  }
  return d;
}


// function to write contents of a DirStructType struct back to the specified index of the extent
// in block of memory (disk block 0) pointed to by e
void writeDirStruct(DirStructType *d, uint8_t index, uint8_t *e){
  int row, col;
  int buf_index = 0;
  for(row=0;row<32;row++){
	for(col=0;col<32;col++){
	if(col == 0){
		*(e+buf_index) = (d+row)->status; //Write back status
	}
	else if(col>=1 && col<=8){
		*(e+buf_index) = (uint8_t)(d+row)->name[col-1]; //Write back name
	}
	else if(col>=9 && col<=11){
		*(e+buf_index) = (uint8_t)(d+row)->extension[col-9]; //Write back extension
	}
	else if(col==12){
		*(e+buf_index) = (d+row)->XL; //Write back XL
	}
	else if(col==13){
		*(e+buf_index) = (d+row)->BC; //Write back BC
	}
	else if(col==14){
		*(e+buf_index) = (d+row)->XH; //Write back XH
	}
	else if(col==15){
		*(e+buf_index) = (d+row)->RC; //Write back RC
	}
	else if(col>=16 && col<=31){
		*(e+buf_index) = (d+row)->blocks[col-16]; //Write back memory block map
	}
	buf_index++;
	}
  }
  blockWrite(e,index); //Write buffer back to the Disk
  free(d); //Free memory
}

// populate the FreeList global data structure. freeList[i] == true means 
// that block i of the disk is free. block zero is never free, since it holds
// the directory. freeList[i] == false means the block is in use. 
void makeFreeList(){
  int i,j;
  DirStructType *d;
  d = mkDirStruct(0,buffer1);
  for(i=0;i<NUM_BLOCKS;i++){FreeList[i]=false;} //Initialize free list with everything false
  for(i=0;i<32;i++){
	if((d+i)->status != 229){
		for(j=0;j<16;j++){	//Check what blocks are used
			FreeList[(d+i)->blocks[j]]=true; //Mark them true in FreeList		
		}
 	}
  }
  writeDirStruct(d,0,buffer1); //Save directory structure 	
}


// debugging function, print out the contents of the free list in 16 rows of 16, with each 
// row prefixed by the 2-digit hex address of the first block in that row. Denote a used
// block with a *, a free block with a .
void printFreeList(){
  int i,j,k=0;
  printf("FREE BLOCK LIST: (* means in-use)\n");
  for(i=0;i<16;i++){
	printf("%02x : ",i<<4); //print indexes in the hex
  	for(j=0;j<16;j++){
		if(FreeList[k]==true) //Print marks of FreeList
			printf("* ");
		else
			printf(". ");
		k++;
	}
	printf("\n");
  }
  printf("\n");
}


// internal function, returns -1 for illegal name or name not found
// otherwise returns extent nunber 0-31
int findExtentWithName(char *name, uint8_t *block0){
  int i,j=0,k=0;
  char name1[15];
  DirStructType *d;
  d = mkDirStruct(0,block0); //Load directory structure
  for(i=0;i<32;i++){
	if((d+i)->status != 229){
		while(j!=8 && (d+i)->name[j]!=' '){
			name1[k] = (d+i)->name[j]; //Concate name for name.extension
			j++;
			k++;
		}
		j=0;
		name1[k] = '.'; //Add "."
		k++;
		while(j!=4 && (d+i)->extension[j]!=' '){
			name1[k] = (d+i)->extension[j]; //Concate extension for name.extension
			j++;
			k++;
		}
		if(j==0)
			name1[k-1]='\0'; //if no extension replace "." with NULL		
		else
			name1[k] = '\0';
		j=0;
  		if(strcmp(name1,name)==0){  //Compare names
			free(d);
			return i; //If true return extent 0-31
		}
		k=0;
	}
  }
  free(d);
  return -1;
}


// internal function, returns true for legal name (8.3 format), false for illegal
// (name or extension too long, name blank, or  illegal characters in name or extension)
bool checkLegalName(char *name){
  char namecheck[13];
  int i,length,namecnt=0,extcnt=0;
  bool dotfound=false;
  strcpy(namecheck,name);
  length = strlen(namecheck);
  if(!(namecheck[0] >= 'a' && namecheck[0] <= 'z') || (namecheck[0] >= 'A' && namecheck[0] <= 'Z')){
  	return false;	//Return false if new name starts with illegal character
  }
  for(i=0;i<length;i++){
	if(namecheck[i] == '.'){dotfound = true;} //Check name and extension seperately
  	if(dotfound == false){
		namecnt++;
	}
	else if(dotfound == true){
		extcnt++;
	}
  }
  if(namecnt>9 || extcnt>4) //Return false if name is greater than 8 and extension is greater than 4
	return false;
  else
	return true;	//If the name meets all requirements return true
}

// print the file directory to stdout. Each filename should be printed on its own line, 
// with the file size, in base 10, following the name and extension, with one space between
// the extension and the size. If a file does not have an extension it is acceptable to print
// the dot anyway, e.g. "myfile. 234" would indicate a file whose name was myfile, with no 
// extension and a size of 234 bytes. This function returns no error codes, since it should
// never fail unless something is seriously wrong with the disk
void cpmDir(){
  int i,j=0,k;
  DirStructType *d;
  int NumBlocks=0;
  int TotalBytes;
  d = mkDirStruct(0,buffer1);
  printf("*DIRECTORY LISTING*\n");
  for(i=0;i<32;i++){
	if((d+i)->status != 229){ //Only proceed further if the block is in use
		while(j!=8 && (d+i)->name[j]!=' '){
			printf("%c",(d+i)->name[j]); //Print name
			j++;
		}
		printf("."); //Print "."
		j=0;
		while(j!=4 && (d+i)->extension[j]!=' '){
			printf("%c",(d+i)->extension[j]); //Print extension
			j++;
		}
		j=0;
		for(k=0;k<16;k++){
			if((d+i)->blocks[k]>0) //Scan all the blocks in use
			NumBlocks++;	
		}
		TotalBytes = (1024*(NumBlocks-1)) + (128*(d+i)->RC) + (d+i)->BC; //Calculate total bytes
		NumBlocks = 0; //Reset number of blocks to 0 for the next file's bytes calculation
		printf(" %d\n",TotalBytes);
	}
  }
  writeDirStruct(d,0,buffer1);
  printf("\n");
}


// error codes for next five functions (not all errors apply to all 5 functions)  
/* 
    0 -- normal completion
   -1 -- source file not found
   -2 -- invalid  filename
   -3 -- dest filename already exists 
   -4 -- insufficient disk space 
*/

//read directory block, 
// modify the extent for file named oldName with newName, and write to the disk
int cpmRename(char *oldName, char *newName){
  int extent_index;
  char namecheck[13];
  char name[9];
  char extension[4];
  int i,length,namecnt=0,extcnt=0;
  bool dotfound=false;

  strcpy(namecheck,newName);
  length = strlen(namecheck);
  DirStructType *d;
  d = mkDirStruct(0,buffer1);
  if(checkLegalName(newName)==0)
	return -2;
  if((extent_index = findExtentWithName(oldName,buffer1)) == -1)
	return -1;

  for(i=0;i<length;i++){
	if(namecheck[i] == '.'){dotfound = true;i++;}
  	if(dotfound == false){
		name[namecnt] = namecheck[i];
		namecnt++;
	}
	name[namecnt] = '\0';
	if(dotfound == true){
		extension[extcnt] = namecheck[i];
		extcnt++;
	}
	extension[extcnt] = '\0';
  }
  printf("RENAMING file \"%s\" to \"%s\"...",oldName,newName);
  strcpy((d+extent_index)->name,name);
  strcpy((d+extent_index)->extension,extension);
  writeDirStruct(d,0,buffer1);
  printf("Done\n");
  return 0;
}


// delete the file named name, and free its disk blocks in the free list
int  cpmDelete(char * name){
  int extent_index;
  DirStructType *d;
  if((extent_index = findExtentWithName(name,buffer1)) == -1)
	return -1;
  else
 	d = mkDirStruct(0,buffer1);
  	(d+extent_index)->status = 229;
  	printf("File \"%s\" is deleted.\n",name);
  	writeDirStruct(d,0,buffer1);
  	makeFreeList();
  return 0;
}
