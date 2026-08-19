
#include "xioctl.h"

int spictl_config_slave(void *param1, void *param2, void *param3, void *param4);

ioctl_func_t common_ioctl_funcs []= {};
ioctl_func_t uart_ioctl_funcs []= {uart_ioctl};
ioctl_func_t i2c_ioctl_funcs []= {i2c_ioctl};
ioctl_func_t spi_ioctl_funcs []= {spi_ioctl};
ioctl_func_t gpio_ioctl_funcs []= {gpio_ioctl};

class_func_t func_class_tbls[] = {
	{sizeof(common_ioctl_funcs)/sizeof(ioctl_func_t), common_ioctl_funcs},
	{sizeof(uart_ioctl_funcs)/sizeof(ioctl_func_t), uart_ioctl_funcs},
	{sizeof(i2c_ioctl_funcs)/sizeof(ioctl_func_t), i2c_ioctl_funcs},
	{sizeof(spi_ioctl_funcs)/sizeof(ioctl_func_t), spi_ioctl_funcs},
	{sizeof(gpio_ioctl_funcs)/sizeof(ioctl_func_t), gpio_ioctl_funcs},};

int spictl_config_slave(void *param1, void *param2, void *param3, void *param4)
{
//	mss_spi_slave_t spi_slave = (mss_spi_slave_t) param1;
//	mss_spi_protocol_mode_t mode= (mss_spi_protocol_mode_t) param2;
//	uint32_t clk_div = (uint32_t) param3;
//	uint8_t frame_bit_length= (uint8_t) param4;
//
//	printk("I==%x, %x, %x, %x==\n", spi_slave, mode, clk_div, frame_bit_length);
	return 0;
}


int xioctl(uint16_t ctrl_num, void *param1, void *param2, void *param3, void *param4)
{
	int io_class = (ctrl_num & 0xF000) >> 12;
	int ctrl_idx = (ctrl_num & 0x0FFF);
	ioctl_func_t * class_tbl = func_class_tbls[io_class].class_funcs;
	int num_funcs = func_class_tbls[io_class].num_of_funcs;
	printk("io class=%d, ctrl_idx = %d %d \n", io_class, ctrl_idx, num_funcs);
	if( (num_funcs == 0) || ctrl_idx > (num_funcs-1))
	{
		printk("Fatal: out of IO_CTL controls\n");
		return 1;
	}
	return class_tbl[ctrl_idx](param1, param2, param3, param4);
}

void xioresponse(int error){
    switch(error){
        case E_SUCCESS:
            printk("E_SUCCESS\n");
            break;
        case E_INVALID_INPUT:
            printk("E_INVALID_INPUT\n");
            break;
        case E_BUSY:
            printk("E_BUSY\n");
            break;
        case E_TIMEOUT:
            printk("E_TIMEOUT\n");
            break;
        case E_NOT_SUPPORT:
            printk("E_NOT_SUPPORT\n");
            break;
        default:
        {
            return;
        }
    }
}
