/*****************************************************************************************
* File: main.cpp
* Desc: file analyser / dumper tool for the Universe game from Core Design™ (1994).
*****************************************************************************************/

/*
License :	Yadadadadadadadada, this code is free, you can use it for
            anything as long as you don't hold me liable for ANY damages
			   that may be the result of using it. This includes dying of laughter
            of the piss poor quality. If you use it for anything, it would
            be nice if you dropped me a line, but this is optional.
*/

// https://github.com/Malvineous/libgamearchive/blob/master/src/filter-epfs.cpp

// ----- Includes -----
#include <stdio.h>
#include <iostream>
#include <stack>
#include <cstdlib>
#include <string.h>
#include <vector>
#include <stdint.h>

using namespace std;

uint64_t counter = 0;            //   parse specific global
uint8_t  *buffer = NULL;         //   parse specific global 

typedef unsigned char	BYTE;		//   An unsigned char can store 1Bytes (8bits) of data (0-255)
typedef unsigned int	UINT32;
typedef unsigned short  UINT16;

#pragma pack(1)
struct structFileHeader {
	BYTE signature[4];
	UINT32 fatOffset;
	BYTE unknown;
	UINT16 numFiles;
} __attribute__((packed));       //   gcc specific pack

#pragma pack(1)
struct structFileEntry {
	char	filename[13];           //   SIZE 10
	BYTE	compressed;             //   SIZE 1
	UINT32	compressedSize;      //   SIZE 4
	UINT32	deCompressedSize;    //   SIZE 4
} __attribute__((packed));       //   gcc specific pack

struct structCompleteFileEntry {
	struct structFileEntry fe;
	UINT32 offset;
};

typedef struct structCompleteFileEntry CFileEntry;
typedef struct structFileEntry FileEntry;
typedef struct structFileHeader FileHeader;


// Get the size of a file
long getFileSize(FILE *file){
	long lCurPos, lEndPos;
	lCurPos = ftell(file);
	fseek(file, 0, 2);
	lEndPos = ftell(file);
	fseek(file, lCurPos, 0);
	return lEndPos;
}


uint64_t getNextToken(size_t bitsize) {

   uint64_t mask = (1 << (bitsize+1)) - 1;
   uint64_t remainder = counter % 8;
   uint64_t *ptr = (uint64_t*) buffer + (counter / 8);

   counter = counter + bitsize;

   uint64_t tmp = *ptr; // FIXME: this generates a segfault ... buffer is 0x0, counter is 0, and therefore ptr is also 0x0
   uint64_t value = tmp >> remainder;

   return value & mask;
}


void parseBuffer(){
   /*
      Setting up the LZW string output table, index 0 - 255 is a copy of the ascii table.
      This means that index 256 and forward is going to hold the decompressed data.
   */
   vector<string> stringTable;
   for (int i=0; i<256; i++){
      char c = i;      
      char* temp = &c;
      stringTable.push_back(temp);
   }

   int bitlength = 9; // initial bitlength is 9, max bitlength is 14
   int nextFreeIndex = 256;

   /*uint64_t*/ counter = 0;
   /*uint8_t */ //buffer = NULL;
   uint64_t oldCode = 0;
   uint64_t newCode = 0;
   char* character = 0;
   string str = "";

   /* TODO: implement the LZW algorithm here */
   
   oldCode  = getNextToken(bitlength); // FIXME: remember to increase the bitlength  ... also this currently segfaults here (178)
   cout << oldCode;
   *character = (char) oldCode;


   while ((newCode = getNextToken(bitlength)) != 1023){
      if (newCode < nextFreeIndex) {
         str = stringTable.at(newCode);
         str = str + character;
      }
      else{
         str = str + str[0];//get translation of NEW_CODE;    .. note , this is probably fucked up ....fixme
      }
      cout << str;
      *character = str[0];
      stringTable.push_back(oldCode + character);
      nextFreeIndex++;
      oldCode = newCode;
   }

}


