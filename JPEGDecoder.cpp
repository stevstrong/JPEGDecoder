/*
JPEGDecoder.cpp

JPEG Decoder for Arduino
https://github.com/MakotoKurauchi/JPEGDecoder
Public domain, Makoto Kurauchi <http://yushakobo.jp>

Latest version here:
https://github.com/Bodmer/JPEGDecoder

Bodmer (21/6/15): Adapted by Bodmer to display JPEGs on TFT (works with Mega and Due) but there
is a memory leak somewhere, crashes after decoding 1 file :-)

Bodmer (29/1/16): Now in a state with sufficient Mega and Due testing to release in the wild

Bodmer (various): Various updates and latent bugs fixed

Bodmer (14/1/17): Tried to merge ESP8266 and SPIFFS support from Frederic Plante's broken branch,
				worked on ESP8266, but broke the array handling :-(

Bodmer (14/1/17): Scrapped all FP's updates, extended the built-in approach to using different
				data sources (currently array, SD files and/or SPIFFS files)

Bodmer (14/1/17): Added ESP8266 support and SPIFFS as a source, added configuration option to
				swap bytes to support fast image transfer to TFT using ESP8266 SPI writePattern().

Bodmer (15/1/17): Now supports ad hoc use of SPIFFS, SD and arrays without manual configuration.

Bodmer (19/1/17): Add support for filename being String type

Bodmer (20/1/17): Correct last mcu block corruption (thanks stevstrong for tracking that bug down!)

Bodmer (20/1/17): Prevent deleting the pImage pointer twice (causes an exception on ESP8266),
				tidy up code.

Bodmer (24/1/17): Correct greyscale images, update examples
*/

#include "JPEGDecoder.h"
#include "picojpeg.h"

#pragma GCC optimize "-O3"

JPEGDecoder JpegDec;

JPEGDecoder::JPEGDecoder()
{
	abort();
}


JPEGDecoder::~JPEGDecoder()
{
	abort();
}


int JPEGDecoder::pjpeg_callback(uint8_t* pBuf, uint8_t buf_size, uint8_t *pBytes_actually_read)
{
	JpegDec.pjpeg_need_bytes_callback(pBuf, buf_size, pBytes_actually_read);
	return 0;
}


int JPEGDecoder::pjpeg_need_bytes_callback(uint8_t* pBuf, uint8_t buf_size, uint8_t *pBytes_actually_read)
{
	uint n = jpg_min(g_nInFileSize - g_nInFileOfs, buf_size);

	if (jpg_source == JPEG_ARRAY) { // We are handling an array
		for (int i = 0; i < n; i++) {
			pBuf[i] = pgm_read_byte(jpg_data++);
			//Serial.println(pBuf[i],HEX);
		}
	}

#ifdef LOAD_SPIFFS
	if (jpg_source == JPEG_FS_FILE) g_pInFileFs.read(pBuf,n); // else we are handling a file
#endif

#ifdef LOAD_SD_LIBRARY
	if (jpg_source == JPEG_SD_FILE) g_pInFileSd.read(pBuf,n); // else we are handling a file
#endif

	*pBytes_actually_read = (uint8_t)(n);
	g_nInFileOfs += n;
}

int JPEGDecoder::decode_mcu(void)
{
	int status = pjpeg_decode_mcu();

	if (status) {
		is_available = 0 ;

		if (status != PJPG_NO_MORE_BLOCKS) {
			#ifdef DEBUG
			Serial.print("pjpeg_decode_mcu() failed with status ");
			Serial.println(status);
			#endif

			return -1;
		}
	}
	return 1;
}


