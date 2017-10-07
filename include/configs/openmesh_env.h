#ifndef __OPENMESH_ENV_H
#define __OPENMESH_ENV_H


#undef CONFIG_IPADDR
#define CONFIG_IPADDR			192.168.100.9

#undef CONFIG_SERVERIP
#define CONFIG_SERVERIP			192.168.100.8

#undef CONFIG_BOOTARGS
#define	CONFIG_BOOTARGS		"console=ttyS0,115200 rootfstype=squashfs,jffs2 init=/etc/preinit board=OM5P-ACv2"

#undef CONFIG_BOOTCOMMAND

#define	CONFIG_OPENMESH_EXTRA_ENV_SETTINGS					\
		"bootcmd_1=run imagechk && bootm 0x9f0b0000 \0"		\
		"bootcmd_2=run imagechk && bootm 0x9f850000 \0"		\
		"preboot=flashit 0x80100000 fwupgrade.cfg \0"		\
		"imagechk=test -n \"${check_skip}\" || check_skip=1 && datachk vmlinux,rootfs	\0"	\
		"bootcmd_0=tftp 0x81000000 vmlinux-initramfs.bin && bootm 0x81000000 \0"	\
		"bootargs_0=${bootargs} root=31:04 mtdparts=spi0.0:256k(u-boot),64k(u-boot-env),384k(custom),1280k(kernel),6528k(rootfs),7808k(inactive),64k(ART)\0" 	\
		"set_bootargs_1=set bootargs_1 ${bootargs} root=31:04 mtdparts=spi0.0:256k(u-boot),64k(u-boot-env),384k(custom),${kernel_size_1}k(kernel),${rootfs_size_1}k(rootfs),7808k(inactive),64k(ART)\0" 	\
		"set_bootargs_2=set bootargs_2 ${bootargs} root=31:05 mtdparts=spi0.0:256k(u-boot),64k(u-boot-env),384k(custom),7808k(inactive),${kernel_size_2}k(kernel),${rootfs_size_2}k(rootfs),64k(ART)\0" 	\
		"bootcmd=test -n \"${preboot}\" && run preboot; test -n \"${bootseq}\" || bootseq=1,2; run set_bootargs_1; run set_bootargs_2; boot \"${bootseq}\" \0"		\
	""
#define CONFIG_NET_RETRY_COUNT 1
#define CONFIG_NET_TIMEOUT 1
#endif	/* __OPENMESH_ENV_H */
