LDSCRIPT := $(SRCTREE)/board/$(VENDOR)/$(BOARD)/u-boot.lds

sinclude $(OBJTREE)/board/$(VENDOR)/$(BOARD)/config.tmp

ifndef TEXT_BASE
	TEXT_BASE = 0x27800000
endif

ifdef CONFIG_MX6Q_ARM2_LPDDR2POP
	TEXT_BASE = 0x10800000
endif
