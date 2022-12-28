#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "base64.h"

/* Messages */
#define HELP "Usage:  vorbispic INPUT [OUTPUT]\n"
#define ERROR_FORMAT "Error: invalid image format\n"
#define ERROR_OPEN   "Error: could not open %s\n"
#define ERROR_OTHER  "Error: unknown\n"

int main(int argc, char ** argv)
{
	typedef unsigned char byte;

	int status = EXIT_FAILURE;

	/* Check command line arguments */
	if (argc < 2 || argc > 3)
	{
		printf(HELP);
		goto FINISH;
	}

	/* Open input file */
	FILE * input = fopen(argv[1], "r");
	if (input == NULL)
	{
		fprintf(stderr, ERROR_OPEN, argv[1]);
		goto FINISH;
	}

	/* Open output file */
	FILE * output = argc == 3 ? fopen(argv[2], "a") : stdout;
	if (output == NULL)
	{
		fprintf(stderr, ERROR_OPEN, argv[2]);
		goto CLEAN_1;
	}

	/* Get image size */
	fseek(input, 0, SEEK_END);
	long image_size = ftell(input);
	if (image_size < 3)
	{
		fprintf(stderr, ERROR_OTHER);
		goto CLEAN_2;
	}
	fseek(input, 0, SEEK_SET);

	/* Allocate memory */
	const int HEADER_SIZE = 42;
	byte * input_buffer = calloc(1, image_size + HEADER_SIZE);
	if (input_buffer == NULL)
	{
		fprintf(stderr, ERROR_OTHER);
		goto CLEAN_2;
	}
	byte * header = input_buffer;
	byte * image = header + HEADER_SIZE;

	/* Read input file signature */
	size_t limit = image_size < 4096 ? image_size : 4096;
	size_t count = fread(image, 1, limit, input);			 
	if (count < limit)
	{
		fprintf(stderr, ERROR_OTHER);
		goto CLEAN_3;
	}

	/* Check file signature and build header */
	const byte SIGNATURE_JPG[] = { 0xFF, 0xD8, 0xFF };
	const byte SIGNATURE_PNG[] = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A };
	if (memcmp(image, SIGNATURE_JPG, 3) == 0)
	{
		header[7] = 10;
		strcpy((char *)&header[8], "image/jpeg");
		header[38] = (image_size >> 24) & 0xFF;
		header[39] = (image_size >> 16) & 0xFF;
		header[40] = (image_size >> 8) & 0xFF;
		header[41] = image_size & 0xFF;
	}
	else if (memcmp(image, SIGNATURE_PNG, 8) == 0)
	{
		header ++;
		header[7] = 9;
		strcpy((char *)&header[8], "image/png");
		header[37] = (image_size >> 24) & 0xFF;
		header[38] = (image_size >> 16) & 0xFF;
		header[39] = (image_size >> 8) & 0xFF;
		header[40] = image_size & 0xFF;
	}
	else
	{
		fprintf(stderr, ERROR_FORMAT);
		goto CLEAN_3;
	}
	header[3] = 3;

	/* Read the rest of the file */
	limit = image_size - count;
	count = fread(&image[count], 1, limit, input);
	if (count < limit)
	{
		fprintf(stderr, ERROR_OTHER);
		goto CLEAN_3;
	}

	/* Build tag */
	int value_size = image_size + HEADER_SIZE - (header - input_buffer);
	int encoded_value_size = Base64encode_len(value_size);
	const int KEY_SIZE = 23;
	size_t tag_size = KEY_SIZE + encoded_value_size;
	byte * tag = malloc(tag_size);
	if (tag == NULL)
	{
		fprintf(stderr, ERROR_OTHER);
		goto CLEAN_3;
	}
	strcpy((char *)tag, "METADATA_BLOCK_PICTURE=");
	Base64encode((char *)&tag[KEY_SIZE], (char *)header, value_size);

	/* Write tag to output */
	count = fwrite(tag, 1, tag_size, output);
	if (count < tag_size)
	{
		fprintf (stderr, ERROR_OTHER);
		goto CLEAN_4;
	}

	status = EXIT_SUCCESS;

	/* Free resources & finish */
	CLEAN_4:
	free(tag);

	CLEAN_3:
	free(input_buffer);

	CLEAN_2:
	fclose(output);

	CLEAN_1:
	fclose(input);

	FINISH:
	exit(status);
}
