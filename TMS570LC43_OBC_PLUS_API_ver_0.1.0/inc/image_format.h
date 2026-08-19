/*
 * image_format.h
 *
 *  Created on: 2023/03/16
 *      Author: smithKao
 */

#ifndef IMAGE_FORMAT_H_
#define IMAGE_FORMAT_H_

#include <stdint.h>

struct ImageInfoStruct
{
    uint32_t size;
    uint32_t timestamp; // captured time, remember to set timestamp before capture
    uint8_t flag;
    uint8_t mode;      // image mode while capturing
    uint8_t type;      // saved image type
    uint16_t sequence; // sequence to show whitch image captured recently (old > new)
    uint16_t index;    // index in memory
} __attribute__((packed));
typedef struct ImageInfoStruct ImageInfo;

typedef enum
{
    IMAGE_OPERATE_BY_INDEX = 0,
    IMAGE_OPERATE_BY_NEW,
    IMAGE_OPERATE_BY_OLD,
    IMAGE_OPERATE_ALL_IMAGE = 255,
} ImageOperateFlag;

typedef enum
{
    IMAGE_TYPE_NONE = 0,
    IMAGE_TYPE_RAW,
    IMAGE_TYPE_RGBA,
} ImageType;

typedef enum
{
    IMAGE_MODE_0 = 0,
    IMAGE_MODE_1,
    IMAGE_MODE_2,
    IMAGE_MODE_3,
    IMAGE_MODE_4,
} ImageMode;

typedef enum
{
    IMAGE_FLAG_TEST_PATTERN = (1U << 0),
    IMAGE_FLAG_AUTO_EXPOSURE = (1U << 1),
    IMAGE_FLAG_AUTO_WHITE_BALANCE = (1U << 2),
    IMAGE_FLAG_AI = (1U << 3),
    IMAGE_FLAG_LZO_COMPRESSED = (1U << 4),
    IMAGE_FLAG_BAD_BLOCK = (1U << 7),
} ImageFlag;

#endif /* IMAGE_FORMAT_H_ */
