/*
 * Copyright (c) 2010, Atheros Communications Inc.
 * 
 * Modified by cfho (cfho@edimax.com.tw) 2013-June
 * 
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */


#include <config.h>
#include <linux/types.h>
#include <common.h>
#include <miiphy.h>
#include "phy.h"
#include <asm/addrspace.h>
#include <atheros.h>
#include "athr_ar8035_phy.h"

#define TRUE 1
#define FALSE 0

#define sysMsDelay(_x) udelay((_x) * 1000)

#define DRV_PRINT(FLG, X) 
#define AR8035_PHY0_ADDR   0x0
#define AR8035_PHY1_ADDR   0x1
#define AR8035_PHY2_ADDR   0x2
#define AR8035_PHY3_ADDR   0x3
#define AR8035_PHY4_ADDR   0x4

#define MODULE_NAME "AR8035"
BOOL athr_ar8035_phy_is_link_alive(int ethUnit);

/*
 * Track per-PHY port information.
 */
typedef struct {
    BOOL   isEnetPort;       /* normal enet port */
    BOOL   isPhyAlive;       /* last known state of link */
    int    ethUnit;          /* MAC associated with this phy port */
    uint32_t phyBase;
    uint32_t phyAddr;          /* PHY registers associated with this phy port */
} athrPhyInfo_t;


/*
 * Per-PHY information, indexed by PHY unit number.
 */
static athrPhyInfo_t athrPhyInfo[] = {
    {
                TRUE,   /* phy port 4 -- WAN port or LAN port 4 */
     FALSE,
     0,
     0,
     AR8035_PHY4_ADDR, 
    },
};


#define AR8035_PHY_MAX 1

/* Convenience macros to access myPhyInfo */
#define AR8035_IS_ENET_PORT(phyUnit) (athrPhyInfo[phyUnit].isEnetPort)
#define AR8035_IS_PHY_ALIVE(phyUnit) (athrPhyInfo[phyUnit].isPhyAlive)
#define AR8035_ETHUNIT(phyUnit) (athrPhyInfo[phyUnit].ethUnit)
#define AR8035_PHYBASE(phyUnit) (athrPhyInfo[phyUnit].phyBase)
#define AR8035_PHYADDR(phyUnit) (athrPhyInfo[phyUnit].phyAddr)

#define AR8035_IS_ETHUNIT(phyUnit, ethUnit) \
            (AR8035_IS_ENET_PORT(phyUnit) &&        \
            AR8035_ETHUNIT(phyUnit) == (ethUnit))
            
void
athr_ar8035_mgmt_init(void)
{
     

}

