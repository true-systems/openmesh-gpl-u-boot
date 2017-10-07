#include <common.h>
#include <command.h>
#include <asm/mipsregs.h>
#include <asm/addrspace.h>
#include <config.h>
#include <version.h>
#include "ar7240_soc.h"

extern int wasp_ddr_initial_config(uint32_t refresh);
extern int ar7240_ddr_find_size(void);

#ifdef COMPRESSED_UBOOT
#	define prmsg(...)
#	define args		char *s
#	define board_str(a)	strcpy(s, a)
#else
#	define prmsg		printf
#	define args		void
#	define board_str(a)	printf(a)
#endif

void
wasp_usb_initial_config(void)
{
#define unset(a)	(~(a))

	if ((ar7240_reg_rd(WASP_BOOTSTRAP_REG) & WASP_REF_CLK_25) == 0) {
		ar7240_reg_wr_nf(AR934X_SWITCH_CLOCK_SPARE,
			ar7240_reg_rd(AR934X_SWITCH_CLOCK_SPARE) |
			SWITCH_CLOCK_SPARE_USB_REFCLK_FREQ_SEL_SET(2));
	} else {
		ar7240_reg_wr_nf(AR934X_SWITCH_CLOCK_SPARE,
			ar7240_reg_rd(AR934X_SWITCH_CLOCK_SPARE) |
			SWITCH_CLOCK_SPARE_USB_REFCLK_FREQ_SEL_SET(5));
	}

	udelay(1000);
	ar7240_reg_wr(AR7240_RESET,
		ar7240_reg_rd(AR7240_RESET) |
		RST_RESET_USB_PHY_SUSPEND_OVERRIDE_SET(1));
	udelay(1000);
	ar7240_reg_wr(AR7240_RESET,
		ar7240_reg_rd(AR7240_RESET) &
		unset(RST_RESET_USB_PHY_RESET_SET(1)));
	udelay(1000);
	ar7240_reg_wr(AR7240_RESET,
		ar7240_reg_rd(AR7240_RESET) &
		unset(RST_RESET_USB_PHY_ARESET_SET(1)));
	udelay(1000);
	ar7240_reg_wr(AR7240_RESET,
		ar7240_reg_rd(AR7240_RESET) &
		unset(RST_RESET_USB_HOST_RESET_SET(1)));
	udelay(1000);
	if ((ar7240_reg_rd(AR7240_REV_ID) & 0xf) == 0) {
		/* Only for WASP 1.0 */
		ar7240_reg_wr(0xb8116c84 ,
			ar7240_reg_rd(0xb8116c84) & unset(1<<20));
	}
}

void wasp_gpio_config(void)
{
	/* disable the CLK_OBS on GPIO_4 and set GPIO4 as input */
	ar7240_reg_rmw_clear(GPIO_OE_ADDRESS, (1 << 4));
	ar7240_reg_rmw_clear(GPIO_OUT_FUNCTION1_ADDRESS, GPIO_OUT_FUNCTION1_ENABLE_GPIO_4_MASK);
	ar7240_reg_rmw_set(GPIO_OUT_FUNCTION1_ADDRESS, GPIO_OUT_FUNCTION1_ENABLE_GPIO_4_SET(0x80));
	ar7240_reg_rmw_set(GPIO_OE_ADDRESS, (1 << 4));
#ifdef CONFIG_GPIO_CUSTOM
	ar7240_reg_rmw_clear(0x18040034, 0xff000000);
	ar7240_reg_rmw_clear(0x1804003c, 0x0000ffff);
	ar7240_reg_rmw_set(0x1804003c, 0x00002d29);
	ar7240_reg_rmw_set(0x1804006c, 0x2);
	ar7240_reg_rmw_clear (AR7240_GPIO_OE, 0x3f001);
	ar7240_reg_rmw_clear (AR7240_GPIO_OUT, 0x30001);
	ar7240_reg_rmw_set (AR7240_GPIO_OUT, 0xe800);
#endif
}

