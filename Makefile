ifneq ($(strip $(shell firmtool -v 2>&1 | grep usage)),)
$(error "Please install firmtool v1.1 or greater")
endif

NAME		:=	$(notdir $(CURDIR))
REVISION	:=	v10.2.1 ESP

SUBFOLDERS	:=	sysmodules arm11 arm9 k11_extension

.PHONY:	all release clean $(SUBFOLDERS)

all:		boot.firm

release:	$(NAME)$(REVISION).zip

clean:
	@$(foreach dir, $(SUBFOLDERS), $(MAKE) -C $(dir) clean &&) true
	@rm -rf *.firm *.zip *.3dsx

# boot.3dsx comes from https://github.com/fincs/new-hbmenu/releases
$(NAME)$(REVISION).zip:	boot.firm boot.3dsx
	@zip -r $@ $^ -x "*.DS_Store*" "*__MACOSX*"

boot.firm:	$(SUBFOLDERS)
	@firmtool build $@ -D sysmodules/sysmodules.bin arm11/arm11.elf arm9/arm9.elf k11_extension/k11_extension.elf \
	-A 0x18180000 -C XDMA XDMA NDMA XDMA
	@echo built... $(notdir $@)

boot.3dsx:
	@curl -sSL "https://github.com/fincs/new-hbmenu/releases/latest/download/boot.3dsx" -o "$@"
	@echo downloaded... $(notdir $@)

$(SUBFOLDERS):
	@$(MAKE) -C $@ all