int
athr_ar8035_phy_setup(int ethUnit)
{
     int phyid1, phyid2;
    uint16_t    phyHwStatus;
    uint16_t    timeout;
    int         liveLinks = 0;
    uint32_t    phyBase = 0;
    uint32_t    phyAddr = 0;
    uint16_t        regval = 0;
 
    int phyUnit=ethUnit;
        
    printf("AR8035 found!\n");
          DRV_PRINT(DRV_DEBUG_PHYSETUP,("%s:: ether unit %d\n", __func__, phyUnit));
    /* See if there's any configuration data for this enet */
    /* start auto negogiation on each phy */

    phyBase = AR8035_PHYBASE(phyUnit);
    phyAddr = AR8035_PHYADDR(phyUnit);

    phyid1 = phy_reg_read(phyBase, phyAddr, AR8035_PHY_ID1);
    phyid2 = phy_reg_read(phyBase, phyAddr, AR8035_PHY_ID2);
        printf("[%d:%d]Phy ID %x:%x\n", phyBase, phyAddr, phyid1, phyid2);
 
    if((phyid1 == 0xff && phyid2 == 0xff))
    {
            printf("PHY ID for AR8035 not found! HW Error!\n");
                return FALSE;
    }


    phy_reg_write(0, phyAddr, AR8035_AUTONEG_ADVERT,
                  AR8035_ADVERTISE_ALL);

    phy_reg_write(phyBase, phyAddr, AR8035_1000BASET_CONTROL,
                  AR8035_ADVERTISE_1000FULL);


    /* delay tx_clk */
    phy_reg_write(0, phyAddr, AR8035_DEBUG_PORT_ADDRESS, 0x5);
        regval=phy_reg_read(phyBase, phyAddr, AR8035_DEBUG_PORT_DATA);
//      printf("[yp debug]: now tx_clk = 0x%x\n",regval);
    phy_reg_write(0, phyAddr, AR8035_DEBUG_PORT_DATA, 0x100);
        regval=phy_reg_read(phyBase, phyAddr, AR8035_DEBUG_PORT_DATA);
//      printf("[yp debug]: now tx_clk = 0x%x\n",regval);


    /* Reset PHYs*/
    phy_reg_write(0, phyAddr, AR8035_PHY_CONTROL,
                  AR8035_CTRL_AUTONEGOTIATION_ENABLE
                  | AR8035_CTRL_SOFTWARE_RESET);

    /*
     * After the phy is reset, it takes a little while before
     * it can respond properly.
     */
    sysMsDelay(500);
    
    /*
     * Wait up to .75 seconds for ALL associated PHYs to finish
     * autonegotiation.  The only way we get out of here sooner is
     * if ALL PHYs are connected AND finish autonegotiation.
     */

    timeout=30;
    for (;;) {
        phyHwStatus=phy_reg_read(phyBase, phyAddr, AR8035_PHY_CONTROL);

        if (AR8035_RESET_DONE(phyHwStatus)) {
            printf("Port %d, Neg Success\n", phyUnit);
            break;
        }
        if (timeout == 0) {
            
            printf("Port %d, Negogiation timeout\n", phyUnit);
            break;
        }
        if (--timeout == 0) {
            printf("Port %d, Negogiation timeout\n", phyUnit);
            break;
        }

        sysMsDelay(200);
    }

    /*
     * PHY have had adequate time to autonegotiate.
     * Now initialize software status.
     *
     * It's possible that some ports may take a bit longer
     * to autonegotiate; but we can't wait forever.  They'll
     * get noticed by mv_phyCheckStatusChange during regular
     * polling activities.
     */
    if (athr_ar8035_phy_is_link_alive(phyUnit)) {
        liveLinks++;
        AR8035_IS_PHY_ALIVE(phyUnit) = TRUE;
    } else {
        AR8035_IS_PHY_ALIVE(phyUnit) = FALSE;
    }

    return (liveLinks > 0);
}


/******************************************************************************
*
* ar8035_phy_is_fdx - Determines whether the phy ports associated with the
* specified device are FULL or HALF duplex.
*
* RETURNS:
*    1 --> FULL
*    0 --> HALF
*/
int athr_ar8035_phy_is_fdx(int ethUnit)
{
        
    uint32_t    phyBase;
    uint32_t    phyAddr;
    uint16_t    phyHwStatus;
    int         ii = 200;
    uint32_t    phyUnit=0;
    
    
    if (athr_ar8035_phy_is_link_alive(phyUnit)) {

        phyBase = AR8035_PHYBASE(phyUnit);
        phyAddr = AR8035_PHYADDR(phyUnit);

        do {
             phyHwStatus=phy_reg_read(phyBase, phyAddr, AR8035_PHY_SPEC_STATUS);
                                sysMsDelay(10);
        } while((!(phyHwStatus & AR8035_STATUS_RESOVLED)) && --ii);

        if (phyHwStatus & AR8035_STATUS_FULL_DEPLEX)
            return TRUE;
    }

    return FALSE;

}

int
athr_ar8035_phy_is_link_alive(int phyUnit)
{
//    int     phyUnit=ethUnit;
    uint16_t phyHwStatus;
    uint32_t phyBase;
    uint32_t phyAddr;
    int ii=20;

    phyBase = AR8035_PHYBASE(phyUnit);
    phyAddr = AR8035_PHYADDR(phyUnit);
     
        
    do {
             phyHwStatus=phy_reg_read(phyBase, phyAddr, AR8035_PHY_SPEC_STATUS);
             
            sysMsDelay(100);
        }while((!(phyHwStatus & AR8035_STATUS_LINK_PASS)) && --ii);

    if (phyHwStatus & AR8035_STATUS_LINK_PASS)
        return TRUE;
    else
        return FALSE;
}

  int
