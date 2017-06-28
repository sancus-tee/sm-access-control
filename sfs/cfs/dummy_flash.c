/*
 * a dummy flash driver implementation to have I/O independent measurements
 *
 * \author Jo Van Bulck
 *
 */
#include "../../common.h"
#include "flash_driver.h"

void sf_read_id(uint8_t *buf, int buf_size)
{
    return;
}

void sf_sector_erase(unsigned long addr_in_sector)
{
    return;
}

int sf_read(unsigned long start_addr, char *buf, unsigned int size)
{
    return size;
}

int sf_program_page(unsigned long start_addr, char *buf, unsigned int size)
{
    return size;
}
