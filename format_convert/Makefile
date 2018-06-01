CROSS_COMPILE=/opt/allwiner_compiler/external-toolchain/bin/arm-linux-gnueabi-

CXX=${CROSS_COMPILE}g++
CC=${CROSS_COMPILE}gcc

CFLAGS += -O 

objects = yuyv2rgb.o
#objects = main.o camera.o 
#objects += serial.o singer.o #buzzer.o
#objects += YokoScanner.o
target = yuyv2rgb

.PHONY:all
all:${target}

${target}:$(objects)
	@$(CXX) ${LDFLAGS} $^ -o $@
	
%.o:%.cpp
	@$(CXX) -c $^ ${CFLAGS}

.PHONY:clean
clean:
	@rm -f  ${target}
	@rm -f *.o
