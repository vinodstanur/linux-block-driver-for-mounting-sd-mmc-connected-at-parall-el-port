#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/unistd.h>
#include <linux/delay.h>
#include <asm/io.h>
#include "sd.h"
 
unsigned long int arg = 0;
char IS_SDHC;
unsigned long int read_card_size(void)
{
    int i,j,k;
    int count = 0;
    unsigned int C_SIZE = 0;
    unsigned int BLOCK_LEN = 0;
    unsigned long int BLOCKNR = 0;
    unsigned int MULT = 0;
    unsigned int C_SIZE_MULT = 0;
    unsigned int READ_BL_LEN = 0;
    unsigned char bf[200];
    unsigned char csd_byte = 0;
    if (command(9, 1,0xff) != 0)
    while(spi_read() != 0 && count++ <100);
    if(count < 90){
        count = 0;
        while(spi_read() != 0xfe && count++ <100);
        if(count >90) {printk(KERN_ALERT "NO TOCKEN!!!!\n"); return 0;}
        printk(KERN_ALERT "GOTTT TOCKEN!!!!\n");
        k = 127;
        for(i = 0; i < 16; i++) {
            csd_byte = spi_read();
            for(j = 0; j < 8; j++) {
                if(csd_byte & (1<<7))
                bf[k--] = 1;
                else
                bf[k--] = 0;
                csd_byte <<= 1;
            }
        }
        if(!IS_SDHC) {
            for(i = 0; i < 128; i++) {
                printk(KERN_ALERT "%d - %c", i, bf[i]);
            }
            for(i = 83; i>=80; i--) {
                READ_BL_LEN <<= 1;
                if(bf[i]) READ_BL_LEN++;
            }
            for(i = 73; i>=62; i--) {
                C_SIZE <<= 1;
                if(bf[i]) C_SIZE++;
            }
            for(i = 49; i>=47; i--) {
                C_SIZE_MULT <<= 1;
                if(bf[i]) C_SIZE_MULT++;
            }
            //printk("READ_BL_LEN = %d\n",READ_BL_LEN); //USED FOR TESTING WHILE CODING
            //printk("C_SIZE = %d\n",C_SIZE);
            //printk("C_SIZE_MULT = %d\n",C_SIZE_MULT);
            MULT = 1;
            for(i = 0; i < C_SIZE_MULT + 2; i++)
            MULT *= 2;
            BLOCKNR = (C_SIZE+1)*MULT;
            BLOCK_LEN = 1;
            for(i = 0; i<READ_BL_LEN; i++)
            BLOCK_LEN *= 2;
            //printk("MULT = %d\n",MULT); //USED FOR TESTING WHILE CODING
            //printk("BLOCK_LEN = %d\n",BLOCK_LEN);
            //printk("BLOCKNR = %d\n",BLOCKNR);
            return (BLOCKNR*BLOCK_LEN);
            } else {
            for(i = 69; i>=48; i--) {
                C_SIZE <<= 1;
                if(bf[i]) C_SIZE++;
            }
            return ( (((unsigned long int)C_SIZE +1)*512)*1024 );
        }
    }
    return 0;
}
 
 
int mmc_write_multiple_sector(unsigned long int sector, unsigned char *buffer, unsigned int nsect)
{
    int i, cnt;
    sector *= 512;
    if(command(25, sector, 0xff) != 0)
    while (spi_read() != 0 );
     
    while(nsect) {
        spi_write(0xff);
        spi_write(0xff);
        spi_write(0b11111100); //write token
        for(i = 0; i < 512; i++)
        spi_write(*buffer++);
        spi_write(0xff);
        spi_write(0xff);
        while (((spi_read() & 0b00011111) != 0x05));
        while ((spi_read() != 0xff));
        nsect--;
    }
    spi_write(0xff);
    spi_write(0xff);
    spi_write(0b11111101); //stop token
    spi_write(0xff);
    while ((spi_read() != 0xff));
    return 0;
}
 
