/**
 * a driver implementation for the  ST M25P16 flash disk
 */
#include "../../../common.h"
#include <sancus_support/spi.h>
#include "flash_driver.h"

#define FD CYAN "\t[flash-driver] " NONE
#define printd(str) printdebug(FD str)

// ST M25P16 command set codes
#define SPI_FLASH_WREN          0x06
#define SPI_FLASH_WRDI          0x04
#define SPI_FLASH_RDID          0x9f
#define SPI_FLASH_RDSR          0x05
#define SPI_FLASH_WRSR          0x01
#define SPI_FLASH_READ          0x03
#define SPI_FLASH_RDHS          0x0b
#define SPI_FLASH_PP            0x02
#define SPI_FLASH_SE            0xd8
#define SPI_FLASH_BE            0xc7
#define SPI_FLASH_DP            0xb9
#define SPI_FLASH_RES           0xab

// status register bit masks
#define STATUS_WIP_MASK         0x01

void sf_read_id(uint8_t *buf, int buf_size)
{
#ifndef FLASH_NODEBUG
    printdebug_int(FD "read id into buf with size %d\n", buf_size);
#endif

    spi_select();
    spi_write_byte(SPI_FLASH_RDID);
    spi_read(buf, buf_size);
    spi_deselect();

    //print_buf("SF ID", buf, sizeof(buf));
}

uint8_t sf_read_status()
{
    spi_select();
    spi_write_byte(SPI_FLASH_RDSR);
    uint8_t b = spi_read_byte();
    spi_deselect();
    return b;
}

#define sf_write_enable() \
    do { \
        spi_select(); \
        spi_write_byte(SPI_FLASH_WREN); \
        spi_deselect(); \
    } while(0)

#define sf_write_disable() \
    do { \
        spi_select(); \
        spi_write_byte(SPI_FLASH_WRDI); \
        spi_deselect(); \
    } while(0)

#define BLOCK_WAITING() \
    do { \
        uint8_t cur_status = sf_read_status(); \
        /*printdebug_int("reading status for busy waiting: 0x%x\n",cur_status);  \
        printdebug("Now busy waiting on WIP bit...");*/  \
        while ((cur_status & STATUS_WIP_MASK) == STATUS_WIP_MASK)  \
            cur_status = sf_read_status();  \
        /*printdebug_int("status WIP bit cleared now; status is %x\n", cur_status);*/ \
    } while(0)

void sf_bulk_erase()
{
    sf_write_enable();
    spi_select();
    spi_write_byte(SPI_FLASH_BE);
    spi_deselect();
    BLOCK_WAITING();
}

void sf_sector_erase(unsigned long addr_in_sector)
{
    sf_write_enable();
    spi_select();
    spi_write_byte(SPI_FLASH_SE);
    
    // three address bytes on serial data input (DQ0). Any address inside
    // the sector is a valid address for the SECTOR ERASE command.
    spi_write_byte(addr_in_sector >> 16);
    spi_write_byte(addr_in_sector >> 8);
    spi_write_byte(addr_in_sector >> 0);
    
    spi_deselect();
    BLOCK_WAITING();
}


int sf_read(unsigned long start_addr, char *buf, unsigned int size)
{
#ifndef FLASH_NODEBUG
    printdebug_int(FD "reading %d bytes ", size);
    printdebug_long("from addr %lu\n", start_addr);
#endif

    spi_select();
    spi_write_byte(SPI_FLASH_READ);
    
    // write the 3 start address bytes (MSB to LSD)
    spi_write_byte(start_addr >> 16);
    spi_write_byte(start_addr >> 8);
    spi_write_byte(start_addr >> 0);
    
    char *cur;
    for (cur = buf; cur < buf+size; cur++) {
        char b = spi_read_byte();
        *cur = ~b;
    }

    spi_deselect();
    return size;
}

int sf_read_hs(unsigned long start_addr, uint8_t *buf, unsigned int size)
{
    spi_select();
    spi_write_byte(SPI_FLASH_RDHS);
    
    // write the 3 start address bytes (MSB to LSD)
    spi_write_byte(start_addr >> 16);
    spi_write_byte(start_addr >> 8);
    spi_write_byte(start_addr >> 0);
    
    //write a dummy byte
    spi_write_byte(0);
    
    uint8_t *cur;
    for (cur = buf; cur < buf+size; cur++) {
        uint8_t b = spi_read_byte();
        *cur = ~b; //XXX inverted
    }

    spi_deselect();
    return size;
}

char sf_read_byte(unsigned long addr)
{
    spi_select();
    spi_write_byte(SPI_FLASH_READ);
    
    // write the 3 start address bytes (MSB to LSD)
    spi_write_byte(addr >> 16);
    spi_write_byte(addr >> 8);
    spi_write_byte(addr >> 0);
    
    char rv = ~spi_read_byte();

    spi_deselect();
    return rv;
}

int sf_program_page(unsigned long start_addr, char *buf, unsigned int size)
{
#ifndef FLASH_NODEBUG
    printdebug_int(FD "writing %d bytes", size);
    printdebug_long(" to addr %lu\n", start_addr);
    printdebug("\t=== begin given buf dump ===");
    printdebug_str("\t");
    printdebug_buf(buf, size); 
    printdebug("\n\t=== end given buf dump ===");
#endif

    if (size < 1)
        return 0;
    
    sf_write_enable();
    spi_select();
    spi_write_byte(SPI_FLASH_PP);
    
    // write the 3 start address bytes (MSB to LSD)
    spi_write_byte(start_addr >> 16);
    spi_write_byte(start_addr >> 8);
    spi_write_byte(start_addr >> 0);
    
    // write all (>= 1) data bytes
    char *cur;
    for (cur = buf; cur < buf+size; cur++) {
        spi_write_byte(~*cur); // XXX inverted
    }
    
    spi_deselect();
    return size;
}

int sf_program_byte(unsigned long addr, uint8_t b)
{
    sf_write_enable();
    spi_select();
    spi_write_byte(SPI_FLASH_PP);
    
    // write the 3 start address bytes (MSB to LSD)
    spi_write_byte(addr >> 16);
    spi_write_byte(addr >> 8);
    spi_write_byte(addr >> 0);
    
    spi_write_byte(~b);
    spi_deselect();
    return 1;
}
