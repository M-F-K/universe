
#ifndef UPRIGHT_LZW_H

#define UPRIGHT_LZW_H





/* LZW.h by N.A.Bozinis @ 19/01/2010 08:55:52

 * ----------------------------------------------------------------------------------

 *

 * Plain C++ port of LZW compression algorithm and code originally (c) Mark R. Nelson

 * http://marknelson.us/1989/10/01/lzw-data-compression

 * Variable bit length encoding idea and code originally by Michael Dipperstein

 * http://michael.dipperstein.com/lzw

 *

 * There are a lot of compression classes floating around but most are based on the

 * zlib (zip/unzip) library, which is good but a bit of overkill for simple and small

 * code. LZW combines decent compression ratios with very small code footprint. If

 * you need something more powerful here are a few resources:

 *

 *      http://www.codeproject.com/KB/files/zip_utils.aspx

 *      http://www.codeproject.com/KB/cpp/xzipunzip.aspx

 *      http://www.codeproject.com/KB/cpp/ChauMemzip.aspx

 *

 * Microsoft types can check the CAB protocol that is available in all windows:

 *      http://www.codeproject.com/KB/files/CABCompressExtract.aspx

 *      http://msdn.microsoft.com/en-us/library/bb417343.aspx

 *

 */



#include <stdio.h>

#include <stdlib.h>

#include <limits.h>

#include <string.h>

#include <assert.h>

#define ATLASSERT assert



/* NOTE: function and variable names left as much as possible matching the original

	LZW.c by Mark, naturally bundled in classes to get rid of static/globals etc

*/



#define MIN_CODE_LEN    9                   /* min # bits in a code word */

#define MAX_CODE_LEN    20                  /* max # bits in a code word */

#define CURRENT_MAX_CODES(x)     (1UL << (x))



#define FIRST_CODE      (1 << CHAR_BIT)     /* value of 1st string code */



#if (MIN_CODE_LEN <= CHAR_BIT)

#error Code words must be larger than 1 character

#endif



#if (MAX_CODE_LEN >= 25)

#error Code words must fit in an integer

#endif





/* VARIABLE BIT LENGTH ENCODING

 * Instead of using a fixed number of bits for code words, we start at 9 (=MIN_CODE_LEN)

 * and go up to BITS (<=MAX_CODE_LEN) so that small files are tightly packed and larger

 * files are fine too. The BITS constant determines the maximum hash table size. For 18

 * this means 250KB runtime table size which is enough for files ~4MB.

 * There is no problem for files larger than that; if we run out of table space for new

 * codes then the same codes are emitted (uncompressed obviously)

 */



#define BITS 17                   /* Setting the number of bits to 12, 13*/

#define HASHING_SHIFT (BITS-8)    /* or 14 affects several constants.    */

#define MAX_VALUE (1 << BITS) - 1 /* Note that MS-DOS machines need to   */

#define MAX_CODE MAX_VALUE - 1    /* compile their code in large model if*/

				  /* 14 bits are selected.               */



#if BITS == 20

	#define TABLE_SIZE 1048583

#elif BITS == 19

	#define TABLE_SIZE 524309

#elif BITS == 18

	#define TABLE_SIZE 262147

#elif BITS == 17

	#define TABLE_SIZE 131101

#elif BITS == 16

	#define TABLE_SIZE 65543

#elif BITS == 15

	#define TABLE_SIZE 32797

#elif BITS == 14

  #define TABLE_SIZE 18041        /* The string table size needs to be a */

				  /* prime number that is somewhat larger*/

#elif BITS == 13                  /* than 2**BITS.                       */

  #define TABLE_SIZE 9029

#elif BITS == 12

  #define TABLE_SIZE 5021

#else

#error define smaller or bigger table sizes

#endif



#if (TABLE_SIZE <= MAX_VALUE)

#error your prime numbers need attention

#endif



#if (BITS > MAX_CODE_LEN)

#error BITS can only go up to a maximum

#endif





/*

 This class does most of the job, except reading source and writing the compressed data

 A derived class does that so that there's flexibility to read either from files or memory

 */



