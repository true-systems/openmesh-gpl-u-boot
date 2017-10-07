#include <common.h>
#include <jffs2/jffs2.h>
#include <asm/addrspace.h>
#include <asm/types.h>
#include <atheros.h>
#include "ath_flash.h"

#if !defined(ATH_DUAL_FLASH)
#	define	ath_spi_flash_print_info	flash_print_info
#endif

/*
 * globals
 */
flash_info_t flash_info[CFG_MAX_FLASH_BANKS];

/*
 * statics
 */
static void ath_spi_write_enable(void);
static void ath_spi_poll(void);
#if !defined(ATH_SST_FLASH)
static void ath_spi_write_page(uint32_t addr, uint8_t * data, int len);
#endif
static void ath_spi_sector_erase(uint32_t addr);

static u32
ath_spi_read_id(void)
{
	u32 rd;

	ath_reg_wr_nf(ATH_SPI_WRITE, ATH_SPI_CS_DIS);
	ath_spi_bit_banger(ATH_SPI_CMD_RDID);
	reset_watchdog();
	ath_spi_delay_8();
	ath_spi_delay_8();
	ath_spi_delay_8();
	ath_spi_go();
	reset_watchdog();

	rd = ath_reg_rd(ATH_SPI_RD_STATUS);

	printf("Flash Manuf Id 0x%x, DeviceId0 0x%x, DeviceId1 0x%x\n",
		(rd >> 16) & 0xff, (rd >> 8) & 0xff, (rd >> 0) & 0xff);
        return rd;
}


#ifdef ATH_SST_FLASH
void ath_spi_flash_unblock(void)
{
	ath_spi_write_enable();
	ath_spi_bit_banger(ATH_SPI_CMD_WRITE_SR);
	ath_spi_bit_banger(0x0);
	ath_spi_go();
	ath_spi_poll();
}
#endif




struct chip_info {
        char            *name;
        u8              id;
        u32             jedec_id;
        unsigned long   sector_size;
        unsigned int    n_sectors;
        char            addr4b;
};

#define ARRAY_SIZE(x)    (sizeof(x) / sizeof((x)[0]))
static struct chip_info chips_data [] = {
        /* REVISIT: fill in JEDEC ids, for parts that have them */
        { "AT25DF321",          0x1f, 0x47000000, 64 * 1024, 64,  0 },
        { "AT26DF161",          0x1f, 0x46000000, 64 * 1024, 32,  0 },
        { "FL016AIF",           0x01, 0x02140000, 64 * 1024, 32,  0 },
        { "FL064AIF",           0x01, 0x02160000, 64 * 1024, 128, 0 },
        { "MX25L1605D",         0xc2, 0x2015c220, 64 * 1024, 32,  0 },
#if 0
        { "MX25L3205D",         0xc2, 0x2016c220, 64 * 1024, 64,  0 },
        { "MX25L6405D",         0xc2, 0x2017c220, 64 * 1024, 128, 0 },
        { "MX25L12805D",        0xc2, 0x2018c220, 64 * 1024, 256, 0 },
#else
        //jaykung we use 4K sector size
        { "MX25L3206E",        0xc2, 0x2016c200, 4 * 1024, 1024, 0 }, // MX25L3206E, 0x2016c220, 1024 Sectors = 1024 * (4 * 1024) Bytes
        { "MX25L3205D",     0xc2, 0x2016c220, 4 * 1024, 1024,  0 },
//         { "MX25L6405D",         0xc2, 0x2017c220, 4 * 1024, 2048, 0 },
	//let's keep 64k sector size for 9531 platform
	{ "MX25L6405D",         0xc2, 0x2017c220, 64 * 1024, 128, 0 },
        { "MX25L12805D",        0xc2, 0x2018c220, 4 * 1024, 4096, 0 },
        { "MX25L12845E",        0xc2, 0x2018c200, 64 * 1024, 256, 0 },
#endif
        { "MX25L25635E",        0xc2, 0x2019c220, 64 * 1024, 512, 1 },
        { "S25FL256S",          0x01, 0x02194D01, 64 * 1024, 512, 1 },
        { "S25FL128P",          0x01, 0x20180301, 64 * 1024, 256, 0 },
        { "S25FL129P",          0x01, 0x20184D01, 64 * 1024, 256, 0 },
        { "S25FL032P",          0x01, 0x02154D00, 64 * 1024, 64,  0 },
        { "S25FL064P",          0x01, 0x02164D00, 64 * 1024, 128, 0 },
        { "EN25F16",            0x1c, 0x31151c31, 64 * 1024, 32,  0 },
        { "EN25Q32B",           0x1c, 0x30161c30, 64 * 1024, 64,  0 },
        { "EN25F32",            0x1c, 0x31161c31, 64 * 1024, 64,  0 },
        { "EN25Q32B",           0x1c, 0x30161c30, 64 * 1024, 64,  0 },
        { "EN25F64",            0x1c, 0x20171c20, 64 * 1024, 128, 0 },  // EN25P64
        { "EN25Q64",            0x1c, 0x30171c30, 64 * 1024, 128, 0 },
        //{ "W25Q32BV",         0xef, 0x40160000, 64 * 1024, 64,  0 },
        { "W25Q32BV",           0xef, 0x40160000, 4 * 1024, 1024,  0 },//jaykung we use 4k sector size
        { "W25X32VS",           0xef, 0x30160000, 64 * 1024, 64,  0 },       
        { "W25Q64BV",           0xef, 0x40170000, 64 * 1024, 128, 0 }, //S25FL064K
        { "W25Q128BV",          0xef, 0x40180000, 64 * 1024, 256, 0 },
};