void ath_set_tuning_caps(void)
{
	typedef struct {
		u_int8_t	pad[0x28],
				params_for_tuning_caps[2],
				featureEnable;
	} __attribute__((__packed__)) ar9300_eeprom_t;

	ar9300_eeprom_t	*eep = (ar9300_eeprom_t *)WLANCAL;
	uint32_t	val;


	val = 0;
	/* checking feature enable bit 6 and caldata is valid */
	if ((eep->featureEnable & 0x40) && (eep->pad[0x0] != 0xff)) {
		/* xtal_capin -bit 17:23 and xtag_capout -bit 24:30*/
		val = (eep->params_for_tuning_caps[0] & 0x7f) << 17;
		val |= (eep->params_for_tuning_caps[0] & 0x7f) << 24;
	} else {
		/* default when no caldata available*/
		/* checking clock in bit 4 */
		if (ar7240_reg_rd(RST_BOOTSTRAP_ADDRESS) & 0x10) {
			val = (0x1020 << 17);  /*default 0x2040 for 40Mhz clock*/
		} else {
			val = (0x2040 << 17); /*default 0x4080 for 25Mhz clock*/
		}
	}
	val |= (ar7240_reg_rd(XTAL_ADDRESS) & (((1 << 17) - 1) | (1 << 31)));
	ar7240_reg_wr(XTAL_ADDRESS, val);
	prmsg("Setting 0xb8116290 to 0x%x\n", val);
	return;
}

int
wasp_mem_config(void)
{
	extern void ath_ddr_tap_cal(void);
	unsigned int type, reg32;

	type = wasp_ddr_initial_config(CFG_DDR_REFRESH_VAL);

	ath_ddr_tap_cal();

	prmsg("Tap value selected = 0x%x [0x%x - 0x%x]\n",
		ar7240_reg_rd(AR7240_DDR_TAP_CONTROL0),
		ar7240_reg_rd(0xbd007f10), ar7240_reg_rd(0xbd007f14));

	/* Take WMAC out of reset */
	reg32 = ar7240_reg_rd(AR7240_RESET);
	reg32 = reg32 &  ~AR7240_RESET_WMAC;
	ar7240_reg_wr_nf(AR7240_RESET, reg32);

#if !defined(CONFIG_ATH_NAND_BR)
	/* Switching regulator settings */
	ar7240_reg_wr_nf(0x18116c40, 0x633c8176); /* AR_PHY_PMU1 */
	ar7240_reg_wr_nf(0x18116c44, 0x10380000); /* AR_PHY_PMU2 */

	wasp_usb_initial_config();

#endif /* !defined(CONFIG_ATH_NAND_BR) */

	wasp_gpio_config();

	ath_set_tuning_caps(); /* Needed here not to mess with Ethernet clocks */ 

	reg32 = ar7240_ddr_find_size();

	return reg32;
}

long int initdram(int board_type)
{
	return (wasp_mem_config());
}

int	checkboard(args)
{
	board_str(BOARD_NAME" (ar934x) U-boot\n");

	return 0;
}

void hw_watchdog_reset (void)
{
    if ((ar7240_reg_rd (AR7240_GPIO_OUT) & 0x1000) == 0) {
        ar7240_reg_rmw_set (AR7240_GPIO_OUT, 0x1000);
    }
    else {
        ar7240_reg_rmw_clear (AR7240_GPIO_OUT, 0x1000);
    }
}

#ifdef CONFIG_GPIO_CUSTOM
int gpio_custom (int opcode)
{
	int rcode = 0;
	static int cmd = -1;

	switch (opcode) {
		case 0:
			if ((ar7240_reg_rd (AR7240_GPIO_IN) & 0x2) == 0) {
				cmd = 0;
			}
			else {
				cmd = -1;
			}
			rcode = cmd >= 0;
			break;
		case 1:
			switch (cmd) {
				case 0:
				{
					char key[] = "netretry", *val = NULL, *s = NULL;
					int led[] = {13, 15, 14};
					int i = -1, j = 0;

					if ((s = getenv (key)) != NULL && (val = malloc (strlen (s) + 1)) != NULL) {
						strcpy (val, s);
					}
					setenv (key, "no");
					s = "boot 0";
					for (i = -1; ++i < 36; udelay (25000)) {
						if ((j = i % 6) < 3) {
							ar7240_reg_rmw_set (AR7240_GPIO_OUT, 1 << led[j]);
						}
						else {
							ar7240_reg_rmw_clear (AR7240_GPIO_OUT, 1 << led[5 - j]);
						}
					}
					for (i = 3; i-- > 0 && run_command (s, 0) == -1;) {
						printf ("Retry%s\n", i > 0? "...": " count exceeded!");
						if (i <= 0) {
							run_command ("boot", 0);
						}
					}
					if (val != NULL) {
						setenv (key, val);
					}
					break;
				}
				default:
					break;
			}
			cmd = -1;
			break;
		default:
			break;
	}

	return rcode;
}
#endif