class CLZWImpl {

protected:

int *code_value;                  /* This is the code value array        */

unsigned int *prefix_code;        /* This array holds the prefix codes   */

unsigned char *append_character;  /* This array holds the appended chars */

unsigned char decode_stack[4000]; /* This array holds the decoded string */

unsigned char CUR_BITS;           /* ~nab: added for variable bit size */

/* we are processing bits but in the end of the day we do I/O in bytes */

int input_bit_count, output_bit_count;

unsigned long input_bit_buffer, output_bit_buffer;



public:

CLZWImpl() {

	code_value = 0;

	prefix_code = 0;

	append_character = 0;

}



~CLZWImpl() {

	if(code_value)

		free(code_value);

	if(prefix_code)

		free(prefix_code);

	if(append_character)

		free(append_character);

}



int get_bits() { return CUR_BITS; }



protected:

int Init() {

	ATLASSERT(!code_value); /* call just once */



	code_value=(int*)malloc(TABLE_SIZE*sizeof(int));

	prefix_code=(unsigned int*)malloc(TABLE_SIZE*sizeof(unsigned int));

	append_character=(unsigned char*)malloc(TABLE_SIZE*sizeof(unsigned char));



	return code_value != 0 && prefix_code != 0 && append_character != 0;

}



/* override these 4: read a byte from source */

virtual int getc_src() = 0;

/* read a byte from compressed source (during expansion) and write to compressed output */

virtual int getc_comp() = 0;

/* write a byte to compressed output */

virtual int putc_comp(int ch) = 0;

/* write a byte to expanded output */

virtual int putc_out(int ch) = 0;



/*

** This is the compression routine.  The code should be a fairly close

** match to the algorithm accompanying the article.

**

*/



void compress()

{

unsigned int next_code;

unsigned int character;

unsigned int string_code;

unsigned int index;

unsigned int bit_limit;

int i;



	ATLASSERT(code_value); /* initialized? */



	CUR_BITS = MIN_CODE_LEN;

	bit_limit = CURRENT_MAX_CODES(CUR_BITS) - 1;

	output_bit_count=0;

	output_bit_buffer=0L;



	ATLASSERT(256==FIRST_CODE);

	next_code=FIRST_CODE;       /* Next code is the next available string code*/

	for (i=0;i<TABLE_SIZE;i++)  /* Clear out the string table before starting */

		code_value[i]=-1;



  string_code=getc_src();      /* Get the first code                         */

  if(-1 == string_code)

	  return; /* empty file or error */



/*

** This is the main loop where it all happens.  This loop runs util all of

** the input has been exhausted.  Note that it stops adding codes to the

** table after all of the possible codes have been defined.

*/

  while ((character=getc_src()) != -1)

  {

    index=find_match(string_code,character);/* See if the string is in */

    if (code_value[index] != -1)            /* the table.  If it is,   */

      string_code=code_value[index];        /* get the code value.  If */

    else                                    /* the string is not in the*/

    {                                       /* table, try to add it.   */

      if (next_code <= MAX_CODE)

      {

	code_value[index]=next_code++;

	prefix_code[index]=string_code;

	append_character[index]=character;

      }



		/* are we using enough bits to write out this code word? */

		if(string_code >= bit_limit && CUR_BITS < BITS)

		{

			/* mark need for bigger code word with all ones */

			output_code(bit_limit);

			CUR_BITS++;

			bit_limit = (CURRENT_MAX_CODES(CUR_BITS) - 1);

		}



		ATLASSERT(string_code < bit_limit);



      output_code(string_code);  /* When a string is found  */

      string_code=character;            /* that is not in the table*/

    }                                   /* I output the last string*/

  }                                     /* after adding the new one*/

/*

** End of the main loop.

*/



  output_code(string_code); /* Output the last code               */

  output_code(-1);          /* This code flushes the output buffer*/

}



/*

** This is the hashing routine.  It tries to find a match for the prefix+char

** string in the string table.  If it finds it, the index is returned.  If

** the string is not found, the first available index in the string table is

** returned instead.

*/

int find_match(unsigned int hash_prefix,unsigned int hash_character)

{

int index;

int offset;



  index = (hash_character << HASHING_SHIFT) ^ hash_prefix;

  if (index == 0)

    offset = 1;

  else

    offset = TABLE_SIZE - index;

  while (1)

  {

    if (code_value[index] == -1)

      return(index);

    if (prefix_code[index] == hash_prefix &&

	append_character[index] == hash_character)

      return(index);

    index -= offset;

    if (index < 0)

      index += TABLE_SIZE;

  }

}



/*

**  This is the expansion routine.  It takes an LZW format file, and expands

**  it to an output file.  The code here should be a fairly close match to

**  the algorithm in the accompanying article.

*/



void expand()

{

unsigned int next_code;

unsigned int new_code;

unsigned int old_code;

int character;

unsigned char *string;

unsigned int bit_limit;



	ATLASSERT(code_value); /* initialized? */



	CUR_BITS = MIN_CODE_LEN;

	bit_limit = CURRENT_MAX_CODES(CUR_BITS) - 1;

	input_bit_count=0;

	input_bit_buffer=0L;



// @@@ what if we pass uncompressed file to decode?



  next_code=FIRST_CODE;        /* This is the next available code to define */



  old_code=input_code();       /* Read in the first code, initialize the */

  if(-1 == old_code)

	  return; /* read error? */

  character=old_code;          /* character variable, and send the first */

  if(putc_out(old_code)==-1)   /* code to the output file                */

	  return; /* write error */

/*

**  This is the main expansion loop.  It reads in characters from the LZW file

**  until it sees the special code used to inidicate the end of the data.

*/

  while ((new_code=input_code()) != (-1))

  {

	  /* look for code length increase marker */

	  if(bit_limit == new_code && CUR_BITS < BITS)

	  {

		  CUR_BITS++;

		  bit_limit = CURRENT_MAX_CODES(CUR_BITS) - 1;



		  new_code=input_code();

		  ATLASSERT(new_code != -1); /* must be read error? */

		  if(new_code == -1)

			  break;

	  }



		ATLASSERT(new_code < bit_limit);



/*

** This code checks for the special STRING+CHARACTER+STRING+CHARACTER+STRING

** case which generates an undefined code.  It handles it by decoding

** the last code, and adding a single character to the end of the decode string.

*/

    if (new_code>=next_code)

    {

      *decode_stack=character;

      string=decode_string(decode_stack+1,old_code);

    }

/*

** Otherwise we do a straight decode of the new code.

*/

    else

      string=decode_string(decode_stack,new_code);

/*

** Now we output the decoded string in reverse order.

*/

    character=*string;

    while (string >= decode_stack)

      putc_out(*string--);

/*

** Finally, if possible, add a new code to the string table.

*/

    if (next_code <= MAX_CODE)

    {

      prefix_code[next_code]=old_code;

      append_character[next_code]=character;

      next_code++;

    }

    old_code=new_code;

  }

}



/*

** This routine simply decodes a string from the string table, storing

** it in a buffer.  The buffer can then be output in reverse order by

** the expansion program.

*/

/* ~nab: these char* aren't a risk for unicode; we are reading bytes */

unsigned char *decode_string(unsigned char *buffer,unsigned int code)

{

int i;



  i=0;

  while (code >= FIRST_CODE)

  {

    *buffer++ = append_character[code];

    code=prefix_code[code];

	 i++;

	 ATLASSERT(i < sizeof(decode_stack)); /* buffer overrun if it blows, increase stack size! */

  }

  *buffer=code;

  return(buffer);

}



/*

** The following two routines are used to output variable length

** codes.  They are written strictly for clarity, and are not

** particularyl efficient.



  ~nab: there's room for improvement in these I/O functions eg work in DWORDS instead of bytes

*/



unsigned int input_code()

{

int c;

unsigned int return_value;

//static int input_bit_count=0;

//static unsigned long input_bit_buffer=0L;



  while (input_bit_count <= 24)

  {

		if ((c = getc_comp()) == -1)

			break;



    input_bit_buffer |=

	(unsigned long) c << (24-input_bit_count);

    input_bit_count += 8;

  }



  if(input_bit_count < CUR_BITS) {

	  ATLASSERT(!input_bit_buffer);

		return -1; /* EOF */

	}



  return_value=input_bit_buffer >> (32-CUR_BITS);

  input_bit_buffer <<= CUR_BITS;

  input_bit_count -= CUR_BITS;



  ATLASSERT(return_value < (1UL << CUR_BITS));

  return(return_value);

}



/* bits are written outside normal byte boundaries, hence the need for keeping old values */

void output_code(unsigned int code)

{

//static int output_bit_count=0;

//static unsigned long output_bit_buffer=0L;



	ATLASSERT(output_bit_count < 8); /* leftovers */

	ATLASSERT(CUR_BITS + output_bit_count <= 32);

	/*codes <256 are possible for single characters, zero bytes etc*/



	if(-1 == code) {

		/* pad remaining zeros and flush the last byte */

		if(output_bit_count) {

			output_bit_buffer >>= 24;

			ATLASSERT((output_bit_buffer & 0xFF) == output_bit_buffer);

			putc_comp(output_bit_buffer);



			output_bit_count = 0;

			output_bit_buffer = 0; /* in case some eejit calls us again */

		}



		return;

	}



	ATLASSERT(code < (1UL << CUR_BITS));



	/* sends new bytes near the top (MSB) */

  output_bit_buffer |= (unsigned long) code << (32-CUR_BITS-output_bit_count);

  output_bit_count += CUR_BITS;

  while (output_bit_count >= 8)

  {

	 /* no check for error but if there was a problem we'd know from the time we wrote the identifier */

    putc_comp(output_bit_buffer >> 24);

    output_bit_buffer <<= 8;

    output_bit_count -= 8;

  }

}

}; /* CLZWImpl */