unsigned long my_flash_get_geom(u32 rd, flash_info_t *flash_info)
{
    struct chip_info *info, *match;
    unsigned long flashsize=0;
    u8 buf[5];
    u32  jedec, weight;
    int i;
    int i_found=-1;
    buf[3]=(rd>>8)&0xff;
    buf[2]=(rd>>0)&0xff;
    buf[1]=(rd>>16)&0xff;
    buf[0]=(rd>>24)&0xff;
        
         
    jedec = (u32)((u32)(buf[3] << 24) | ((u32)buf[2] << 16) | ((u32)buf[1] <<8) | (u32)buf[0]);

        //printf("device id : %x[0] %x[1] %x[2] %x[3] %x (%x)\n", buf[0], buf[1], buf[2], buf[3], buf[4], jedec);
         
        // FIXME, assign default as AT25D
    weight = 0xffffffff;
    match = &chips_data[0];
    for (i = 0; i < ARRAY_SIZE(chips_data); i++) {
                info = &chips_data[i];
                if (info->id == ((rd >> 16) & 0xff) /** Manuf Id  */) {
//                     printk("info->jedec_id  0x%x jedec 0x%x info->jedec_id ^ jedec 0x%x\n", info->jedec_id, jedec,info->jedec_id ^ jedec );
                        if (info->jedec_id == jedec)
                        {
                                i_found=i;
                                break;
                        }

                        if (weight > (info->jedec_id ^ jedec)) {
                                weight = info->jedec_id ^ jedec;
                                match = info;
                                i_found=i;
                        }
                }
        }
        
      
     if (i_found>=0)
     {
     //       printf("found matched flash");
            flashsize=chips_data[i_found].sector_size*chips_data[i_found].n_sectors;
         
     }
     else
     {
            flashsize=FLASH_SIZE*1024*1024; /** MB*/
            printf("No matched flash found, use defaul flash size");
     }
     printf("Flash [%s] sectors: %d\n",
            (i_found>=0)?chips_data[i_found].name:"Unkown", chips_data[i_found].n_sectors);
     
     /** cfho 2013-0516, fill the flash information to the FLASH[] */
     flash_info->flash_id = chips_data[i_found].id;
     flash_info->size = flashsize; /* bytes */
     flash_info->sector_count = chips_data[i_found].n_sectors;

      for (i = 0; i < flash_info->sector_count; i++) {
                flash_info->start[i] = CFG_FLASH_BASE +
                                        (i * chips_data[i_found].sector_size);
                flash_info->protect[i] = 0;
      }

//         printf ("flash size %dMB, sector count = %d\n",
//                         FLASH_SIZE, flash_info->sector_count);

    return (flash_info->size);

}
unsigned long flash_init(void)
{
     u32 rd;
#if !(defined(CONFIG_WASP_SUPPORT) || defined(CONFIG_MACH_QCA955x) || defined(CONFIG_MACH_QCA956x))
#ifdef ATH_SST_FLASH
	ath_reg_wr_nf(ATH_SPI_CLOCK, 0x3);
	ath_spi_flash_unblock();
	ath_reg_wr(ATH_SPI_FS, 0);
#else
	ath_reg_wr_nf(ATH_SPI_CLOCK, 0x43);
#endif
#endif 

#if  defined(CONFIG_MACH_QCA953x) /* Added for HB-SMIC */  
#ifdef ATH_SST_FLASH
	ath_reg_wr_nf(ATH_SPI_CLOCK, 0x4);
	ath_spi_flash_unblock();
	ath_reg_wr(ATH_SPI_FS, 0);
#else
	ath_reg_wr_nf(ATH_SPI_CLOCK, 0x44);
#endif
#endif
	ath_reg_rmw_set(ATH_SPI_FS, 1);
	rd=ath_spi_read_id();
	ath_reg_rmw_clear(ATH_SPI_FS, 1);
	reset_watchdog();
	/*
	 * hook into board specific code to fill flash_info
	 */
	//return (flash_get_geom(&flash_info[0]));
	return (my_flash_get_geom(rd,&flash_info[0]));
        
}

