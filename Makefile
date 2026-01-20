# -- Path --
TOOLPATH = tool
INCPATH = tool/haribote
OUT_DIR = out
APP_OUT_DIR = out/app
IMG_DIR = img

KOREAN_FONT = src/graphics/font/H04.FNT

# -- Tools --
NASK = $(TOOLPATH)/nask
CC1 = $(TOOLPATH)/gocc1 -I$(INCPATH) -Os -Wall -quiet
GAS2NASK = $(TOOLPATH)/gas2nask -a
OBJ2BIM = $(TOOLPATH)/obj2bim
MAKEFONT = $(TOOLPATH)/makefont
BIN2OBJ = $(TOOLPATH)/bin2obj
BIM2HRB = $(TOOLPATH)/bim2hrb
BIM2BIN = $(TOOLPATH)/bim2bin
RULEFILE = $(TOOLPATH)/haribote/haribote.rul
EDIMG = $(TOOLPATH)/edimg
FDIMG2ISO = $(TOOLPATH)/makeiso/fdimg2iso
GOLIB = $(TOOLPATH)/golib00

QEMU = qemu-system-i386

COPY = cp
DEL = rm -f

# -- Source Path --
VPATH = src/kernel:src/boot:src/graphics/font:app/src:app/api

# -- APP API --
API_SRC = $(wildcard app/api/api*.nas) app/api/alloca.nas
API_OBJS = $(patsubst app/api/%.nas, $(APP_OUT_DIR)/api/%.obj, $(API_SRC))
API_LIB = $(APP_OUT_DIR)/api/apilib.lib

# -- Application Targets --
APP_SRC_C   = $(wildcard app/src/*.c)
APP_TARGETS = $(patsubst app/src/%.c, $(APP_OUT_DIR)/%.hrb, $(APP_SRC_C))

# -- Kernel Objects --
OBJS_BOOTPACK = $(OUT_DIR)/bootpack.obj $(OUT_DIR)/naskfunc.obj $(OUT_DIR)/hankaku.obj $(OUT_DIR)/graphic.obj $(OUT_DIR)/dsctbl.obj \
				$(OUT_DIR)/int.obj $(OUT_DIR)/fifo.obj $(OUT_DIR)/keyboard.obj $(OUT_DIR)/mouse.obj $(OUT_DIR)/memory.obj $(OUT_DIR)/sheet.obj \
				$(OUT_DIR)/timer.obj $(OUT_DIR)/mtask.obj $(OUT_DIR)/window.obj $(OUT_DIR)/console.obj $(OUT_DIR)/file.obj $(OUT_DIR)/tek.obj \
				$(OUT_DIR)/hangul.obj


# -- Build Rule --
default :
	@mkdir -p $(OUT_DIR) $(APP_OUT_DIR)/api $(IMG_DIR)
	$(MAKE) $(IMG_DIR)/haribote.img

# -- API Lib Build --
$(API_LIB) : $(API_OBJS)
	@mkdir -p $(APP_OUT_DIR)/api
	$(GOLIB) $(API_OBJS) out:$@

# -- Default Memory Size --
STACK_DEFAULT = 1k
MALLOC_DEFAULT = 0k

# -- APP Specific Memory Size --
STACK_bball = 52k
STACK_invader = 90k
STACK_calc = 4k
STACK_gview = 4480k

MALLOC_beepdown = 40k
MALLOC_color = 56k
MALLOC_invader = 48k
MALLOC_timer = 40k
MALLOC_walk = 48k

# -- Application Build --
$(APP_OUT_DIR)/%/%.gas : app/src/%.c
	@mkdir -p $(APP_OUT_DIR)/$*
	$(CC1) -o $@ $<

$(APP_OUT_DIR)/%/%.nas : $(APP_OUT_DIR)/%/%.gas
	$(GAS2NASK) $< $@

$(APP_OUT_DIR)/%/%.obj : $(APP_OUT_DIR)/%/%.nas
	$(NASK) $< $@ $(APP_OUT_DIR)/$*/$*.lst

$(APP_OUT_DIR)/%/%.bim : $(APP_OUT_DIR)/%/%.obj $(API_LIB)
	$(OBJ2BIM) @$(RULEFILE) out:$@ map:$(APP_OUT_DIR)/$*/$*.map stack:$(if $(STACK_$*),$(STACK_$*),$(STACK_DEFAULT)) $< $(API_LIB)

$(APP_OUT_DIR)/%/%.org : $(APP_OUT_DIR)/%/%.bim
	$(BIM2HRB) $< $@ $(if $(MALLOC_$*),$(MALLOC_$*),$(MALLOC_DEFAULT))

$(APP_OUT_DIR)/%.hrb : $(APP_OUT_DIR)/%/%.org
	$(BIM2BIN) -osacmp in:$< out:$@

# -- Kernel Build --
$(OUT_DIR)/bootpack.bim : $(OBJS_BOOTPACK)
	$(OBJ2BIM) @$(RULEFILE) out:$@ stack:3136k map:$(subst .bim,.map,$@) $(OBJS_BOOTPACK)

$(OUT_DIR)/bootpack.hrb : $(OUT_DIR)/bootpack.bim
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
$(OUT_DIR)/ipl.bin: src/boot/ipl.nas
	$(NASK) $< $@ $(subst .bin,.lst,$@)

$(OUT_DIR)/asmhead.bin: src/boot/asmhead.nas
	$(NASK) $< $@ $(subst .bin,.lst,$@)

# -- Image Build --
$(IMG_DIR)/haribote.sys: $(OUT_DIR)/asmhead.bin $(OUT_DIR)/bootpack.hrb
	cat $^ > $@

$(IMG_DIR)/haribote.img: $(OUT_DIR)/ipl.bin $(IMG_DIR)/haribote.sys $(APP_TARGETS) $(KOREAN_FONT)
	$(EDIMG) imgin:$(TOOLPATH)/fdimg0at.tek \
		wbinimg src:$(OUT_DIR)/ipl.bin len:512 from:0 to:0 \
		copy from:$(IMG_DIR)/haribote.sys to:@: \
		copy from:$(KOREAN_FONT) to:@: \
		$(foreach app, $(APP_TARGETS), copy from:$(app) to:@: ) \
		copy from:fujisan.jpg to:@: \
		imgout:$@

iso : $(IMG_DIR)/haribote.img
	$(FDIMG2ISO) $(TOOLPATH)/makeiso/fdimg2iso.dat $(IMG_DIR)/haribote.img $(IMG_DIR)/haribote.iso

run: $(IMG_DIR)/haribote.img
	$(QEMU) -m 128M -drive format=raw,if=floppy,file=$(IMG_DIR)/haribote.img -no-reboot -d int

clean :
	rm -rf $(OUT_DIR) $(IMG_DIR)
