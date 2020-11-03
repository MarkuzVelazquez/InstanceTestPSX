PROJNAME = prueba
CC = mipsel-unknown-elf-gcc
PSX_ROOT = /usr/local/psxsdk
LIC_FILE = ${PSX_ROOT}/share/licenses/infousa.dat
LIB = -L${PSX_ROOT}/lib
INCLUDE = -I${PSX_ROOT}/include
LDSCRIPT = -T ${PSX_ROOT}/mipsel-unknown-elf/lib/ldscripts/playstation.x
CCFLAGS = -nostdlib -fsigned-char -msoft-float -mno-gpopt -fno-builtin -G0 ${LIB} ${INCLUDE} ${LDSCRIPT}
AR = mipsel-unknown-elf-ar
CCLIB = pelota

all: sonidos texturas build ${PROJNAME}.hsf
	mkpsxiso ${PROJNAME}.hsf ${PROJNAME}.bin ${LIC_FILE}
	rm ${PROJNAME}.hsf

${PROJNAME}.hsf:
	mkisofs -o ${PROJNAME}.hsf -V ${PROJNAME} -sysid PLAYSTATION cd_root

build: lib${CCLIB}.a
	${CC} ${CCFLAGS} -o ${PROJNAME}.elf src/main.c -lm -L. -l${CCLIB}
	elf2exe ${PROJNAME}.elf ${PROJNAME}.exe 
	cp ${PROJNAME}.exe cd_root
	systemcnf ${PROJNAME}.exe > cd_root/system.cnf
	mv ${PROJNAME}.* bin
	mv lib${CCLIB}.* bin

lib${CCLIB}.a: lib${CCLIB}.o
	${AR} -rcs lib${CCLIB}.a lib${CCLIB}.o

lib${CCLIB}.o:
	${CC} ${CCFLAGS} -c -o lib${CCLIB}.o lib/${CCLIB}.c

texturas:
	bmp2tim data/textura1.bmp cd_root/textura1.tim 16 -org=320,0

sonidos:
	wav2vag data/coin.wav cd_root/coin.raw -raw

clean:
	rm -I -r bin/*
	rm -I -r cd_root/*
	rm -I -r ${PROJNAME}.*

clean-lib:
	rm -rf lib${CCLIB}.*

run:
	pcsxr -psxout -nogui -cdfile ${PROJNAME}.bin



