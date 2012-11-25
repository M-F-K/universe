/*****************************************************************************************
* File: main.cpp
* Desc: file analyser / dumper tool for the Universe game from Core (1994).
*****************************************************************************************/

/*
License :	Yadadadadadadadada, this code is free, you can use it for anything as long as you don't hold me liable for ANY damages
			that may be the result of using it. This includes dying of laughter of the piss poor quality. If you use it for
            anything, it would be nice if you dropped me a line, but this is optional.             
*/

// ----- Includes -----

#include <stdio.h>
#include <iostream>
#include <stack>
#include <cstdlib>
#include <string.h>
#include <stdint.h>

using namespace std;

typedef unsigned char	BYTE;       // An unsigned char can store 1 Bytes (8bits) of data (0-255)
typedef unsigned int	UINT32;

struct structFileEntry {
	char*	filename;
	bool	compressed;
	UINT32	compressedSize;
	UINT32	deCompressedSize;
	UINT32  fileOffset; 	

	structFileEntry() {
		filename			= new char [13];
		compressed			= false;
		compressedSize		= 0;
		deCompressedSize	= 0;
		fileOffset			= 0;
	}

	~structFileEntry() {
		delete[] filename;
	}
};
typedef struct structFileEntry FileEntry;




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
	const char *filePath = "UNIVERSE.EPF";	// File path to the universe binary
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

	fileBuf = new BYTE[fileSize];
	fread(fileBuf, fileSize, 1, file);


	cout << "======HEADER==INFO=======" << endl;	


	outputASCII(fileBuf, 0, 4, "signature   :");
	cout << "unknown     : TODO" << endl;
	cout << "numFiles    : TODO" << endl;
	// this only works because of a small file (ie 2 missing bytes)
	// need to figure out how to do a hex to unsigned long int conversion properly.......
	unsigned long  fatOffset = ((unsigned char)fileBuf[6] << 16) | ((unsigned char)fileBuf[5] << 8) | (unsigned char)fileBuf[4]; //FIXME
	printf("fatOffset   : %lu\n", fatOffset);


	cout << "======LOOKING=AT=FAT====" << endl;


	unsigned long tempOffset = fatOffset;
	int filesEncountered = 0;

	while ( tempOffset < fileSize ) {

		FileEntry f;
	
		outputASCII(fileBuf, tempOffset, 13, "File name   :");

		f.compressed = (bool)fileBuf[tempOffset+13+1];
		printf("compressed  : %d\n", f.compressed);

    
		char compressedSizeHex[8];
		memset(compressedSizeHex, 0, 8); // hmmm, if we don't have theese memsets, the result sometimes is jumbled / explodes
		nibblesLEToHexString(fileBuf, (tempOffset + 14), 4, compressedSizeHex);
		sscanf(compressedSizeHex, "%x", &f.compressedSize);
		cout << "comp. size  : " << f.compressedSize << endl;
	
	
		char deCompressedSizeHex[8];
		memset(deCompressedSizeHex, 0, 8); // hmmm, if we don't have theese memsets, the result sometimes is jumbled / explodes
		nibblesLEToHexString(fileBuf, (tempOffset + 18), 4, deCompressedSizeHex);
		sscanf(deCompressedSizeHex, "%x", &f.deCompressedSize);
		cout << "decomp. size: " << f.deCompressedSize << endl;
	
		tempOffset += 22;
		filesEncountered += 1;
	
	}

	cout << "" << endl;	
	cout << "Files expected in fat table    : " << "TODO" << endl;
	cout << "Files encountered in fat table : " << filesEncountered << endl;

	
	// TODO:	
	// * set the (calculated ) offset of the first file to the location after the header, the following files offset needs to be calculated....
	// * figure out why there are some jumbled caharcters extra in some of the filenames.....
	// * store f filentries in a arraylist
	// * have a goo at dumping the compressed files to disk

	// Test crap below this line ......
//	cout << "TODO : we need to convert these hex values to UINT32 = (3539) : " << endl;
//	for (unsigned int i = fatOffset + 14; i < (fatOffset + 18); i++)
//		printf("%02X ", fileBuf[i]);
//	
//	cout << "" << endl;
//	cout << "TODO : we need to convert these hex values to UINT32 = (6092) : " << endl;
//	for (unsigned int i = fatOffset + 18; i < (fatOffset + 22); i++)
//		printf("%02X ", fileBuf[i]);


	cout << "" << endl;
	cout << "" << endl;
	//cin.get();
	delete[]fileBuf;
    fclose(file);

	return 0;
}