void
ath_spi_flash_print_info(flash_info_t *info)
{
	printf("The hell do you want flinfo for??\n");
}

int
flash_erase(flash_info_t *info, int s_first, int s_last)
{
	int i, sector_size = info->size / info->sector_count;
	reset_watchdog();
	printf("\nFirst %#x last %#x sector size %#x\n",
		s_first, s_last, sector_size);

	for (i = s_first; i <= s_last; i++) {
		printf("\b\b\b\b%4d", i);
		reset_watchdog();
		ath_spi_sector_erase(i * sector_size);
	}
	ath_spi_done();
	reset_watchdog();
	printf("\n");

	return 0;
}

/*
 * Write a buffer from memory to flash:
 * 0. Assumption: Caller has already erased the appropriate sectors.
 * 1. call page programming for every 256 bytes
 */
#ifdef ATH_SST_FLASH
void
ath_spi_flash_chip_erase(void)
{
	ath_spi_write_enable();
	ath_spi_bit_banger(ATH_SPI_CMD_CHIP_ERASE);
	ath_spi_go();
	ath_spi_poll();
}

int
write_buff(flash_info_t *info, uchar *src, ulong dst, ulong len)
{
	uint32_t val;

	dst = dst - CFG_FLASH_BASE;
	printf("write len: %lu dst: 0x%x src: %p\n", len, dst, src);

	for (; len; len--, dst++, src++) {
		ath_spi_write_enable();	// dont move this above 'for'
		ath_spi_bit_banger(ATH_SPI_CMD_PAGE_PROG);
		ath_spi_send_addr(dst);

		val = *src & 0xff;
		ath_spi_bit_banger(val);

		ath_spi_go();
		ath_spi_poll();
	}
	/*
	 * Disable the Function Select
	 * Without this we can't read from the chip again
	 */
	ath_reg_wr(ATH_SPI_FS, 0);

	if (len) {
		// how to differentiate errors ??
		return ERR_PROG_ERROR;
	} else {
		return ERR_OK;
	}
}
#else
int
write_buff(flash_info_t *info, uchar *source, ulong addr, ulong len)
{
	int total = 0, len_this_lp, bytes_this_page;
	ulong dst;
	uchar *src;

	printf("write addr: %x\n", addr);
	addr = addr - CFG_FLASH_BASE;

	while (total < len) {
		reset_watchdog();
		src = source + total;
		dst = addr + total;
		bytes_this_page =
			ATH_SPI_PAGE_SIZE - (addr % ATH_SPI_PAGE_SIZE);
		len_this_lp =
			((len - total) >
			bytes_this_page) ? bytes_this_page : (len - total);
		ath_spi_write_page(dst, src, len_this_lp);
		total += len_this_lp;
	}

	ath_spi_done();

	return 0;
}
#endif

static void
ath_spi_write_enable()
{
	ath_reg_wr_nf(ATH_SPI_FS, 1);
	ath_reg_wr_nf(ATH_SPI_WRITE, ATH_SPI_CS_DIS);
	ath_spi_bit_banger(ATH_SPI_CMD_WREN);
	ath_spi_go();
}

static void
ath_spi_poll()
{
	int rd;

	do {
		ath_reg_wr_nf(ATH_SPI_WRITE, ATH_SPI_CS_DIS);
		ath_spi_bit_banger(ATH_SPI_CMD_RD_STATUS);
		ath_spi_delay_8();
		rd = (ath_reg_rd(ATH_SPI_RD_STATUS) & 1);
	} while (rd);
}

#if !defined(ATH_SST_FLASH)
static void
ath_spi_write_page(uint32_t addr, uint8_t *data, int len)
{
	int i;
	uint8_t ch;

	display(0x77);
	ath_spi_write_enable();
	ath_spi_bit_banger(ATH_SPI_CMD_PAGE_PROG);
	ath_spi_send_addr(addr);

	for (i = 0; i < len; i++) {
		reset_watchdog();
		ch = *(data + i);
		ath_spi_bit_banger(ch);
	}

	ath_spi_go();
	display(0x66);
	ath_spi_poll();
	display(0x6d);
}
#endif

static void
ath_spi_sector_erase(uint32_t addr)
{
	ath_spi_write_enable();
	ath_spi_bit_banger(ATH_SPI_CMD_SECTOR_ERASE);
	ath_spi_send_addr(addr);
	ath_spi_go();
	display(0x7d);
	ath_spi_poll();
}

#ifdef ATH_DUAL_FLASH
void flash_print_info(flash_info_t *info)
{
	ath_spi_flash_print_info(NULL);
	ath_nand_flash_print_info(NULL);
}
#endif
