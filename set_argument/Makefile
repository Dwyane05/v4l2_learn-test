ARCH?=mips

TOPDIR=${PWD}

CFLAGS := -Wall -Wno-unused-variable -Wno-sign-compare
CFLAGS += -Wno-psabi
CFLAGS +=  -Wno-unused-function
#CFLAGS += -I${PWD}

Q=@

ifeq ($(ARCH), arm)
CROSS_COMPILE=/opt/allwiner_compiler/external-toolchain/bin/arm-linux-gnueabi-
LIBS_PATH=/opt/yoko_libs

#libzbar
CFLAGS += -I${LIBS_PATH}/allwinner/zbar_qronly/include
LDFLAGS += -lzbar -L${LIBS_PATH}/allwinner/zbar_qronly/lib
else

ifeq ($(ARCH), mips)
CROSS_COMPILE=/opt/ingenic_compiler/mips-gcc472-glibc216/bin/mips-linux-gnu-
LIBS_PATH=/opt/yoko_libs/ingenic

#libzbar
CFLAGS += -I${LIBS_PATH}/zbar_qr/include
LDFLAGS += -lzbar -L${LIBS_PATH}/zbar_qr/lib

#libz
#LDFLAGS += -lz -L${LIBS_PATH}/zlib/lib

else
$(error "Unknow ARCH")
endif #mips
endif #arm

CXX=${CROSS_COMPILE}g++
CC=${CROSS_COMPILE}gcc

# for optimizing, close it for debug
#CFLAGS += -O 
#CFLAGS += -Os
#CFLAGS += -std=gnu++11
#CFLAGS += -std=c++11

LDFLAGS += -lrt -lpthread -lc -lstdc++ -lzbar

ifdef FULL
CFLAGS_FULL:= -Wextra -Wfloat-equal -save-temps 
else
CFLAGS_FULL:=
endif

#CFLAGS += -I${PWD}/api

src := main.cpp 

src += camera/Camera.cpp 
CFLAGS += -I${PWD}/camera

src += scanner/YokoScanner.cpp
CFLAGS += -I${PWD}/scanner

objects=$(patsubst %.cpp,%.o, $(src))
target = set_argument

.PHONY:all
all:${target}

${target}:$(objects)
	$(Q)$(CXX) $^ ${LDFLAGS} -o $@
	
%.o:%.cpp
	$(Q)$(CXX) -c $^ ${CFLAGS} ${CFLAGS_FULL} -o $@

.PHONY:clean
clean:
	@rm -f  ${target}
	@find -name "*.o" -execdir rm {} \;
