//#include "stdafx.h"    ... precompiled headers for win ... yuck ....

/*****************************************************************************************
* File: main.cpp
* Desc: file analyser / dumper tool for the Universe game from Core (1994).
*****************************************************************************************/

/*
License :	Yadadadadadadadada, this code is free, you can use it for
anything as long as you don't hold me liable for ANY damages
			that may be the result of using it. This includes dying of laughter
of the piss poor quality. If you use it for
            anything, it would be nice if you dropped me a line, but
this is optional.
*/
// https://github.com/Malvineous/libgamearchive/blob/master/src/filter-epfs.cpp
// ----- Includes -----
#include <stdio.h>
#include <iostream>
#include <stack>
#include <cstdlib>
#include <string.h>
#include "lzw.h"
// #include <stdint.h>

using namespace std;

typedef unsigned char	BYTE;		//   An unsigned char can store 1Bytes (8bits) of data (0-255)
typedef unsigned int	UINT32;
typedef unsigned short  UINT16;

#pragma pack(1)
struct structFileHeader {
	BYTE signature[4];
	UINT32 fatOffset;
	BYTE unknown;
	UINT16 numFiles;
} __attribute__((packed));		//   gcc specific pack

#pragma pack(1)
struct structFileEntry {
	char	filename[13];		//   SIZE 10
	BYTE	compressed;		//   SIZE 1
	UINT32	compressedSize;		//   SIZE 4
	UINT32	deCompressedSize;	//   SIZE 4
} __attribute__((packed));		//   gcc specific pack

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

void parseEpsStructure(FileHeader** fh_ptr, CFileEntry** f_ptr){

  UINT32 global_offset_counter_thingie = 11;
  
  const char *filePath = "./UNIVERSE.EPF";			// File path to the universe binary
  BYTE *fileBuf;						// Pointer to our buffered data
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
  BYTE *fileBuf;						// Pointer to our buffered data
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

     	cout << "File found in index : " << endl;
    	printf("File name: %s ", buffer_filename);
    	printf("comp. size: %d ", fileEntries[a].fe.compressedSize);
    	printf("decomp. size: %d ", fileEntries[a].fe.deCompressedSize);

  
	// TODO: inflate + save file..
	//fseek(file, fh.fatOffset, SEEK_SET);  //FIXME ...params
	//fread(&f[a].fe, sizeof(FileEntry), 1, file);  //FIXME ...params
    }
  }
    
  
  if(feof(file)) {
    abort();
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

