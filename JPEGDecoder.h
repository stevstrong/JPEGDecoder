/*
JPEGDecoder.h

JPEG Decoder for Arduino
Public domain, Makoto Kurauchi <http://yushakobo.jp>

Adapted by Bodmer for use with a TFT screen

Latest version here:
https://github.com/Bodmer/JPEGDecoder

*/

#ifndef JPEGDECODER_H
#define JPEGDECODER_H

#include "Arduino.h"

#define LOAD_SD_LIBRARY 1

#ifdef LOAD_SD_LIBRARY
	#include <SdFat.h>
#endif

	
#include "picojpeg.h"

enum {
	JPEG_ARRAY = 1,
	JPEG_FS_FILE,
	JPEG_SD_FILE
};

//#define DEBUG

//------------------------------------------------------------------------------
#ifndef jpg_min
	#define jpg_min(a,b)     (((a) < (b)) ? (a) : (b))
#endif
//------------------------------------------------------------------------------
typedef unsigned char uint8;
typedef unsigned int uint;
//------------------------------------------------------------------------------

class JPEGDecoder
{
public:
	uint16_t *pImage;

	int16 width;
	int16 height;
	int16 MCUSPerRow;
	int16 MCUSPerCol;
	pjpeg_scan_type_t scanType;
	int16 MCUWidth;
	int16 MCUHeight;
	int16 MCUx;
	int16 MCUy;

	JPEGDecoder();
	~JPEGDecoder();

	int available(void);
	int read(void);
	int readSwappedBytes(void);

#ifdef LOAD_SD_LIBRARY
	int decodeSdFile (const char *pFilename);
	int decodeSdFile (const String& pFilename);
	int decodeSdFile (File g_pInFile);
#endif

	int decodeArray(const uint8_t array[], uint32_t  array_size);
	void abort(void);

private:
#ifdef LOAD_SD_LIBRARY
	File g_pInFileSd;
#endif
	pjpeg_scan_type_t scan_type;
	pjpeg_image_info_t image_info;

	int16 is_available;
	int16 mcu_x;
	int16 mcu_y;
	uint32 g_nInFileSize;
	uint16 g_nInFileOfs;
	uint16 jpg_source;

	uint8_t* jpg_data;
	
	static int pjpeg_callback(unsigned char* pBuf, unsigned char buf_size, unsigned char *pBytes_actually_read);
	int pjpeg_need_bytes_callback(unsigned char* pBuf, unsigned char buf_size, unsigned char *pBytes_actually_read);
	int decode_mcu(void);
	int decodeCommon(void);
};

extern JPEGDecoder JpegDec;

#endif // JPEGDECODER_H