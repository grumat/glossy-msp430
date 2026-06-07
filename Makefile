CXX = arm-none-eabi-g++
CC  = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
CPU = -mcpu=cortex-m3 -mthumb
LDFLAGS = $(CPU) --specs=nosys.specs -Wl,--gc-sections

DEFINES = -DSTM32F103xB -DSTM32F10X_MD -DUSE_HAL_DRIVER

INC = -I. \
      -I./Firmware.shared \
      -I./bmt/include \
      -I./target.bluepill \
      -I./Drivers/STM32F1xx_HAL_Driver/Inc \
      -I./CMSIS/Device/ST/STM32F1xx/Include \
      -I./CMSIS/Include

CXXFLAGS = $(CPU) $(DEFINES) -O2 -std=c++20 -fno-exceptions -fno-rtti $(INC)
CFLAGS   = $(CPU) $(DEFINES) -O2 $(INC)

SRC_CPP = $(wildcard Firmware.shared/*.cpp) \
          $(wildcard Firmware.shared/*/*.cpp) \
          $(wildcard target.bluepill/*.cpp) \
          $(wildcard bmt/*.cpp)

SRC_C   = $(wildcard Drivers/STM32F1xx_HAL_Driver/Src/*.c)

OBJ = $(SRC_CPP:.cpp=.o) $(SRC_C:.c=.o) Firmware.shared/res/symbols.o

all: glossy.elf 

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.o: %.c
	$(CC) $(CPU) $(DEFINES) $(INC) -c $< -o $@

glossy.elf: $(OBJ)
	$(CXX) $(LDFLAGS) $(OBJ) -o $@

Firmware.shared/res/symbols.o: Firmware.shared/res/symbols.s
	$(CC) $(CPU) -c $< -o $@

clean: 
	rm -f $(SRC_CPP:.cpp=.o) $(SRC_C:.c=.o) Firmware.shared/res/symbols.o glossy.elf