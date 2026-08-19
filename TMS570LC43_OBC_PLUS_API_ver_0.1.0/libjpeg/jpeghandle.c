
#include <global.h>
#include "jpeghandle.h"
#include <libjpeg/libjpeg/jinclude.h>
#include <libjpeg/libjpeg/jpeglib.h>

jpeg_info_struct jpegInfo __SRAM2_SECTION__;
jpeg_info_struct smallJpegInfo __SRAM2_SECTION__;
ManualAllocStruct jpegAlloc __SRAM2_SECTION__;

void ImageCompressInit(jpeg_compress_struct* cinfo, uint8_t* dst, uint32_t * size, uint32_t width, uint32_t height, uint8_t quality)
{
    // use this for vPortFree() & pvPortMalloc()
    BaseType_t xRunningPrivileged = prvRaisePrivilege();
    jpeg_error_mgr_t jerr;

    cinfo->err = jpeg_std_error(&jerr);

    jpeg_create_compress(cinfo);

    jpeg_mem_dest(cinfo, &dst, (unsigned long *)size);
    cinfo->image_width = width; 	/* image width and height, in pixels */
    cinfo->image_height = height;
    cinfo->input_components = 3;		/* # of color components per pixel */
    cinfo->in_color_space = JCS_RGB; 	/* colorspace of input image */
    cinfo->next_scanline = 0;

    jpeg_set_defaults(cinfo);

    jpeg_set_quality(cinfo, quality, TRUE /* limit to baseline-JPEG values */);

    jpeg_start_compress(cinfo, TRUE);
}

unsigned int IsImageCompressCompleted(jpeg_compress_struct* cinfo)
{
    return (cinfo->next_scanline == cinfo->image_height);
}

void ImageCompressByLines(jpeg_compress_struct *cinfo, uint8_t *src, uint32_t lines)
{
    uint32_t index;
    JSAMPROW row_pointer;
    uint32_t row_stride = cinfo->image_width * 3;	/* JSAMPLEs per row in image_buffer */
    for (index = 0; index < lines; index++)
    {
        if (!IsImageCompressCompleted(cinfo))
        {
            row_pointer = (JSAMPROW)&src[index * row_stride];
            jpeg_write_scanlines(cinfo, &row_pointer, 1);
        }
        vTaskDelay(10);
    }
}

uint32_t ImageCompress(jpeg_info_struct *info, uint8_t* src, uint8_t* dst, uint32_t dstSize, uint32_t width, uint32_t height, uint8_t quality)
{
    //uint32_t size = 0;
    printk("## Start JPEG Compress\n");
    InitJpegInfo(info, dst, dstSize, width, height, quality);
    while (!IsImageCompressCompleted(&info->cinfo)) {
        // pass the head ptr of image line to function
        ImageCompressByLines(&info->cinfo, &src[info->cinfo.next_scanline * info->row_stride], 1);
    }
    jpeg_finish_compress(&info->cinfo);
    jpeg_destroy_compress(&info->cinfo);
    printk("[ImageCompress]JPEG compress end, width = %u, height = %u, quality = %u, size = %u\n", info->cinfo.image_width, info->cinfo.image_height, info->quality, info->size);

    return info->size;
}

void InitJpegInfo(jpeg_info_struct *info, uint8_t* dst, uint32_t dstSize, uint32_t width, uint32_t height, uint8_t quality)
{
    memset(dst, 0, sizeof(dstSize));
    memset(info, 0, sizeof(jpeg_info_struct));

    info->dst = dst;
    info->size = dstSize;
    info->row_stride = width * 3;
    info->quality = quality;

    ImageCompressInit(&info->cinfo, info->dst, &info->size, width, height, info->quality);
}

void CompressPartOfImage(jpeg_info_struct *info, uint8_t* src, uint32_t size, uint8_t* dst, uint32_t dstSize, uint32_t width, uint32_t height, uint8_t quality)
{
    if(!IsImageCompressCompleted(&info->cinfo))
    {
        ImageCompressByLines(&info->cinfo, src, size / info->row_stride);
    }
    else
    {
        InitJpegInfo(info, dst, dstSize, width, height, quality);
        ImageCompressByLines(&info->cinfo, src, size / info->row_stride);
    }

    if(IsImageCompressCompleted(&info->cinfo))
    {
//        BaseType_t xRunningPrivileged = prvRaisePrivilege();
        jpeg_finish_compress(&info->cinfo);
        jpeg_destroy_compress(&info->cinfo);
        printk("JPEG compress end, width = %u, height = %u, quality = %u, size = %u\n", info->cinfo.image_width, info->cinfo.image_height, info->quality, info->size);
        //jpeg_output_file_size=info->size;
        //printk("JPEG compress end, jpeg_output_file_size %d\n", jpeg_output_file_size);
    }
}