int JPEGDecoder::read(void)
{
	int y, x;
	uint16_t *pDst_row;

	if(is_available == 0 || mcu_y >= image_info.m_MCUSPerCol) {
		abort();
		return 0;
	}

	// Copy MCU's pixel blocks into the destination bitmap.
	pDst_row = pImage;
	for (y = 0; y < image_info.m_MCUHeight; y += 8) {

		const int by_limit = jpg_min(8, image_info.m_height - (mcu_y * image_info.m_MCUHeight + y));

		for (x = 0; x < image_info.m_MCUWidth; x += 8) {
			uint16_t *pDst_block = pDst_row + x;

			// Compute source byte offset of the block in the decoder's MCU buffer.
			uint src_ofs = (x * 8U) + (y * 16U);
			const uint8_t *pSrcR = image_info.m_pMCUBufR + src_ofs;
			const uint8_t *pSrcG = image_info.m_pMCUBufG + src_ofs;
			const uint8_t *pSrcB = image_info.m_pMCUBufB + src_ofs;

			const int bx_limit = jpg_min(8, image_info.m_width - (mcu_x * image_info.m_MCUWidth + x));

			if (image_info.m_scanType == PJPG_GRAYSCALE) {
				int bx, by;
				for (by = 0; by < by_limit; by++) {
					uint16_t *pDst = pDst_block;

					for (bx = 0; bx < bx_limit; bx++) {
#ifdef SWAP_BYTES
						*pDst++ = (*pSrcR & 0xF8) | (*pSrcR & 0xE0) >> 5 | (*pSrcR & 0xF8) << 5 | (*pSrcR & 0x1C) << 11;
#else
						*pDst++ = (*pSrcR & 0xF8) << 8 | (*pSrcR & 0xFC) <<3 | *pSrcR >> 3;
#endif
						pSrcR++;
					}
					pSrcR += (8 - bx_limit);
					pDst_block += MCUWidth;
				}
			}
			else {
				int bx, by;
				for (by = 0; by < by_limit; by++) {
					uint16_t *pDst = pDst_block;

					for (bx = 0; bx < bx_limit; bx++) {
#ifdef SWAP_BYTES
						*pDst++ = (*pSrcR & 0xF8) | (*pSrcG & 0xE0) >> 5 | (*pSrcB & 0xF8) << 5 | (*pSrcG & 0x1C) << 11;
#else
						*pDst++ = (*pSrcR & 0xF8) << 8 | (*pSrcG & 0xFC) <<3 | *pSrcB >> 3;
#endif
						pSrcR++; pSrcG++; pSrcB++;
					}

					pSrcR += (8 - bx_limit);
					pSrcG += (8 - bx_limit);
					pSrcB += (8 - bx_limit);

					pDst_block += MCUWidth;
				}
			}
		}
		pDst_row += (MCUWidth * 8);
	}

	MCUx = mcu_x;
	MCUy = mcu_y;

	mcu_x++;
	if (mcu_x == image_info.m_MCUSPerRow) {
		mcu_x = 0;
		mcu_y++;
	}

	if(decode_mcu()==-1) is_available = 0 ;

	return 1;
}
/*
int JPEGDecoder::readSwappedBytes(void)
{
	int y, x;
	uint16_t *pDst_row;

	if(is_available == 0 || mcu_y >= image_info.m_MCUSPerCol) {
		abort();
		return 0;
	}
	
	// Copy MCU's pixel blocks into the destination bitmap.
	pDst_row = pImage;
	for (y = 0; y < image_info.m_MCUHeight; y += 8) {

		const int by_limit = jpg_min(8, image_info.m_height - (mcu_y * image_info.m_MCUHeight + y));

		for (x = 0; x < image_info.m_MCUWidth; x += 8) {
			uint16_t *pDst_block = pDst_row + x;

			// Compute source byte offset of the block in the decoder's MCU buffer.
			uint src_ofs = (x * 8U) + (y * 16U);
			const uint8_t *pSrcR = image_info.m_pMCUBufR + src_ofs;
			const uint8_t *pSrcG = image_info.m_pMCUBufG + src_ofs;
			const uint8_t *pSrcB = image_info.m_pMCUBufB + src_ofs;

			const int bx_limit = jpg_min(8, image_info.m_width - (mcu_x * image_info.m_MCUWidth + x));

			if (image_info.m_scanType == PJPG_GRAYSCALE) {
				int bx, by;
				for (by = 0; by < by_limit; by++) {
					uint16_t *pDst = pDst_block;

					for (bx = 0; bx < bx_limit; bx++) {

						*pDst++ = (*pSrcR & 0xF8) | (*pSrcR & 0xE0) >> 5 | (*pSrcR & 0xF8) << 5 | (*pSrcR & 0x1C) << 11;

						pSrcR++;
					}
					pSrcR += (8 - bx_limit);
					pDst_block += MCUWidth;
				}
			}
			else {
				int bx, by;
				for (by = 0; by < by_limit; by++) {
					uint16_t *pDst = pDst_block;

					for (bx = 0; bx < bx_limit; bx++) {

						*pDst++ = (*pSrcR & 0xF8) | (*pSrcG & 0xE0) >> 5 | (*pSrcB & 0xF8) << 5 | (*pSrcG & 0x1C) << 11;

						pSrcR++; pSrcG++; pSrcB++;
					}

					pSrcR += (8 - bx_limit);
					pSrcG += (8 - bx_limit);
					pSrcB += (8 - bx_limit);

					pDst_block += MCUWidth;
				}
			}
		}
		pDst_row += (MCUWidth * 8);
	}

	MCUx = mcu_x;
	MCUy = mcu_y;

	mcu_x++;
	if (mcu_x == image_info.m_MCUSPerRow) {
		mcu_x = 0;
		mcu_y++;
	}

	if(decode_mcu()==-1) is_available = 0 ;

	return 1;
}
*/

