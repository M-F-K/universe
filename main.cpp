

// UniverseTest.cpp : Defines the entry point for the console application.
//

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
// #include <stdint.h>

using namespace std;

typedef unsigned char	BYTE;       // An unsigned char can store 1Bytes (8bits) of data (0-255)
typedef unsigned int	UINT32;
typedef unsigned short  UINT16;

#pragma pack(1)
struct structFileHeader {
	BYTE signature[4];
	UINT32 fatOffset;
	BYTE unknown;
	UINT16 numFiles;
};  // gcc - __attribute__((packed))

#pragma pack(1)
struct structFileEntry {
	char	filename[13];     //   SIZE 10
	BYTE	compressed;       //   SIZE 1
	UINT32	compressedSize;   //   SIZE 4
	UINT32	deCompressedSize; //   SIZE 4
};  // gcc - __attribute__((packed))

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


void outputASCII(BYTE buf[], int index, int read, const char* prefix){
	printf("%s ", prefix);	
	for (int i = index; i < (index + read ); i++){
		printf("%C", buf[i]);
	}
	printf("\n");
}

void outputUnsignedDecimalInteger(BYTE buf[], int index, int read){
	for (int i = index; i < (index + read ); i++){
		
		printf("%d", buf[i]);
	}
	printf("\n");
}

void nibblesToHexString(BYTE buf[], int index, int read, char* back){
	char result[50];
	memset(result, 0, 50);
	char charBuf[3];
	
	for (unsigned int i = index; i < (index + read ); i++){
		//printf("%02X ", buf[i]);
		int b = sprintf(charBuf, "%02X", buf[i]);
		//cout << b << endl;
		
		strncat(result, charBuf, 2);
	}

	//printf("%s", result);

	//printf("\n");

	strcat(back,result);
}


void nibblesLEToHexString(BYTE buf[], int index, int read, char* back){
	char result[50];
	memset(result, 0, 50);
	char charBuf[3];
	
	for (unsigned int i = (index + read - 1 ); i >= index; i--){
		//printf("%02X ", buf[i]);
		int b = sprintf(charBuf, "%02X", buf[i]);
		//cout << b << endl;
		
		strncat(result, charBuf, 2);
	}

	//printf("%s", result);

	//printf("\n");

	strcat(back,result);
}






// Converts a hexadecimal string to integer
int xtoi(const char* xs, unsigned int* result)
{
 size_t szlen = strlen(xs);
 int i, xv, fact;

 if (szlen > 0)
 {
  // Converting more than 32bit hexadecimal value?
  if (szlen>8) return 2; // exit

  // Begin conversion here
  *result = 0;
  fact = 1;

  // Run until no more character to convert
  for(i=szlen-1; i>=0 ;i--)
  {
   if (isxdigit(*(xs+i)))
   {
    if (*(xs+i)>=97)
    {
     xv = ( *(xs+i) - 97) + 10;
    }
    else if ( *(xs+i) >= 65)
    {
     xv = (*(xs+i) - 65) + 10;
    }
    else
    {
     xv = *(xs+i) - 48;
    }
    *result += (xv * fact);
    fact *= 16;
   }
   else
   {
    // Conversion was abnormally terminated
    // by non hexadecimal digit, hence
    // returning only the converted with
    // an error value 4 (illegal hex character)
    return 4;
   }
  }
 }

 // Nothing to convert
 return 1;
}
string intToHex(unsigned int dec)
{
  int i = 0;
  stack <int>remainder;
  string hex, temp;
  char hexDigits[] = { "0123456789abcdef" };

  if(dec == 0)
    hex = hexDigits[0];

  while (dec != 0)
  {
    remainder.push(dec % 16);
    dec /= 16;
    ++i;
  }

  while (i != 0)
  {
    if (remainder.top() > 15)
    {
      temp = intToHex(remainder.top());
      hex += temp;
    }
    hex.push_back(hexDigits[remainder.top()]);
    remainder.pop();
    --i;
  }
  return hex;
}



int main()
{
	UINT32 global_offset_counter_thingie = 11;

	const char *filePath = "./UNIVERSE.EPF";			// File path to the universe binary
	BYTE *fileBuf;							// Pointer to our buffered data
	FILE *file = NULL;						// File pointer

	if ((file = fopen(filePath, "rb")) == NULL) {
		cout << "Could not open specified file" << endl;
	}
	else {
		cout << "File opened successfully" << endl;
	}

	long fileSize = getFileSize(file);
	printf("filesize    : %lu\n", fileSize);

	
	FileHeader fh;	
	fread(&fh, sizeof(FileHeader), 1, file);
	
	printf("unknown     : %d\n", fh.unknown);
	printf("numFiles    : %d\n", fh.numFiles);
	printf("fatOffset   : %lu\n", (long unsigned int) fh.fatOffset);

	cout << "======LOOKING=AT=FAT====" << endl;
	
	fseek(file, fh.fatOffset, SEEK_SET);
	
	CFileEntry *f = new CFileEntry[fh.numFiles];
	
	memset((void*) f, 0x0, sizeof(FileEntry) * fh.numFiles);

	printf("sizeof = %d", sizeof(FileEntry));
	for (int a =0; a < fh.numFiles; a++) {
		
		fread(&f[a].fe, sizeof(FileEntry), 1, file);

		if(feof(file)) {
			abort();
		}

		char buffer_filename[14];
		strncpy(buffer_filename, f[a].fe.filename, 13);
		buffer_filename[13] = 0x0;


		printf("File name   : %s\n", buffer_filename);
		printf("comp. size  : %d\n", f[a].fe.compressedSize);
		printf("decomp. size: %d\n", f[a].fe.deCompressedSize);	
		
		f[a].offset = global_offset_counter_thingie;
		global_offset_counter_thingie += f[a].fe.compressedSize;
		printf("offset      : %d\n", f[a].offset);	
	}
	
		
    fclose(file);

	return 0;
}


