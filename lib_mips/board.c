/*
 * (C) Copyright 2003
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <command.h>
#include <malloc.h>
#include <devices.h>
#include <version.h>
#include <net.h>
#include <environment.h>

DECLARE_GLOBAL_DATA_PTR;

#if ( ((CFG_ENV_ADDR+CFG_ENV_SIZE) < CFG_MONITOR_BASE) || \
      (CFG_ENV_ADDR >= (CFG_MONITOR_BASE + CFG_MONITOR_LEN)) ) || \
    defined(CFG_ENV_IS_IN_NVRAM)
#define	TOTAL_MALLOC_LEN	(CFG_MALLOC_LEN + CFG_ENV_SIZE)
#else
#define	TOTAL_MALLOC_LEN	CFG_MALLOC_LEN
#endif

#undef DEBUG

extern int timer_init(void);

extern int incaip_set_cpuclk(void);
#ifdef SUPPORT_SECOND_FLASH
void ath_set_second_flash();
#endif
#if defined(CONFIG_WASP_SUPPORT) || defined(CONFIG_MACH_QCA955x) || defined(CONFIG_MACH_QCA953x) || defined(CONFIG_MACH_QCA956x)
void ath_set_tuning_caps(void);
#endif


extern ulong uboot_end_data;
extern ulong uboot_end;

ulong monitor_flash_len;

const char version_string[] =
	U_BOOT_VERSION" (" __DATE__ " - " __TIME__ ")";

#define ELX_VERSION_PATTERN_HEAD    "ELX_"
#define ELX_VERSION_PATTERN_TAIL    "_ELX"
const char edi_ver[] = ELX_VERSION_PATTERN_HEAD \
    ELX_LOCAL_VERSION ELX_VERSION_PATTERN_TAIL;

static char *failed = "*** failed ***\n";

/*
 * Begin and End of memory area for malloc(), and current "brk"
 */
static ulong mem_malloc_start;
static ulong mem_malloc_end;
static ulong mem_malloc_brk;

#ifdef HAS_OPERATION_SELECTION/*add */
#define SEL_LOAD_LINUX_SDRAM            1
#define SEL_LOAD_LINUX_WRITE_FLASH      2
#define SEL_BOOT_FLASH                  3
#define SEL_ENTER_CLI                   4
#define SEL_LOAD_UCOS_SDRAM     5
#define SEL_LOAD_CRAMFS_WRITE_FLASH     7
#define SEL_LOAD_BOOT_SDRAM             8
#define SEL_LOAD_BOOT_WRITE_FLASH       9
#define ARGV_LEN  0x32
static char  file_name_space[ARGV_LEN];
int modifies=0;
extern int do_bootm (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);
extern char        console_buffer[CFG_CBSIZE];      /* console I/O buffer   */
extern int flash_sect_protect (int, ulong, ulong);
void input_value(u8 *str)
{
    while(1)
    {
        if (readline ("==:") > 0)
        {
            strcpy (str, console_buffer);
            break;
        }
        else
            break;
    }
}
void OperationSelect(void)
{
    printf("\nPlease choose the operation: \n");
    printf("   %d: Load system code to SDRAM via TFTP. \n", SEL_LOAD_LINUX_SDRAM);
    //printf("   %d: Load system code then write to Flash via TFTP. \n", SEL_LOAD_LINUX_WRITE_FLASH);
    printf("   %d: Boot system code via Flash (default).\n", SEL_BOOT_FLASH);
    printf("   %d: Entr boot command line interface.\n", SEL_ENTER_CLI);
    //printf("   %d: Load ucos code to SDRAM via TFTP. \n", SEL_LOAD_UCOS_SDRAM);
    //printf("   %d: Load Linux filesystem then write to Flash via TFTP. \n", SEL_LOAD_CRAMFS_WRITE_FLASH);
    //printf("   %d: Load Boot Loader code to SDRAM via TFTP. \n", SEL_LOAD_BOOT_SDRAM);
    //printf("   %d: Load Boot Loader code then write to Flash via TFTP. \n", SEL_LOAD_BOOT_WRITE_FLASH);
}
void filename_copy (uchar *dst, uchar *src, int size)
{
    *dst = '"';
    dst++;
    while ((size > 0) && *src && (*src != '"')) {
        *dst++ = *src++;
        size--;
    }
    *dst++ = '"';
    *dst = '\0';
}

