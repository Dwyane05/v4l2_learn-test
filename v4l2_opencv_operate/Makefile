ARCH?=arm

TOPDIR=${PWD}

CFLAGS := -Wall
#CFLAGS += -I${PWD} -Wno-psabi
#CFLAGS += -I${PWD}/../include
CFLAGS +=  -DDEBUG

Q=@

#CROSS_COMPILE=/opt/allwiner_compiler/external-toolchain/bin/arm-linux-gnueabi-
#LIBS_PATH=/opt/yoko_libs

CROSS_COMPILE=/opt/ingenic_compiler/mips-gcc472-glibc216/bin/mips-linux-gnu-
LIBS_PATH=/opt/yoko_libs


CXX=${CROSS_COMPILE}g++
CC=${CROSS_COMPILE}gcc

# for optimizing, close it for debug
#CFLAGS += -O 
#CFLAGS += -Os
#CFLAGS += -std=gnu++11
#CFLAGS += -std=c++11

#LDFLAGS += -lrt -lpthread 

#libopencv
CFLAGS += -I${LIBS_PATH}/ingenic/opencv320/include
LDFLAGS += -lopencv_core -lopencv_video -lopencv_videoio  -lopencv_highgui -lopencv_imgcodecs -lopencv_imgproc -L${LIBS_PATH}/ingenic/opencv320/lib
#

ifdef FULL
CFLAGS_FULL:= -Wextra -Wfloat-equal -save-temps 
else
CFLAGS_FULL:=
endif


src := main_mat.cpp 

src += camera/Camera.cpp
CFLAGS += -I${PWD}/camera

src += bmp/bmp.cpp 
CFLAGS += -I${PWD}/bmp


objects=$(patsubst %.cpp,%.o, $(src))
target = camera_test

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

	
