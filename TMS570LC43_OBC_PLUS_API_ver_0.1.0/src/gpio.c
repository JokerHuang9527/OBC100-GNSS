/*
 * gpio.c
 *
 *  Created on: 2024¦~2¤ë2¤é
 *      Author: user
 */

#include "xioctl.h"

io_gpio_devices_handle io_gpio_devices_handle_table[] = {
    {gioPORTA, 7, IO_GPIO_PRSET_GENERIC},//CN14 P46
    {gioPORTB, 7, IO_GPIO_PRSET_GENERIC},//CN14 P45
    {hetPORT2, 15, IO_GPIO_PRSET_OUTPUT}, //CN18 P1, On the circuit diagram it is OUTPUT.
    {hetPORT2, 16, IO_GPIO_PRSET_OUTPUT}, //CN18 P2, On the circuit diagram it is OUTPUT.
    {hetPORT2, 17, IO_GPIO_PRSET_INPUT}, //CN18 P4, On the circuit diagram it is INPUT.
    {hetPORT2, 18, IO_GPIO_PRSET_INPUT}, //CN18 P5, On the circuit diagram it is INPUT.
    {hetPORT2, 11, IO_GPIO_PRSET_OUTPUT}, //CN15 P16, On the circuit diagram it is OUTPUT.
    {hetPORT2, 10, IO_GPIO_PRSET_INPUT}, //CN15 P13, On the circuit diagram it is INPUT.
    {hetPORT2, 9, IO_GPIO_PRSET_GENERIC},};// CN15 P51

void gpio_info(){
    int i;
    io_gpio_devices_handle *handle;
    printk_ni("\n=======GPIO INFOMATION======\n");
    printk_ni("Index Input/Output High/Low Drain PULL  PULLDIR\n");
    for(i = 0; i < 9; i++){
        handle = (io_gpio_devices_handle *)(gpio_open(i));
        printk_ni("%02d   ", i+1);
        if((handle->gpio->DIR >> handle->pinbit) & (uint32)1U)
            printk_ni("   Output     ");
        else
            printk_ni("   Input      ");
        if((handle->gpio->DSET >> handle->pinbit) & (uint32)1U)
            printk_ni("  High   ");
        else
            printk_ni("  Low    ");
        if((handle->gpio->PDR >> handle->pinbit) & (uint32)1U)
            printk_ni("Open  ");
        else
            printk_ni("Close ");
        if((handle->gpio->PULDIS >> handle->pinbit) & (uint32)1U)
            printk_ni("Close ");
        else
            printk_ni("Open  ");
        if((handle->gpio->PSL >> handle->pinbit) & (uint32)1U)
            printk_ni("   Up  \n");
        else
            printk_ni("  Down \n");
    }
}
/*The following sets the proper direction for all GPIO*/
void gpio_init(){
    int i;
    io_gpio_devices_handle *handle;
    for(i = 0; i < 9; i++){
        handle = (io_gpio_devices_handle *)(gpio_open(i));
        gpio_ioctl(i, GPIO_CTL_INPUT_PULL,  IO_GPIO_INPUT_PULL_CLOSE, NULL);
        if(handle->ioflag == IO_GPIO_PRSET_INPUT){
            gpio_ioctl(i, GPIO_CTL_DIR_MODE,  IO_GPIO_DIR_INPUT, NULL);
        }
        else{
            gpio_output(i, IO_GPIO_LOW);
            gpio_ioctl(i, GPIO_CTL_OUTPUT_DRAIN,  IO_GPIO_OUTPUT_DRAIN_CLOSE, NULL);
        }
    }
//    for(i = 2;i < 6; i++)
//        gpio_ioctl(i, GPIO_CTL_PULL_MODE,  IO_GPIO_PULL_DIR_UP, NULL);
}