int tftp_config(int type, char *argv[])
{
    char *s;
    char default_file[ARGV_LEN], file[ARGV_LEN], devip[ARGV_LEN], srvip[ARGV_LEN], default_ip[ARGV_LEN];

    printf(" Please Input new ones /or Ctrl-C to discard\n");

    memset(default_file, 0, ARGV_LEN);
    memset(file, 0, ARGV_LEN);
    memset(devip, 0, ARGV_LEN);
    memset(srvip, 0, ARGV_LEN);
    memset(default_ip, 0, ARGV_LEN);

    printf("\tInput device IP ");
    s = getenv("ipaddr");
    memcpy(devip, s, strlen(s));
    memcpy(default_ip, s, strlen(s));

    printf("(%s) ", devip);
    input_value(devip);
    setenv("ipaddr", devip);
    if (strcmp(default_ip, devip) != 0)
        modifies++;

    printf("\tInput server IP ");
    s = getenv("serverip");
    memcpy(srvip, s, strlen(s));
    memset(default_ip, 0, ARGV_LEN);
    memcpy(default_ip, s, strlen(s));

    printf("(%s) ", srvip);
    input_value(srvip);
    setenv("serverip", srvip);
    if (strcmp(default_ip, srvip) != 0)
        modifies++;

    if(type == SEL_LOAD_BOOT_SDRAM || type == SEL_LOAD_BOOT_WRITE_FLASH 
            || type == SEL_LOAD_UCOS_SDRAM) {
//         if(type == SEL_LOAD_BOOT_SDRAM)
// #if defined (RT2880_ASIC_BOARD) || defined (RT2880_FPGA_BOARD)
//             argv[1] = "0x8a200000";
// #else
//         argv[1] = "0x80200000";
// #endif
//         else if (type == SEL_LOAD_UCOS_SDRAM)
//             argv[1] = "0x88001000";
//         else
// #if defined (RT2880_ASIC_BOARD) || defined (RT2880_FPGA_BOARD)
//             argv[1] = "0x8a100000";
// #else
//         argv[1] = "0x80100000";
// #endif
// //         printf("\tInput Uboot filename ");
//         //argv[2] = "uboot.bin";
//         strncpy(argv[2], "uboot.bin", ARGV_LEN);
    }
    else if (type == SEL_LOAD_LINUX_WRITE_FLASH 
            || type == SEL_LOAD_CRAMFS_WRITE_FLASH) {
                argv[1] = "0x80060000";
            printf("\tInput Linux Kernel filename ");
                strncpy(argv[2], "uImage", ARGV_LEN);
    }
    else if (type == SEL_LOAD_LINUX_SDRAM ) {
        /* bruce to support ramdisk */

        argv[1] = "0x80800000";

        printf("\tInput Linux Kernel filename ");
        //argv[2] = "uImage";
        strncpy(argv[2], "uImage", ARGV_LEN);
    }

    s = getenv("bootfile");
    if (s != NULL) {
        memcpy(file, s, strlen(s));
        memcpy(default_file, s, strlen(s));
    }
    printf("(%s) ", file);
    input_value(file);
    if (file == NULL)
        return 1;
    filename_copy (argv[2], file, sizeof(file));
    setenv("bootfile", file);
    if (s && strcmp(default_file, file) != 0)
        modifies++;

    return 0;
}
#endif


/*
 * The Malloc area is immediately below the monitor copy in DRAM
 */
static void mem_malloc_init (void)
{
	ulong dest_addr = CFG_MONITOR_BASE + gd->reloc_off;

	mem_malloc_end = dest_addr;
	mem_malloc_start = dest_addr - TOTAL_MALLOC_LEN;
	mem_malloc_brk = mem_malloc_start;

	memset ((void *) mem_malloc_start,
		0,
		mem_malloc_end - mem_malloc_start);
}

void *sbrk (ptrdiff_t increment)
{
	ulong old = mem_malloc_brk;
	ulong new = old + increment;

	if ((new < mem_malloc_start) || (new > mem_malloc_end)) {
		return (NULL);
	}
	mem_malloc_brk = new;
	return ((void *) old);
}


static int init_func_ram (void)
{
#ifdef	CONFIG_BOARD_TYPES
	int board_type = gd->board_type;
#else
	int board_type = 0;	/* use dummy arg */
#endif
	puts ("DRAM:  ");

	if ((gd->ram_size = initdram (board_type)) > 0) {
		print_size (gd->ram_size, "\n");
		return (0);
	}
	puts (failed);
	return (1);
}

