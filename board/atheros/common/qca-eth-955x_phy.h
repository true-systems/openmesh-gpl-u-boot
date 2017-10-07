#ifndef _QCA_ETH_955x_PHY_H
#define _QCA_ETH_955x_PHY_H
#include <miiphy.h>


#ifdef CONFIG_ATHR_8033_PHY
extern int athrs_ar8033_reg_init(void *arg);
extern int athrs_ar8033_phy_setup(void  *arg);
extern int athrs_ar8033_phy_is_fdx(int ethUnit);
extern int athrs_ar8033_phy_is_link_alive(int phyUnit);
extern int athrs_ar8033_phy_is_up(int ethUnit);
extern int athrs_ar8033_phy_speed(int ethUnit,int phyUnit);
#endif

#ifdef CONFIG_ATHRS17_PHY
extern int athrs17_phy_setup(int ethUnit);
extern int athrs17_phy_is_up(int ethUnit);
extern int athrs17_phy_is_fdx(int ethUnit);
extern int athrs17_phy_speed(int ethUnit);
#endif

static inline void ath_gmac_phy_setup(int unit)
{
#ifdef CONFIG_ATHRS17_PHY
		if (unit == 0) {
			athrs17_phy_setup(unit);
		} else
#endif
#ifdef CONFIG_ATHR_8035_PHY
			if (unit == 0) {
                        athr_ar8035_phy_setup(unit);
			}else 
#endif			
		{
#ifdef CONFIG_VIR_PHY
			athr_vir_phy_setup(unit);
#endif
                        
#ifdef CONFIG_ATHR_8033_PHY
                        athrs_ar8033_phy_setup(unit);
#endif
#if defined(CONFIG_ATHRS17_PHY) && !defined (CFG_DUAL_PHY_SUPPORT)
			athrs17_phy_setup(unit);
#endif

		}
}

static inline void ath_gmac_phy_link(int unit, int *link)
{
#if defined (CONFIG_ATHRS17_PHY) || defined (CONFIG_ATHR_8035_PHY)
		if (unit == 0) {
#ifdef CONFIG_ATHRS17_PHY                    
			*link = athrs17_phy_is_up(unit);
#endif                        
#ifdef CONFIG_ATHR_8035_PHY                    
                        *link = athr_ar8035_phy_is_up(unit);
#endif                    
                        
		} else
#endif
		{
#ifdef CONFIG_VIR_PHY
			*link = athr_vir_phy_is_up(unit);
#endif
#ifdef CONFIG_ATHR_8033_PHY
			*link = athrs_ar8033_phy_is_up(unit);
#endif
#if defined(CONFIG_ATHRS17_PHY) && !defined (CFG_DUAL_PHY_SUPPORT)
			*link = athrs17_phy_is_up(unit);
#endif

		}
}

static inline void ath_gmac_phy_duplex(int unit, int *duplex)
{
#if defined (CONFIG_ATHRS17_PHY) || defined (CONFIG_ATHR_8035_PHY)
                if (unit == 0) {
#ifdef CONFIG_ATHRS17_PHY                    
                        *duplex = athrs17_phy_is_fdx(unit);
#endif                        
#ifdef CONFIG_ATHR_8035_PHY                    
                        *duplex = athr_ar8035_phy_is_fdx(unit);
#endif                    
                        
                } else
#endif
		{
#ifdef CONFIG_VIR_PHY
			*duplex = athr_vir_phy_is_fdx(unit);
#endif
#ifdef CONFIG_ATHR_8033_PHY
			*duplex = athrs_ar8033_phy_is_fdx(unit);
#endif
#if defined(CONFIG_ATHRS17_PHY) && !defined(CFG_DUAL_PHY_SUPPORT)
			*duplex = athrs17_phy_is_fdx(unit);
#endif
		}
}

static inline void ath_gmac_phy_speed(int unit, int *speed)
{
#if defined (CONFIG_ATHRS17_PHY) || defined (CONFIG_ATHR_8035_PHY)
                if (unit == 0) {
#ifdef CONFIG_ATHRS17_PHY                    
                        *speed = _1000BASET;
#endif                        
#ifdef CONFIG_ATHR_8035_PHY                    
                        *speed = athr_ar8035_phy_speed(unit);
#endif                    
                        
                } else
#endif
		{
#ifdef CONFIG_VIR_PHY
			*speed = athr_vir_phy_speed(unit);
#endif
#ifdef CONFIG_ATHR_8033_PHY
			*speed = athrs_ar8033_phy_speed(unit, 5);
#endif

#if defined(CONFIG_ATHRS17_PHY) && !defined (CFG_DUAL_PHY_SUPPORT)
			*speed = _1000BASET;
#endif
		}
}

#endif /* _QCA_ETH_955x_PHY_H */
