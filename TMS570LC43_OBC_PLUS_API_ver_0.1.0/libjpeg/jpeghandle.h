
#ifndef jPEG_HANDLE_H
#define JPEG_HANDLE_H

#include <libjpeg/libjpeg/jinclude.h>
#include <libjpeg/libjpeg/jpeglib.h>
#include "manual_alloc.h"

#define JPEG_USE_SRAM_MEMORY 1

typedef struct{
    uint8_t isCompressing;
    uint8_t* dst;
    uint8_t quality;
    uint32_t row_stride;
    uint32_t size;
    jpeg_compress_struct cinfo;
}jpeg_info_struct;

extern jpeg_info_struct jpegInfo;
extern jpeg_info_struct smallJpegInfo;
extern ManualAllocStruct jpegAlloc;

// Init JPEG compress, return a JPEG compress info object, and you will get the compressed size in the end.
void ImageCompressInit(jpeg_compress_struct* cinfo, uint8_t* dst, uint32_t* size, uint32_t width, uint32_t height, uint8_t quality);
// Check if compress was completed.
uint32_t IsImageCompressCompleted(jpeg_compress_struct* cinfo);
// Compress many lines, can compress image data from different source.
void ImageCompressByLines(jpeg_compress_struct* cinfo, uint8_t * src, uint32_t lines);
// Compress whole image to JPEG image, return the compressed size.
uint32_t ImageCompress(jpeg_info_struct *info, uint8_t * src, uint8_t* dst, uint32_t dstSize, uint32_t width, uint32_t height, uint8_t quality);
// This part of code is for jpeg_info_struct
void InitJpegInfo(jpeg_info_struct *info, uint8_t* dst, uint32_t dstSize, uint32_t width, uint32_t height, uint8_t quality);
void CompressPartOfImage(jpeg_info_struct *info, uint8_t* src, uint32_t size, uint8_t* dst, uint32_t dstSize, uint32_t width, uint32_t height, uint8_t quality);

#endif /* CONSOLE_H */