static int display_banner(void)
{

	printf ("\n\n%s\n", version_string);
    printf ("ELX version: %s\n\n", ELX_LOCAL_VERSION);

	return (0);
}

static void display_flash_config(ulong size)
{
	puts ("Flash: ");
	print_size (size, "\n");
}


static int init_baudrate (void)
{
	char tmp[64];	/* long enough for environment variables */
	int i = getenv_r ("baudrate", tmp, sizeof (tmp));

	gd->baudrate = (i > 0)
			? (int) simple_strtoul (tmp, NULL, 10)
			: CONFIG_BAUDRATE;

	return (0);
}


/*
 * Breath some life into the board...
 *
 * The first part of initialization is running from Flash memory;
 * its main purpose is to initialize the RAM so that we
 * can relocate the monitor code to RAM.
 */

/*
 * All attempts to come up with a "common" initialization sequence
 * that works for all boards and architectures failed: some of the
 * requirements are just _too_ different. To get rid of the resulting
 * mess of board dependend #ifdef'ed code we now make the whole
 * initialization sequence configurable to the user.
 *
 * The requirements for any new initalization function is simple: it
 * receives a pointer to the "global data" structure as it's only
 * argument, and returns an integer return code, where 0 means
 * "continue" and != 0 means "fatal error, hang the system".
 */
typedef int (init_fnc_t) (void);

init_fnc_t *init_sequence[] = {
#ifndef COMPRESSED_UBOOT
	timer_init,
#endif
	env_init,		/* initialize environment */
#ifdef CONFIG_INCA_IP
	incaip_set_cpuclk,	/* set cpu clock according to environment variable */
#endif
	init_baudrate,		/* initialze baudrate settings */
#ifndef COMPRESSED_UBOOT
	serial_init,		/* serial communications setup */
#endif
	console_init_f,
	display_banner,		/* say that we are here */
#ifndef COMPRESSED_UBOOT
	checkboard,
        init_func_ram,
#endif
	NULL,
};


void board_init_f(ulong bootflag)
{
	gd_t gd_data, *id;
	bd_t *bd;
	init_fnc_t **init_fnc_ptr;
	ulong addr, addr_sp, len = (ulong)&uboot_end - CFG_MONITOR_BASE;
	ulong *s;
#ifdef COMPRESSED_UBOOT
        char board_string[50];
#endif
#ifdef CONFIG_PURPLE
	void copy_code (ulong);
#endif


	/* Pointer is writable since we allocated a register for it.
	 */
	gd = &gd_data;
	/* compiler optimization barrier needed for GCC >= 3.4 */
	__asm__ __volatile__("": : :"memory");

	memset ((void *)gd, 0, sizeof (gd_t));

	for (init_fnc_ptr = init_sequence; *init_fnc_ptr; ++init_fnc_ptr) {
		if ((*init_fnc_ptr)() != 0) {
			hang ();
		}
	}

#ifdef COMPRESSED_UBOOT
        checkboard(board_string);
        printf("%s\n\n",board_string);
        gd->ram_size = bootflag;
	puts ("DRAM:  ");
	print_size (gd->ram_size, "\n");
#endif

	/*
	 * Now that we have DRAM mapped and working, we can
	 * relocate the code and continue running from DRAM.
	 */
	addr = CFG_SDRAM_BASE + gd->ram_size;

	/* We can reserve some RAM "on top" here.
	 */

	/* round down to next 4 kB limit.
	 */
	addr &= ~(4096 - 1);
	debug ("Top of RAM usable for U-Boot at: %08lx\n", addr);

	/* Reserve memory for U-Boot code, data & bss
	 * round down to next 16 kB limit
	 */
	addr -= len;
	addr &= ~(16 * 1024 - 1);

	debug ("Reserving %ldk for U-Boot at: %08lx\n", len >> 10, addr);

	 /* Reserve memory for malloc() arena.
	 */
	addr_sp = addr - TOTAL_MALLOC_LEN;
	debug ("Reserving %dk for malloc() at: %08lx\n",
			TOTAL_MALLOC_LEN >> 10, addr_sp);

	/*
	 * (permanently) allocate a Board Info struct
	 * and a permanent copy of the "global" data
	 */
	addr_sp -= sizeof(bd_t);
	bd = (bd_t *)addr_sp;
	gd->bd = bd;
	debug ("Reserving %d Bytes for Board Info at: %08lx\n",
			sizeof(bd_t), addr_sp);

	addr_sp -= sizeof(gd_t);
	id = (gd_t *)addr_sp;
	debug ("Reserving %d Bytes for Global Data at: %08lx\n",
			sizeof (gd_t), addr_sp);

 	/* Reserve memory for boot params.
	 */
	addr_sp -= CFG_BOOTPARAMS_LEN;
	bd->bi_boot_params = addr_sp;
	debug ("Reserving %dk for boot params() at: %08lx\n",
			CFG_BOOTPARAMS_LEN >> 10, addr_sp);

	/*
	 * Finally, we set up a new (bigger) stack.
	 *
	 * Leave some safety gap for SP, force alignment on 16 byte boundary
	 * Clear initial stack frame
	 */
	addr_sp -= 16;
	addr_sp &= ~0xF;
	s = (ulong *)addr_sp;
	*s-- = 0;
	*s-- = 0;
	addr_sp = (ulong)s;
	debug ("Stack Pointer at: %08lx\n", addr_sp);

	/*
	 * Save local variables to board info struct
	 */
	bd->bi_memstart	= CFG_SDRAM_BASE;	/* start of  DRAM memory */
	bd->bi_memsize	= gd->ram_size;		/* size  of  DRAM memory in bytes */
	bd->bi_baudrate	= gd->baudrate;		/* Console Baudrate */

	memcpy (id, (void *)gd, sizeof (gd_t));

	/* On the purple board we copy the code in a special way
	 * in order to solve flash problems
	 */
#ifdef CONFIG_PURPLE
	copy_code(addr);
#endif

	relocate_code (addr_sp, id, addr);

	/* NOTREACHED - relocate_code() does not return */
}
/************************************************************************
 *
 * This is the next part if the initialization sequence: we are now
 * running from RAM and have a "normal" C environment, i. e. global
 * data can be written, BSS has been cleared, the stack size in not
 * that critical any more, etc.
 *
 ************************************************************************
 */

