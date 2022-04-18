obj-$(CONFIG_USB_CDNS3)		+= cdns3.o

cdns3-y				:= core.o
cdns3-$(CONFIG_USB_CDNS3_GADGET)	+= gadget.o
cdns3-$(CONFIG_$(SPL_)DM_USB_GADGET)	+= cdns3-generic.o