#ifdef LOAD_SD_LIBRARY
// Call specific to SD filing system in case leading / is used
int JPEGDecoder::decodeSdFile(File jpgFile) // This is for the SD library
{
	if (!jpgFile) {
		#ifdef DEBUG
		Serial.println("ERROR: SD file not found!");
		#endif

		return -1;
	}

	g_pInFileSd = jpgFile;

	g_nInFileSize = g_pInFileSd.size();

	g_nInFileOfs = 0;

	jpg_source = JPEG_SD_FILE; // Flag to indicate a SD file

	return decodeCommon();

}
#endif


int JPEGDecoder::decodeArray(const uint8_t array[], uint32_t  array_size)
{
	jpg_data = (uint8_t *)array;

	g_nInFileSize = array_size;

	g_nInFileOfs = 0;

	jpg_source = JPEG_ARRAY; // We are not processing a file, use arrays

	return decodeCommon();
}

static uint16_t img[16*16];

int JPEGDecoder::decodeCommon(void)
{
	int status = pjpeg_decode_init(&image_info, pjpeg_callback, 0);

	if (status) {
		#ifdef DEBUG
		Serial.print("pjpeg_decode_init() failed with status ");
		Serial.println(status);

		if (status == PJPG_UNSUPPORTED_MODE) {
			Serial.println("Progressive JPEG files are not supported.");
		}
		#endif

		return -1;
	}

	pImage = img;

	is_available = 1 ;

	width =  image_info.m_width;
	height =  image_info.m_height;
	MCUSPerRow = image_info.m_MCUSPerRow;
	MCUSPerCol = image_info.m_MCUSPerCol;
	scanType = image_info.m_scanType;
	MCUWidth = image_info.m_MCUWidth;
	MCUHeight = image_info.m_MCUHeight;

	return decode_mcu();
}

void JPEGDecoder::abort(void)
{
	mcu_x = 0 ;
	mcu_y = 0 ;
	is_available = 0;
	jpg_source = 0;
	pImage = NULL;
	
#ifdef LOAD_SD_LIBRARY
	if (jpg_source == JPEG_SD_FILE) if (g_pInFileSd) g_pInFileSd.close();
#endif
}
