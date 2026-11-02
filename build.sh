#!/bin/sh
# Build the Spooky Ghost section for Halo 2 (Xbox) v1.0 and v1.5: 32-bit, freestanding, no SSE
set -e
cd "$(dirname "$0")"
CFLAGS="-m32 -march=i686 -mno-sse -mno-mmx -mfpmath=387 -mpreferred-stack-boundary=2 -O2 -ffreestanding -fno-builtin -fno-pic -fno-pie -fno-stack-protector -fno-asynchronous-unwind-tables -fno-unwind-tables -fcf-protection=none -fno-exceptions -Wall -Wextra -Wno-builtin-declaration-mismatch"
build() {  # $1 version tag, $2 extra define, $3 section VA, $4 HUD-text function, $5 end-of-frame function, $6 per-frame update
	mkdir -p build_$1
	sed -e "s/@VA@/$3/" -e "s/@HUD@/$4/" -e "s/@END@/$5/" -e "s/@MAIN@/$6/" link.ld.in > build_$1/link.ld
	grep -E '^#define ADDR_(RENDER_STATE|VS_SELECT|VS_CONST|PS_SET|HUD_VIEW|VIEW_BUILD|FP_SETUP|FIRE|INF_AMMO_FN|FP_MARKER|PERSPECTIVE|HUD_DRAW|REGION_DMG|HUD_ANCHOR|D_GRID|D_BMP|D_TEXT|HUD_CENTRE|HUD_QUAD|GRID_DRAW|TEXT_DRAW|TRACKER|SET_VDATA4F|CAM_MODE|MAIN_SAVE|DEATH_REVERT|CAM_TARGET|CAM_MARKER|GAME_TICK|FRAME_DRAW) ' $7 | awk '{print "orig_" tolower(substr($2, 6)) " = " $3 ";"}' >> build_$1/link.ld
	gcc $CFLAGS $2 -c spooky.c -o build_$1/spooky.o
	gcc -m32 -c hooks.S -o build_$1/hooks.o
	ld -m elf_i386 -T build_$1/link.ld -nostdlib -o build_$1/spooky.elf build_$1/hooks.o build_$1/spooky.o 2>&1 | grep -v RWX || true
	objcopy -O binary --set-section-flags .bss=alloc,load,contents build_$1/spooky.elf build_$1/spooky.bin
	nm -n build_$1/spooky.elf > build_$1/spooky.map
}
build v10 "" 0x586000 0x13c780 0x13cd0 0x12b450 addr_v10.h
build v15 -DH2_V15 0x596000 0x13f260 0x13cb0 0x12df60 addr_v15.h