/* example derived class using C buffered I/O functions */

class CLZWCompressFile : public CLZWImpl {

public:

	CLZWCompressFile() {

		io_file = 0;

		lzw_file = 0;

	};



	~CLZWCompressFile() {

		ATLASSERT(!io_file);

		ATLASSERT(!lzw_file);

	};



	int AnyIOErrors() {return io_error; }



	// @@@ these char* should be changed for unicode builds

	unsigned int Compress(char* input_file_name, char* to_name)

	{

		ATLASSERT(input_file_name && *input_file_name);

		ATLASSERT(to_name && *to_name);

		ATLASSERT(strcmp(to_name, input_file_name));



		io_error = 1;



		if(!code_value)

			if(!Init())

				return 0; /* rare memory error */



		u_comp = 0;

		u_io = 0;

		io_file=fopen(input_file_name,"rb");

		if(io_file) {

			lzw_file=fopen(to_name,"wb");

			if(lzw_file) {

				/* write LZW identifier L+starting bytes */

				putc('L', lzw_file);

				if(putc(MIN_CODE_LEN, lzw_file) == MIN_CODE_LEN) {

					compress();

					io_error = ferror(lzw_file) || ferror(io_file);

					if(!io_error)

						ATLASSERT(u_comp <= u_io); /* this is bound to bomb every now and then, no compression! */

				}

				fclose(lzw_file);

				lzw_file = 0;

			}



			fclose(io_file);

			io_file = 0;

		}



		return u_comp;

	}



