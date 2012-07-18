#define MOSI 7  //378h
#define MISO 3  //379h
#define SCK 2  //37ah
#define CHIP_SELECT 3  //37ah
#define MMC_NO_RESPONSE -1
 
#define SET_CHIPSELECT outb(inb(0x37a) & ~(1<<CHIP_SELECT), 0x37a)
#define CLEAR_CHIPSELECT outb(inb(0x37a) | (1<<CHIP_SELECT), 0x37a)
#define spi_read() spi_write(0xff)
#define SPI_CLOCK outb(~(1<<SCK) & 0x0f, 0x37a);outb((~0x00) & 0x0f, 0x37a)
#define CHECK_MISO (inb(0x379) & 1<<MISO)
#define SET_MOSI outb(1<<MOSI,0x378)
#define CLEAR_MOSI outb(0<<MOSI,0x378)
#define SPI_CLOCK_initial outb(inb(0x37a) & ~(1<<SCK), 0x37a);outb(inb(0x37a) | 1<<SCK , 0x37a)
 
unsigned char spi_write(unsigned char);
unsigned char command(unsigned char, uint32_t, unsigned char);
int mmc_init();
void spi_write_32(uint32_t argument);
int mmc_read_multiple_sector(unsigned long int sector, unsigned char *buffer, unsigned int nsect);
int mmc_write_multiple_sector(unsigned long int sector, unsigned char *buffer, unsigned int nsect);
unsigned long int read_card_size(void); 