athr_ar8035_phy_is_up(int ethUnit)
{
        int phyUnit=ethUnit;
    uint16_t      phyHwStatus, phyHwControl;
    athrPhyInfo_t *lastStatus;
    int           linkCount   = 0;
    int           lostLinks   = 0;
    int           gainedLinks = 0;
    uint32_t      phyBase;
    uint32_t      phyAddr;


    phyBase = AR8035_PHYBASE(phyUnit);
    phyAddr = AR8035_PHYADDR(phyUnit);

    lastStatus = &athrPhyInfo[phyUnit];

    if (lastStatus->isPhyAlive) { /* last known link status was ALIVE */
        phyHwStatus=phy_reg_read(phyBase, phyAddr, AR8035_PHY_SPEC_STATUS);

        /* See if we've lost link */
        if (phyHwStatus & AR8035_STATUS_LINK_PASS) {
                        DRV_PRINT(DRV_DEBUG_PHYCHANGE,("\nenet%d port%d upcnt ++\n",
                                           phyUnit, phyUnit));
            linkCount++;
        } else {
            lostLinks++;
            DRV_PRINT(DRV_DEBUG_PHYCHANGE,("\nenet%d port%d down\n",
                                           phyUnit, phyUnit));
            lastStatus->isPhyAlive = FALSE;
        }
    } else { /* last known link status was DEAD */
        /* Check for reset complete */
        phyHwStatus=phy_reg_read(phyBase, phyAddr, AR8035_PHY_STATUS);
        if (!AR8035_RESET_DONE(phyHwStatus))
                                                return (linkCount);

        phyHwControl=phy_reg_read(phyBase, phyAddr, AR8035_PHY_CONTROL);
        /* Check for AutoNegotiation complete */            
        if ((!(phyHwControl & AR8035_CTRL_AUTONEGOTIATION_ENABLE)) 
             || AR8035_AUTONEG_DONE(phyHwStatus)) {
                
            phyHwStatus=phy_reg_read(phyBase, phyAddr, 
                                       AR8035_PHY_SPEC_STATUS);

            if (phyHwStatus & AR8035_STATUS_LINK_PASS) {
                            gainedLinks++;
                            linkCount++;
                            DRV_PRINT(DRV_DEBUG_PHYCHANGE,("\nenet%d port%d up\n",
                                                           phyUnit, phyUnit));
                            lastStatus->isPhyAlive = TRUE;
            }
        }
    }

    return (linkCount);
}
int
athr_ar8035_phy_speed(int ethUnit)
{
    uint32_t    phyUnit= ethUnit;
    uint16_t    phyHwStatus;
    uint32_t    phyBase;
    uint32_t    phyAddr;
    int         ii = 2;

    if (athr_ar8035_phy_is_link_alive(phyUnit)) {

        phyBase = AR8035_PHYBASE(phyUnit);
        phyAddr = AR8035_PHYADDR(phyUnit);
        
        do {
            phyHwStatus=phy_reg_read(phyBase, phyAddr, 
                                          AR8035_PHY_SPEC_STATUS);
            sysMsDelay(10);
        }while((!(phyHwStatus & AR8035_STATUS_RESOVLED)) && --ii);

        phyHwStatus = ((phyHwStatus & AR8035_STATUS_LINK_MASK) >>
                                AR8035_STATUS_LINK_SHIFT);

        switch(phyHwStatus) {
        case 0:
            printf("Speed is 10T\n");
                        return  _10BASET;
        case 1:
            printf("Speed is 100TX\n");
                        return _100BASET;
        case 2:
            printf("Speed is 1000T\n");
                        return  _1000BASET;
        default:
            printf("Unkown speed read! %d\n",phyHwStatus);
        }
    }
 return -1;
}


int 
athr_ar8035_reg_init(void *arg)
{


	athr_ar8035_mgmt_init();
// 	phy_reg_write(0x1,0x5, 0x1f, 0x101);
	//printf("%s: Done %x \n",__func__, phy_reg_read(0x1,0x5,0x1f));
   
	return 0;
}

