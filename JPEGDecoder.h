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

#include "User_Config.h"

#include "Arduino.h"

#ifdef ESP8266

  #include "arduino.h"
  #include <pgmspace.h>
 
  #define LOAD_SPIFFS
  #define FS_NO_GLOBALS
  #include <FS.h>
  
  #ifdef LOAD_SD_LIBRARY
    #include <SD.h> 
  #endif
  
#else
	
  #ifdef LOAD_SD_LIBRARY
    #ifdef __AVR__
      #include <SD.h>    // For the Mega
    #else
      #include <SdFat.h> // For Due etc where we might need to bit bash the SPI
    #endif
  #endif
  
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

class JPEGDecoder {

private:
  #ifdef LOAD_SD_LIBRARY
    File g_pInFileSd;
  #endif
  #ifdef LOAD_SPIFFS // For future support to ESP32 etc
    fs::File g_pInFileFs;
  #endif
    pjpeg_scan_type_t scan_type;
    pjpeg_image_info_t image_info;
    
    int is_available;
    int mcu_x;
    int mcu_y;
    uint g_nInFileSize;
    uint g_nInFileOfs;
    uint8 jpg_source;
    uint8_t* jpg_data; 
    
    static uint8 pjpeg_callback(unsigned char* pBuf, unsigned char buf_size, unsigned char *pBytes_actually_read, void *pCallback_data);
    uint8 pjpeg_need_bytes_callback(unsigned char* pBuf, unsigned char buf_size, unsigned char *pBytes_actually_read, void *pCallback_data);
    int decode_mcu(void);
    int decodeCommon(void);
public:

    uint16_t *pImage;

    int width;
    int height;
    int MCUSPerRow;
    int MCUSPerCol;
    pjpeg_scan_type_t scanType;
    int MCUWidth;
    int MCUHeight;
    int MCUx;
    int MCUy;
    
    JPEGDecoder();
    ~JPEGDecoder();

    int available(void);
    int read(void);

	int decodeFile (const char *pFilename);
	int decodeFile (const String& pFilename);
  #ifdef LOAD_SD_LIBRARY
    int decodeSdFile (const char *pFilename);
    int decodeSdFile (const String& pFilename);
    int decodeSdFile (File g_pInFile);
  #endif
  #ifdef LOAD_SPIFFS
    int decodeFsFile (const char *pFilename);
    int decodeFsFile (const String& pFilename);
	int decodeFsFile (fs::File g_pInFile);
  #endif
    int decodeArray(const uint8_t array[], uint32_t  array_size);
    void abort(void);

};

extern JPEGDecoder JpegDec;

#endif // JPEGDECODER_H