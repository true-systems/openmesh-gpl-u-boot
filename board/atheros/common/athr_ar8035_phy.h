
#ifndef _AR8035_PHY
#define _AR8035_PHY


#define AR8035_PHYID1   0x4d
#define AR8035_PHYID2   0xd072
/*****************/
/* PHY Registers */
/*****************/
#define AR8035_PHY_CONTROL                 0x00
#define AR8035_PHY_STATUS                  0x01
#define AR8035_PHY_ID1                     0x02
#define AR8035_PHY_ID2                     0x03
#define AR8035_AUTONEG_ADVERT              0x04
#define AR8035_LINK_PARTNER_ABILITY        0x05
#define AR8035_AUTONEG_EXPANSION           0x06
#define AR8035_NEXT_PAGE_TRANSMIT          0x07
#define AR8035_LINK_PARTNER_NEXT_PAGE      0x08
#define AR8035_1000BASET_CONTROL           0x09
#define AR8035_1000BASET_STATUS            0x0A
#define AR8035_PHY_SPEC_CONTROL            0x10
#define AR8035_PHY_SPEC_STATUS             0x11
#define AR8035_PHY_INTR_ENABLE             0x12
#define AR8035_PHY_INTR_STATUS            0x11
#define AR8035_DEBUG_PORT_ADDRESS          0x1D
#define AR8035_DEBUG_PORT_DATA             0x1E


/* AR8035_PHY_CONTROL fields */
#define AR8035_CTRL_SOFTWARE_RESET                    0x8000
#define AR8035_CTRL_SPEED_LSB                         0x2000
#define AR8035_CTRL_AUTONEGOTIATION_ENABLE            0x1000
#define AR8035_CTRL_RESTART_AUTONEGOTIATION           0x0200
#define AR8035_CTRL_SPEED_FULL_DUPLEX                 0x0100
#define AR8035_CTRL_SPEED_MSB                         0x0040

#define AR8035_RESET_DONE(phy_control)                   \
    (((phy_control) & (AR8035_CTRL_SOFTWARE_RESET)) == 0)
    
/* Phy status fields */
#define AR8035_STATUS_AUTO_NEG_DONE                   0x0020

#define AR8035_AUTONEG_DONE(ip_phy_status)                   \
    (((ip_phy_status) &                                  \
        (AR8035_STATUS_AUTO_NEG_DONE)) ==                    \
        (AR8035_STATUS_AUTO_NEG_DONE))

/* Link Partner ability */
#define AR8035_LINK_100BASETX_FULL_DUPLEX       0x0100
#define AR8035_LINK_100BASETX                   0x0080
#define AR8035_LINK_10BASETX_FULL_DUPLEX        0x0040
#define AR8035_LINK_10BASETX                    0x0020

/* Advertisement register. */
#define AR8035_ADVERTISE_NEXT_PAGE              0x8000
#define AR8035_ADVERTISE_ASYM_PAUSE             0x0800
#define AR8035_ADVERTISE_PAUSE                  0x0400
#define AR8035_ADVERTISE_100FULL                0x0100
#define AR8035_ADVERTISE_100HALF                0x0080  
#define AR8035_ADVERTISE_10FULL                 0x0040  
#define AR8035_ADVERTISE_10HALF                 0x0020  

#define AR8035_ADVERTISE_ALL (AR8035_ADVERTISE_ASYM_PAUSE | AR8035_ADVERTISE_PAUSE | \
                            AR8035_ADVERTISE_10HALF | AR8035_ADVERTISE_10FULL | \
                            AR8035_ADVERTISE_100HALF | AR8035_ADVERTISE_100FULL)
                       
/* 1000BASET_CONTROL */
#define AR8035_ADVERTISE_1000FULL              0x0200

/* Phy Specific status fields */
#define AR8035_STATUS_LINK_MASK                0xC000
#define AR8035_STATUS_LINK_SHIFT               14
#define AR8035_STATUS_FULL_DEPLEX              0x2000
#define AR8035_STATUS_LINK_PASS                 0x0400 
#define AR8035_STATUS_RESOVLED                  0x0800

/*phy debug port  register */
#define AR8035_DEBUG_SERDES_REG                5

/* Serdes debug fields */
#define AR8035_SERDES_BEACON                   0x0100

#ifndef BOOL
#define BOOL    int
#endif

int athr_ar8035_phy_setup(int ethUnit);
int athr_ar8035_phy_is_up(int unit);
int athr_ar8035_phy_is_fdx(int ethUnit);
int athr_ar8035_phy_speed(int ethUnit);
int athrs_athr_ar8035_phy_register_ops(void *arg);

#endif //_AR8035_PHY