void board_init_r (gd_t *id, ulong dest_addr)
{
	cmd_tbl_t *cmdtp;
	ulong size;
        char addr_str[11+1];
        unsigned char confirm=0;
        ulong e_end;
	extern void malloc_bin_reloc (void);
#ifndef CFG_ENV_IS_NOWHERE
	extern char * env_name_spec;
#endif
#ifdef CONFIG_ATH_NAND_SUPPORT
#ifdef ATH_SPI_NAND
	extern ulong ath_spi_nand_init(void);
#else
	extern ulong ath_nand_init(void);
#endif
#endif
	char *s, *e;
	bd_t *bd;
	int i;

	gd = id;
	gd->flags |= GD_FLG_RELOC;	/* tell others: relocation done */

	debug ("Now running in RAM - U-Boot at: %08lx\n", dest_addr);

	/*jaykung enable hw watchdog if needed*/
	init_hw_watchdog();
	reset_watchdog();
	
	gd->reloc_off = dest_addr - CFG_MONITOR_BASE;

	monitor_flash_len = (ulong)&uboot_end_data - dest_addr;

	/*
	 * We have to relocate the command table manually
	 */
 	for (cmdtp = &__u_boot_cmd_start; cmdtp !=  &__u_boot_cmd_end; cmdtp++) {
		ulong addr;

		addr = (ulong) (cmdtp->cmd) + gd->reloc_off;
#if 0
		printf ("Command \"%s\": 0x%08lx => 0x%08lx\n",
				cmdtp->name, (ulong) (cmdtp->cmd), addr);
#endif
		cmdtp->cmd =
			(int (*)(struct cmd_tbl_s *, int, int, char *[]))addr;

		addr = (ulong)(cmdtp->name) + gd->reloc_off;
		cmdtp->name = (char *)addr;

		if (cmdtp->usage) {
			addr = (ulong)(cmdtp->usage) + gd->reloc_off;
			cmdtp->usage = (char *)addr;
		}
#ifdef	CFG_LONGHELP
		if (cmdtp->help) {
			addr = (ulong)(cmdtp->help) + gd->reloc_off;
			cmdtp->help = (char *)addr;
		}
#endif
	}
	/* there are some other pointer constants we must deal with */
#ifndef CFG_ENV_IS_NOWHERE
	env_name_spec += gd->reloc_off;
#endif
#ifdef SUPPORT_SECOND_FLASH
	ath_set_second_flash();
#endif
#ifndef CONFIG_ATH_NAND_BR
	/* configure available FLASH banks */
	size = flash_init();
	display_flash_config (size);
#endif

	bd = gd->bd;
	bd->bi_flashstart = CFG_FLASH_BASE;
	bd->bi_flashsize = size;
#if CFG_MONITOR_BASE == CFG_FLASH_BASE
	bd->bi_flashoffset = monitor_flash_len;	/* reserved area for U-Boot */
#else
	bd->bi_flashoffset = 0;
#endif

	/* initialize malloc() area */
	mem_malloc_init();
	malloc_bin_reloc();

#ifdef CONFIG_ATH_NAND_BR
	ath_nand_init();
#endif

	/* relocate environment function pointers etc. */
	env_relocate();

	/* board MAC address */
	s = getenv ("ethaddr");
	for (i = 0; i < 6; ++i) {
		bd->bi_enetaddr[i] = s ? simple_strtoul (s, &e, 16) : 0;
		if (s)
			s = (*e) ? e + 1 : e;
	}

	/* IP Address */
	bd->bi_ip_addr = getenv_IPaddr("ipaddr");

#if defined(CONFIG_PCI)
	/*
	 * Do pci configuration
	 */
	pci_init();
#endif

/** leave this here (after malloc(), environment and PCI are working) **/
	/* Initialize devices */
	devices_init ();

	jumptable_init ();

	/* Initialize the console (after the relocation and devices init) */
	console_init_r ();
/** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** **/

	/* Initialize from environment */
	if ((s = getenv ("loadaddr")) != NULL) {
		load_addr = simple_strtoul (s, NULL, 16);
	}
#if (CONFIG_COMMANDS & CFG_CMD_NET)
	if ((s = getenv ("bootfile")) != NULL) {
		copy_filename (BootFile, s, sizeof (BootFile));
	}
#endif	/* CFG_CMD_NET */

#if defined(CONFIG_MISC_INIT_R)
	/* miscellaneous platform dependent initialisations */
	misc_init_r ();
#endif

#if (CONFIG_COMMANDS & CFG_CMD_NET)
#if defined(CONFIG_NET_MULTI)
	puts ("Net:   ");
#endif
	eth_initialize(gd->bd);
#endif

#if defined(CONFIG_ATH_NAND_SUPPORT) && !defined(CONFIG_ATH_NAND_BR)
#ifdef ATH_SPI_NAND
	ath_spi_nand_init();
#else
 	ath_nand_init();
#endif
#endif

//#if defined(CONFIG_WASP_SUPPORT) || defined(CONFIG_MACH_QCA955x)
        ath_set_tuning_caps(); /* Needed here not to mess with Ethernet clocks */
//#endif

	reset_watchdog();	

#ifdef HAS_OPERATION_SELECTION/*add by jaykung*/
    int timer1= CONFIG_BOOTDELAY;
    int my_tmp;
    unsigned char BootType='3';
    OperationSelect();

     reset_watchdog();

    while (timer1 > 0) {
        --timer1;
        /* delay 100 * 10ms */
        for (i=0; i<20; ++i) {

            reset_watchdog();

            if ((my_tmp = tstc()) != 0) {   /* we got a key press   */
                timer1 = 0; /* no more delay    */
                BootType = getc();
                if ((BootType < '1' || BootType > '5') && (BootType != '7') && (BootType != '8') && (BootType != '9'))
                    BootType = '3';
                printf("\n\rYou choosed %c\n\n", BootType);
                break;
            }
            reset_watchdog();

            udelay (10000);

            reset_watchdog();

        }
        printf ("\b\b\b%2d ", timer1);
    }
    putc ('\n');
    if(BootType == '3') {
#ifdef DEF_USE_BOOTCMD
		char *s;
		s = getenv ("bootcmd");
		if (s) 
		{
			run_command (s, 0);
		}
		else
		{
			BootType = '4';
			goto do_other_cmd;
		}
#else			
        char *argv[2];
        sprintf(addr_str, "0x%X", CFG_KERN_ADDR);

        argv[1] = &addr_str[0];
        printf("   \n3: System Boot system code via Flash.\n");

        reset_watchdog();

		if(do_bootm(cmdtp, 0, 2, argv))
		{
			printf("Enter Recovery Mode\n");
		}
		
//         do_bootm(cmdtp, 0, 2, argv);
#endif
    }
    else
    {
        char *argv[4];
        int argc= 3;
		do_other_cmd:
        argv[2] = &file_name_space[0];
        memset(file_name_space,0,ARGV_LEN);
       
//  #if (CONFIG_COMMANDS & CFG_CMD_NET)
//  #if defined(CONFIG_NET_MULTI)
//          puts ("Net:   ");
//  #endif
//          eth_initialize(gd->bd);
//  #endif
        switch(BootType) {
            case '1':
                printf("   \n%d: System Load Linux to SDRAM via TFTP. \n", SEL_LOAD_LINUX_SDRAM);
                tftp_config(SEL_LOAD_LINUX_SDRAM, argv);
                argc= 3;
                setenv("autostart", "yes");
                do_tftpb(cmdtp, 0, argc, argv);
                break;
            case '2':
                                printf("   \n%d: System Load Linux Kernel then write to Flash via TFTP. \n", SEL_LOAD_LINUX_WRITE_FLASH);
                                printf(" Warning!! Erase Linux in Flash then burn new one. Are you sure?(Y/N)\n");
                                confirm = getc();
                                if (confirm != 'y' && confirm != 'Y') {
                                        printf(" Operation terminated\n");
                                        break;
                                }
                                tftp_config(SEL_LOAD_LINUX_WRITE_FLASH, argv);
                                argc= 3;
                                setenv("autostart", "no");
                                do_tftpb(cmdtp, 0, argc, argv);
            
                //protect off kernel-->end
                flash_sect_protect(0, CFG_FLASH_BASE+CFG_BOOTLOADER_SIZE, CFG_FLASH_BASE+size-1);

                                //erase linux
            if (NetBootFileXferSize <= (bd->bi_flashsize - (CFG_BOOTLOADER_SIZE + CFG_CONFIG_SIZE + CFG_FACTORY_SIZE))) {
                                e_end = CFG_KERN_ADDR + NetBootFileXferSize;
                        if (0 != get_addr_boundary(&e_end))
                        break;
                        printf("Erase linux kernel block !!\n");
                        printf("From 0x%X To 0x%X\n", CFG_KERN_ADDR, e_end);
                        flash_sect_erase(CFG_KERN_ADDR, e_end);
                }
                        else {
                                printf("The Linux Image size is too big !! \n");
                                break;
                        }
            //cp.linux
            argc = 4;
            argv[0]= "cp.linux";
            do_mem_cp(cmdtp, 0, argc, argv);

            argc= 2;
                sprintf(addr_str, "0x%X", CFG_KERN_ADDR);
                argv[1] = &addr_str[0];
             do_bootm(cmdtp, 0, argc, argv);            

                break;
            case '4':
                printf("   \n%d: System Enter Boot Command Line Interface.\n", SEL_ENTER_CLI);
                printf ("\n%s\n", version_string);
                /* main_loop() can return to retry autoboot, if so just run it again. */
                for (;;) {                  
                    main_loop ();
                }
                break;
 
        }
    }
#else
	/* main_loop() can return to retry autoboot, if so just run it again. */
	for (;;) {
		main_loop ();
	}
#endif
	/* NOTREACHED - no way out of command loop except booting */
}