	unsigned int Expand(char* lzw_name, char* to_name)

	{

		ATLASSERT(lzw_name && *lzw_name);

		ATLASSERT(to_name && *to_name);

		ATLASSERT(strcmp(to_name, lzw_name));



		io_error = 1;



		if(!code_value)

			if(!Init())

				return 0; /* rare memory error */



		u_comp = 0;

		u_io = 0;

		lzw_file=fopen(lzw_name,"rb");

		if(lzw_file) {

			/* check LZW identifier L+starting bytes */

			int ch1 = getc(lzw_file);

			int ch2 = getc(lzw_file);

			if('L' == ch1 && MIN_CODE_LEN==ch2) {

				io_file=fopen(to_name,"wb");

				if(io_file) {

					expand();

					io_error = ferror(lzw_file) || ferror(io_file);



					fclose(io_file);

					io_file = 0;

				}

			}



			fclose(lzw_file);

			lzw_file = 0;

		}



		return u_io;

	}



protected:

	/* -1 return indicates either EOF or some IO error */

	virtual int getc_src() {

		ATLASSERT(io_file);

		int ch = getc(io_file);

		if(EOF == ch)

			return -1;



		u_io++;

		return ch;

	}

	virtual int getc_comp() {

		ATLASSERT(lzw_file);

		int ch = getc(lzw_file);

		if(EOF == ch)

			return -1;



		u_comp++;

		return ch;

	}

	virtual int putc_comp(int ch) {

		ATLASSERT(lzw_file);

		ATLASSERT(ch >= 0 && ch < 256);

		int ret = putc(ch, lzw_file);



		if(ret != EOF) {

			ATLASSERT(ret == ch);

			u_comp++;

		}

		else

			ret = -1;



		return ret;

	}

	virtual int putc_out(int ch) {

		ATLASSERT(io_file);

		ATLASSERT(ch >= 0 && ch < 256);

		int ret = putc(ch, io_file);



		if(ret != EOF)

			u_io++;

		else

			ret = -1;



		return ret;

	}



	FILE* io_file;

	FILE *lzw_file;

	int io_error;

public:

	unsigned long u_io, u_comp; /* bytes read and written */

};



// @@@ could have a generic one on IStream, CreateStreamOnHGlobal/SHCreateStreamOnFile



#endif /* UPRIGHT_LZW_H */

