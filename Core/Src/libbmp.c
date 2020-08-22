/*
 * libbmp.c
 *
 *  Created on: Aug 13, 2020
 *      Author: mehdi
 *      customized from:https://github.com/marc-q/libbmp
 */
#include "libbmp.h"
// BMP_HEADER
//void bmp_header_init_df(bmp_header *header, const int width, const int height) {
//	header->bfSize = (sizeof(bmp_pixel) * width + BMP_GET_PADDING(width))
//			* abs(height);
//	header->bfReserved = 0;
//	header->bfOffBits = 54;
//	header->biSize = 40;
//	header->biWidth = width;
//	header->biHeight = height;
//	header->biPlanes = 1;
//	header->biBitCount = 24;
//	header->biCompression = 0;
//	header->biSizeImage = 0;
//	header->biXPelsPerMeter = 0;
//	header->biYPelsPerMeter = 0;
//	header->biClrUsed = 0;
//	header->biClrImportant = 0;
//}
//
//enum bmp_error bmp_header_write(const bmp_header *header, FIL *img_file) {
//	UINT writtenbytes;
//	if (header == NULL) {
//		return BMP_HEADER_NOT_INITIALIZED;
//	} else if (img_file == NULL) {
//		return BMP_FILE_NOT_OPENED;
//	}
//
//	// Since an adress must be passed to fwrite, create a variable!
//	const unsigned short magic = BMP_MAGIC;
//	//fwrite(&magic, sizeof(magic), 1, img_file);
//	f_write(img_file,&magic,sizeof(magic),&writtenbytes);
//
//	// Use the type instead of the variable because its a pointer!
//	//fwrite(header, sizeof(bmp_header), 1, img_file);
//	f_write(img_file,header,sizeof(bmp_header),&writtenbytes);
//
//	return BMP_OK;
//}
//
enum bmp_error bmp_header_read(bmp_header *header, FIL *img_file) {
	UINT readbytes;
	if (img_file == NULL) {
		return BMP_FILE_NOT_OPENED;
	}

	// Since an adress must be passed to fread, create a variable!
	unsigned short magic;

	// Check if its an bmp file by comparing the magic nbr:
//	if (fread(&magic, sizeof(magic), 1, img_file) != 1 || magic != BMP_MAGIC) {
	if (f_read(img_file, &magic, sizeof(magic), &readbytes) != FR_OK || magic != BMP_MAGIC) {

		return BMP_INVALID_FILE;
	}
	//if (f_read(header, sizeof(bmp_header), 1, img_file) != 1) {
	if (f_read(img_file, header,sizeof(bmp_header), &readbytes) != FR_OK) {
		return BMP_ERROR;
	}

	return BMP_OK;
}

//// BMP_PIXEL
//
//void bmp_pixel_init(bmp_pixel *pxl, const unsigned char red,
//		const unsigned char green, const unsigned char blue) {
////	pxl->red = red;
////	pxl->green = green;
////	pxl->blue = blue;
//	pxl->bw=blue;
//}
//
//// BMP_IMG

void bmp_img_alloc(bmp_img *img) {
	const size_t h = abs(img->img_header.biHeight);
	const size_t w = abs(img->img_header.biWidth);

	// Allocate the required memory for the pixels:
	img->img_pixels = malloc(sizeof(unsigned char) *h*w);

//	for (size_t y = 0; y < h; y++) {
//		img->img_pixels[y] = malloc(
//				sizeof(bmp_pixel) * img->img_header.biWidth);
//	}
}

//void bmp_img_init_df(bmp_img *img, const int width, const int height) {
//	// INIT the header with default values:
//	bmp_header_init_df(&img->img_header, width, height);
//	bmp_img_alloc(img);
//}

void bmp_img_free(bmp_img *img) {
//	const size_t h = abs(img->img_header.biHeight);
//
//	for (size_t y = 0; y < h; y++) {
//		free(img->img_pixels[y]);
//	}
	free(img->img_pixels);
}

