# -- Path --
TOOLPATH = tool
INCPATH = tool/haribote
OUT_DIR = out
APP_OUT_DIR = out/app
IMG_DIR = img

# -- Tools --
NASK = $(TOOLPATH)/nask
CC1 = $(TOOLPATH)/gocc1 -I$(INCPATH) -Os -Wall -quiet
GAS2NASK = $(TOOLPATH)/gas2nask -a
OBJ2BIM = $(TOOLPATH)/obj2bim
MAKEFONT = $(TOOLPATH)/makefont
BIN2OBJ = $(TOOLPATH)/bin2obj
BIM2HRB = $(TOOLPATH)/bim2hrb
RULEFILE = $(TOOLPATH)/haribote/haribote.rul
EDIMG = $(TOOLPATH)/edimg
FDIMG2ISO = $(TOOLPATH)/makeiso/fdimg2iso
QEMU = qemu-system-i386

COPY = cp
DEL = rm -f

# -- Source Path --
VPATH = src/kernel:src/boot:src/asm:src/graphics/font:src/app

# -- APP --
# (.c -> .hrb)
SRC_APP_C   = $(wildcard src/app/*.c)
# (.nas -> .hrb)
APP_TARGETS = $(patsubst src/app/%.c, $(APP_OUT_DIR)/%.hrb, $(SRC_APP_C))

# -- Kernel Objects --
OBJS_BOOTPACK = $(OUT_DIR)/bootpack.obj $(OUT_DIR)/naskfunc.obj $(OUT_DIR)/hankaku.obj $(OUT_DIR)/graphic.obj $(OUT_DIR)/dsctbl.obj \
				$(OUT_DIR)/int.obj $(OUT_DIR)/fifo.obj $(OUT_DIR)/keyboard.obj $(OUT_DIR)/mouse.obj $(OUT_DIR)/memory.obj $(OUT_DIR)/sheet.obj \
				$(OUT_DIR)/timer.obj $(OUT_DIR)/mtask.obj $(OUT_DIR)/window.obj $(OUT_DIR)/console.obj $(OUT_DIR)/file.obj

# -- Build Rule --
default :
	@mkdir -p $(OUT_DIR) $(APP_OUT_DIR) $(IMG_DIR)
	$(MAKE) $(IMG_DIR)/haribote.img

# -- Kernel Build --
$(OUT_DIR)/bootpack.bim : $(OBJS_BOOTPACK)
	$(OBJ2BIM) @$(RULEFILE) out:$@ stack:3136k map:$(subst .bim,.map,$@) $(OBJS_BOOTPACK)

$(OUT_DIR)/bootpack.hrb : $(OUT_DIR)/bootpack.bim
	$(BIM2HRB) $< $@ 0

# -- Application Build --
$(APP_OUT_DIR)/%.obj: src/app/%.c
	$(CC1) -o $(APP_OUT_DIR)/$*.gas $<
	$(GAS2NASK) $(APP_OUT_DIR)/$*.gas $(APP_OUT_DIR)/$*.nas
	$(NASK) $(APP_OUT_DIR)/$*.nas $@ $(APP_OUT_DIR)/$*.lst

$(APP_OUT_DIR)/%.bim: $(APP_OUT_DIR)/%.obj $(OUT_DIR)/app_nask.obj
	$(OBJ2BIM) @$(RULEFILE) out:$@ map:$(subst .bim,.map,$@) $(APP_OUT_DIR)/$*.obj $(OUT_DIR)/app_nask.obj

$(APP_OUT_DIR)/%.hrb: $(APP_OUT_DIR)/%.bim
	$(BIM2HRB) $< $@ 0

# -- Common Object Build --
$(OUT_DIR)/%.obj: %.c
	$(CC1) -o $(OUT_DIR)/$*.gas $<
	$(GAS2NASK) $(OUT_DIR)/$*.gas $(OUT_DIR)/$*.nas
	$(NASK) $(OUT_DIR)/$*.nas $@ $(OUT_DIR)/$*.lst

$(OUT_DIR)/%.obj: %.nas
	$(NASK) $< $@ $(subst .obj,.lst,$@)

# -- Font Build --
$(OUT_DIR)/hankaku.bin: src/graphics/font/hankaku.txt
	$(MAKEFONT) $< $@

$(OUT_DIR)/hankaku.obj: $(OUT_DIR)/hankaku.bin
	$(BIN2OBJ) $< $@ _hankaku

# -- Boot Loader Build --
$(OUT_DIR)/ipl10.bin: src/boot/ipl10.nas
	$(NASK) $< $@ $(subst .bin,.lst,$@)

$(OUT_DIR)/asmhead.bin: src/boot/asmhead.nas
	$(NASK) $< $@ $(subst .bin,.lst,$@)

# -- Image Build --
$(IMG_DIR)/haribote.sys: $(OUT_DIR)/asmhead.bin $(OUT_DIR)/bootpack.hrb
	cat $^ > $@

$(IMG_DIR)/haribote.img: $(OUT_DIR)/ipl10.bin $(IMG_DIR)/haribote.sys $(APP_TARGETS)
	$(EDIMG) imgin:$(TOOLPATH)/fdimg0at.tek \
		wbinimg src:$(OUT_DIR)/ipl10.bin len:512 from:0 to:0 \
		copy from:$(IMG_DIR)/haribote.sys to:@: \
		$(foreach app, $(APP_TARGETS), copy from:$(app) to:@: ) \
		imgout:$@

iso : $(IMG_DIR)/haribote.img
	$(FDIMG2ISO) $(TOOLPATH)/makeiso/fdimg2iso.dat $(IMG_DIR)/haribote.img $(IMG_DIR)/haribote.iso

run: $(IMG_DIR)/haribote.img
	$(QEMU) -m 128M -drive format=raw,if=floppy,file=$(IMG_DIR)/haribote.img -no-reboot -d int
clean :
	rm -rf $(OUT_DIR) $(IMG_DIR)
