#include <stdio.h>
#include <iostream>
#include <stack>
#include <cstdlib>
#include <string.h>

using namespace std;

typedef unsigned char BYTE;       // An unsigned char can store 1 Bytes (8bits) of data (0-255)
typedef unsigned long int UINT32;

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
		fileOffset 			= 0;
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

	// For now we just look at the first file in the fat - TODO: later on implement a pass where all fileoffsets are calculated
	FileEntry f;
	
	outputASCII(fileBuf, fatOffset, 13, "File name   :");

	f.compressed = (bool)fileBuf[fatOffset+13+1];
	printf("compressed  : %d\n", f.compressed);

	// something is wrong here: hexeditor says that compressed size should be 3539 decimal aka "D3 0D 00 00" in hex
	f.compressedSize = ((unsigned char)fileBuf[6100488] << 16) | ((unsigned char)fileBuf[6100487] << 8) | (unsigned char)fileBuf[6100486]; // FIXME: same goes fore this one...
	printf("%lu\n", f.compressedSize);
	
	// hexeditor states : uncompressed 6092 decimal aka "CC 17 00 00" hex

	cout << "TODO : we need to convert these hex values to UINT32 = (3539) : " << endl;
	for (unsigned int i = fatOffset + 14; i < (fatOffset + 18); i++)
		printf("%02X ", fileBuf[i]);
	cout << "" << endl;
	cout << "TODO : we need to convert these hex values to UINT32 = (6092) : " << endl;
	for (unsigned int i = fatOffset + 18; i < (fatOffset + 22); i++)
		printf("%02X ", fileBuf[i]);


	cin.get();
	delete[]fileBuf;
        fclose(file);   // Almost forgot this 
	return 0;
}

