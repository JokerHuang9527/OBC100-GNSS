/*
 * task_jpeg.h
 *
 *  Created on: 2019¦~8¤ë27¤é
 *      Author: kusoyao
 */

#ifndef INC_TASK_JPEG_H_
#define INC_TASK_JPEG_H_

extern SemaphoreHandle_t xJPEGxFinshSemaphore;

void vTask_jpeg(void *param);

#endif /* INC_TASK_JPEG_H_ */