void parseEpsStructure(FileHeader** fh_ptr, CFileEntry** f_ptr){

   UINT32 global_offset_counter_thingie = 11;
  
  const char *filePath = "./UNIVERSE.EPF";			// File path to the universe binary
  //BYTE *fileBuf;						// Pointer to our buffered data
  FILE *file = NULL;						// File pointer
  
  if ((file = fopen(filePath, "rb")) == NULL) {
    cout << "Could not open specified file" << endl;
  }
  else {
    cout << "File opened successfully" << endl;
  }
  
  long fileSize = getFileSize(file);
  printf("filesize    : %lu\n", fileSize);
  
  FileHeader* fh = new FileHeader;
  *fh_ptr = fh;	
  fread(&*fh, sizeof(FileHeader), 1, file);
  
  printf("unknown     : %d\n", fh->unknown);
  printf("numFiles    : %d\n", fh->numFiles);
  printf("fatOffset   : %lu\n", (long unsigned int) fh->fatOffset);
  
  cout << "======LOOKING=AT=FAT====" << endl;
  
  fseek(file, fh->fatOffset, SEEK_SET);
  CFileEntry* f = new CFileEntry[fh->numFiles];
  *f_ptr = f;
  
  memset((void*) f, 0x0, sizeof(FileEntry) * fh->numFiles);
  
  //printf("sizeof = %d", sizeof(FileEntry));
  for (int a =0; a < fh->numFiles; a++) {
    
    fread(&f[a].fe, sizeof(FileEntry), 1, file);
    
    if(feof(file)) {
      abort();
    }
    
    char buffer_filename[14];
    strncpy(buffer_filename, f[a].fe.filename, 13);
    buffer_filename[13] = 0x0;
    
    
    printf("index: %d ", a);
    printf("File name: %s ", buffer_filename);
    printf("comp. size: %d ", f[a].fe.compressedSize);
    printf("decomp. size: %d ", f[a].fe.deCompressedSize);	
    
    f[a].offset = global_offset_counter_thingie;
    global_offset_counter_thingie += f[a].fe.compressedSize;
    printf("offset: %d\n", f[a].offset);	
  }
  
  fclose(file);
}

void inflateFile(const char* packedFilename, FileHeader* fileHeader, CFileEntry* fileEntries){
  const char *filePath = "./UNIVERSE.EPF";			// File path to the universe binary
  //BYTE *fileBuf;						// Pointer to our buffered data
  FILE *file = NULL;						// File pointer
  
  if ((file = fopen(filePath, "rb")) == NULL) {
    cout << "Could not open specified file" << endl;
  }
  else {
    cout << "File opened successfully" << endl;
  }
  
  for (int a =0; a < fileHeader->numFiles; a++) {

    char buffer_filename[14];
    strncpy(buffer_filename, fileEntries[a].fe.filename, 13);
    buffer_filename[13] = 0x0;
    
    if(strcmp(packedFilename, fileEntries[a].fe.filename) == 0) {

     	cout << "File found in index!" << endl;
    	printf("File name   : %s \n", buffer_filename);
    	printf("comp. size  : %d \n", fileEntries[a].fe.compressedSize);
    	printf("decomp. size: %d \n", fileEntries[a].fe.deCompressedSize);

  
	   // load the compressed filedata into buffer
	   char buffer[fileEntries[a].fe.compressedSize + 8];	// we're going to read 64 bytes at a time, so we add 8 ekstra bytes to avoid out of bounds read
      memset(buffer, 0, sizeof buffer);
	   fseek(file, /*fileHeader->fatOffset*/fileEntries[a].offset, SEEK_SET);
	   fread(buffer, fileEntries[a].fe.compressedSize, 1, file);
	
      //parseBuffer(); // TODO: turn this on when everything works....
    }

    if(feof(file)) {
    	abort();
    }

  }
  fclose(file);
}



int main() {
  FileHeader *fh = NULL;
  CFileEntry *f = NULL;
  parseEpsStructure(&fh,&f);
  inflateFile("README.TXT", fh, f);
  return 0;
}