int mmc_read_multiple_sector(unsigned long int sector, unsigned char *buffer, unsigned int nsect)
{
    int i, cnt;
    sector *= 512;
    cnt = 0;
    if(command(18, sector, 0xff) != 0 )
    while (spi_read() != 0 && cnt++ <1000 );
    if(cnt >980) return MMC_NO_RESPONSE;
    cnt = 0;
    while(nsect) {
        while (spi_read() != 0xfe && cnt++ <1000 );
        if(cnt >980) return MMC_NO_RESPONSE;
        cnt = 0;
        for(i = 0; i < 512; i++)
        *buffer++ = spi_read();
        spi_write(0xff);
        spi_write(0xff);
        nsect--;
    }
    if(command(12, sector, 0xff) != 0)
    while (spi_read() != 0 && cnt++ <1000 );
    if(cnt >980) return MMC_NO_RESPONSE;
    cnt = 0;
    while (spi_read() != 0xff && cnt++ <1000 );
    if(cnt >980) return MMC_NO_RESPONSE;
    return 0;
}
 
/* //SINGLE SECTOR READING CODE, NOT USED HERE
void mmc_read_sector(unsigned int sector)
{
    int i;
    sector *= 512;
    if(command(17, sector, 0xff) != 0)
    while (spi_read() != 0);
    while (spi_read() != 0xfe);
    for(i = 0; i < 512; i++)
    mmc_buf[i] = spi_read();
    spi_write(0xff);
    spi_write(0xff);
}
*/
 
int mmc_init()
{
    int u = 0;
    unsigned int count;
    //clear_pin(LP_PIN[16]);
    unsigned char ocr[10];
    SET_CHIPSELECT;
    for (u = 0; u < 50*8; u++) {
        SPI_CLOCK_initial;
    }
    CLEAR_CHIPSELECT;
    msleep(1);
    count = 0;
    while (command(0, 0, 0x95) != 1 && (count++ < 100));
    if (count > 90) {
        printk( KERN_ALERT "CARD ERROR-CMD0 \n");
        msleep(10);
        return 1;
    }
    //printk( KERN_ALERT "CMD0 PASSED!\n");
    if (command(8, 0x1AA, 0x87) == 1) { /* SDC ver 2.00 */
        for (u = 0; u < 4; u++) ocr[u] = spi_read();
        if (ocr[2] == 0x01 && ocr[3] == 0xAA) { /* The card can work at vdd range of 2.7-3.6V */
            count = 0;
            do
            command(55,0,0xff);
            while (command(41, 1UL << 30, 0xff) && count++ < 1000); /* ACMD41 with HCHIP_SELECT bit */
            if(count > 900) {printk( KERN_ALERT "ERROR SDHC 41"); return 1;}
            count = 0;
            if (command(58, 0, 0xff) == 0 && count++ <100) { /* Check CCHIP_SELECT bit */
                for (u = 0; u < 4; u++) ocr[u] = spi_read();
            }
            IS_SDHC = 1;
        }
        } else {
        command(55, 0, 0xff);
        if(command(41, 0, 0xff) >1) {
            count = 0;
            while ((command(1, 0, 0xff) != 0) && (count++ < 100));
            if (count > 90) {
                printk( KERN_ALERT "CARD ERROR-CMD1 \n");
                msleep(10);
                return 1;
            }
            } else {
            count = 0;
            do {
                command(55, 0, 0xff);
            } while(command(41, 0, 0xff) != 0 && count< 100);
        }
         
        if (command(16, 512, 0xff) != 0) {
            printk( KERN_ALERT "CARD ERROR-CMD16 \n");
            msleep(10);
            return 1;
        }
    }
    printk( KERN_ALERT "CARD INITIALIZED!\n");
    return 0;
}
 
unsigned char command(unsigned char command, uint32_t fourbyte_arg, unsigned char CRCbits)
{
    unsigned char retvalue,n;
    spi_write(0xff);
    spi_write(0b01000000 | command);
    spi_write_32(fourbyte_arg);
    spi_write(CRCbits);
     
    n = 10;
    do
    retvalue = spi_read();
    while ((retvalue & 0x80) && --n);
     
    return retvalue;
}
 
void spi_write_32(uint32_t argument)
{
    unsigned char bit;
    for (bit = 0; bit < 32; bit++) {
         
        if (argument & (1<<31))
        SET_MOSI;
        else
        CLEAR_MOSI;
        argument <<= 1;
        SPI_CLOCK;
    }
}
unsigned char spi_write(unsigned char byte)
{
    unsigned char bit;
    for (bit = 0; bit < 8; bit++) {
        outb(byte,0x378); //MOSI MSB PUSH
        byte <<= 1;
        SPI_CLOCK;
        if(CHECK_MISO)
        byte++;
    }
    return byte;
}