#ifdef UBOOT_SUPPORT_HW_WD
static u32 hw_wd_flag;
static ulong watch_timer=0;
#define HW_RESET_TIME	CFG_HZ/10 //(30000 * (CFG_HZ / 1000000))
int init_hw_watchdog()
{
// 	ath_reg_rmw_set(GPIO_OE_ADDRESS, (1 << UBOOT_SUPPORT_HW_WD_GPIO));
	ath_reg_rmw_clear(GPIO_OE_ADDRESS, (1 << UBOOT_SUPPORT_HW_WD_GPIO));
	watch_timer = get_timer(0);
	return 1;
}
int reset_watchdog()
{
	
	if((ulong)((ulong)get_ticks() - watch_timer) < HW_RESET_TIME)
		return 1;
	
	hw_wd_flag=hw_wd_flag^1;

 	if(hw_wd_flag)
 	{
		ath_reg_rmw_set(GPIO_OUT_ADDRESS, (1 << UBOOT_SUPPORT_HW_WD_GPIO));
 	}
 	else
 	{
		ath_reg_rmw_clear(GPIO_OUT_ADDRESS, (1 << UBOOT_SUPPORT_HW_WD_GPIO));
 	}
 	watch_timer = get_timer(0);
}
#endif



void hang (void)
{
	puts ("### ERROR ### Please RESET the board ###\n");
	for (;;);
}