int gpio_output(int index, int status){

    io_gpio_devices_handle *handle = (io_gpio_devices_handle *)(gpio_open(index));

    if (handle == NULL)
        return E_INVALID_INPUT;

//    if(handle->ioflag == IO_GPIO_PRSET_INPUT)
//        return E_NOT_SUPPORT;

    gpio_ioctl(index, GPIO_CTL_DIR_MODE,  IO_GPIO_DIR_OUTPUT, NULL);
    if(status == IO_GPIO_LOW)
        gioSetBit(handle->gpio, handle->pinbit, IO_GPIO_LOW);
    else if(status == IO_GPIO_HIGH)
        gioSetBit(handle->gpio, handle->pinbit, IO_GPIO_HIGH);
    else
        return E_INVALID_INPUT;

    return E_SUCCESS;
}

int gpio_input(int index){
    int i;
    io_gpio_devices_handle *handle = (io_gpio_devices_handle *)(gpio_open(index));
    if (handle == NULL)
        return E_INVALID_INPUT;

//    if(handle->ioflag == IO_GPIO_PRSET_OUTPUT)
//        return E_NOT_SUPPORT;

    gpio_ioctl(index, GPIO_CTL_DIR_MODE,  IO_GPIO_DIR_INPUT, NULL);

    printk_ni(""); //need a delaya
    return gioGetBit(handle->gpio, handle->pinbit);

//    return E_SUCCESS;
}

void *gpio_open(int index)
{
    if((index >= 0) && (index <= 8)){
        io_gpio_devices_handle* handle;
        handle = &io_gpio_devices_handle_table[index];
        return (void *)handle;
    }
    else
        return NULL;
}

int gpio_ioctl(void *param1, void *param2, void *param3, void *param4)
{
    int index = (int)param1;
    io_gpio_devices_handle *handle = (io_gpio_devices_handle *)(gpio_open(index));
    if (handle == NULL)
        return E_INVALID_INPUT;

    int ctrl_type = (io_ctrl_t) param2;

    switch (ctrl_type){

    case GPIO_CTL_DIR_MODE:
    {
        int flag = (int)param3;
        uint32 mask;
        mask = (uint32)1U << handle->pinbit;
        if (flag == IO_GPIO_DIR_INPUT)
            handle->gpio->DIR &= ~(mask);
        else if(flag == IO_GPIO_DIR_OUTPUT)
            handle->gpio->DIR |= mask;
        else
            return E_INVALID_INPUT;
        return E_SUCCESS;

    }

    case GPIO_CTL_OUTPUT_DRAIN:
    {
        int flag = (int)param3;

        if (flag == IO_GPIO_OUTPUT_DRAIN_CLOSE)
            handle->gpio->PDR &= ~((uint32)1U << handle->pinbit);
        else if(flag == IO_GPIO_OUTPUT_DRAIN_OPEN)
            handle->gpio->PDR |= (uint32)1U << handle->pinbit;
        else
            return E_INVALID_INPUT;

        return E_SUCCESS;
    }

    case GPIO_CTL_INPUT_PULL:
    {
        int flag = (int)param3;

        if (flag == IO_GPIO_INPUT_PULL_CLOSE)
            handle->gpio->PULDIS |= (uint32)1U << handle->pinbit;
        else if(flag == IO_GPIO_INPUT_PULL_OPEN)
            handle->gpio->PULDIS &= ~((uint32)1U << handle->pinbit);
        else
            return E_INVALID_INPUT;

        return E_SUCCESS;
    }

    case GPIO_CTL_PULL_MODE:
    {
        int flag = (int)param3;

        if (flag == IO_GPIO_PULL_DIR_UP)
            handle->gpio->PSL |= (uint32)1U << handle->pinbit;
        else if(flag == IO_GPIO_PULL_DIR_DOWN){
//            if(15 <= handle->pinbit || 18 <= handle->pinbit)
//                return E_NOT_SUPPORT;
            handle->gpio->PSL &= ~((uint32)1U << handle->pinbit);
        }
        else
            return E_INVALID_INPUT;

        return E_SUCCESS;
    }

    default:
    {
        return(E_NOT_SUPPORT);
    }

    }
}