//enum bmp_error bmp_img_write(const bmp_img *img, const char *filename) {
//	UINT writtenbytes;
////	FILE *img_file = f_open(filename, "wb");
//	FIL img_file;
//
//	if (f_open(&img_file, (const TCHAR*) filename, FA_WRITE) != FR_OK) {
//		return BMP_FILE_NOT_OPENED;
//	}
//
//	// NOTE: This way the correct error code could be returned.
//	const enum bmp_error err = bmp_header_write(&img->img_header, &img_file);
//
//	if (err != BMP_OK) {
//		// ERROR: Could'nt write the header!
//		f_close(&img_file);
//		return err;
//	}
//
//	// Select the mode (bottom-up or top-down):
//	const size_t h = abs(img->img_header.biHeight);
//	const size_t offset = (img->img_header.biHeight > 0 ? h - 1 : 0);
//
//	// Create the padding:
//	const unsigned char padding[3] = { '\0', '\0', '\0' };
//
//	// Write the content:
//	for (size_t y = 0; y < h; y++) {
//		// Write a whole row of pixels to the file:
////		f_write(img->img_pixels[abs(offset - y)], sizeof(bmp_pixel),
////				img->img_header.biWidth, img_file);
//		f_write(&img_file,img->img_pixels[abs(offset - y)], sizeof(bmp_pixel)*img->img_header.biWidth,
//				writtenbytes );
//		// Write the padding for the row!
////		f_write(padding, sizeof(unsigned char),
////				BMP_GET_PADDING(img->img_header.biWidth), img_file);
//		f_write(&img_file,padding,sizeof(unsigned char)*BMP_GET_PADDING(img->img_header.biWidth),writtenbytes);
//	}
//
//	// NOTE: All good!
//	f_close(&img_file);
//	return BMP_OK;
//}

enum bmp_error bmp_img_read(bmp_img *img, const char *filename) {
	UINT readbytes;
	FIL img_file;
	if (f_open(&img_file, (const TCHAR*) filename, FA_READ) != FR_OK) {
		return BMP_FILE_NOT_OPENED;
	}
	// NOTE: This way the correct error code can be returned.
	const enum bmp_error err = bmp_header_read(&img->img_header, &img_file);
	////////////////////////////read color table///////////////////////////////
	unsigned char **biColorTable;
	biColorTable = malloc(sizeof(unsigned char)*img->img_header.biBitCount);

	for (size_t y = 0; y < img->img_header.biBitCount; y++) {
			biColorTable[y] = malloc(sizeof(unsigned char)*4);
			f_read(&img_file,biColorTable[y], sizeof(unsigned char)* 4,&readbytes);
			if ( readbytes!= 4) {
				f_close(&img_file);
				return BMP_ERROR;
			}
	}
	for (size_t y = 0; y < img->img_header.biBitCount; y++) {
			f_read(&img_file,biColorTable[y], sizeof(unsigned char)* 4,&readbytes);
			if ( readbytes!= 4) {
				f_close(&img_file);
				return BMP_ERROR;
			}
			free(biColorTable[y]);

	}

	if (err != BMP_OK) {
		// ERROR: Could'nt read the image header!
		f_close(&img_file);
		return err;
	}
	///////////////////////////////////////////////////////////
	bmp_img_alloc(img);

	// Select the mode (bottom-up or top-down):
	const size_t h = abs(img->img_header.biHeight);
	const size_t offset = (img->img_header.biHeight > 0 ? h - 1 : 0);
	const size_t padding = BMP_GET_PADDING(img->img_header.biWidth);

	// Needed to compare the return value of fread
	const size_t items = img->img_header.biWidth/8;

	// Read the content:
	for (size_t y = 0; y < h; y++) {
		// Read a whole row of pixels from the file:
		f_read(&img_file,&img->img_pixels[abs(offset - y)*items], sizeof(unsigned char)* items,&readbytes);
		if ( readbytes!= items) {
			f_close(&img_file);
			return BMP_ERROR;
		}

		// Skip the padding:
		//f_seek(img_file, padding, SEEK_CUR);
		f_lseek(&img_file, f_tell(&img_file)+padding);

	}

	// NOTE: All good!
	f_close(&img_file);
	return BMP_OK;
}
