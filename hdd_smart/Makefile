#G5
TOOLCHAIN_PATH = /opt/toolchains/stbgcc_sf_cortex_a15_neonvfp4-4.8-1.01
CC = $(TOOLCHAIN_PATH)/bin/arm-linux-g++

#CC = g++
TARGET = main
OBJECTS = main.o
LIBS = -Llib 
INCLUDES = -Iinclude 
CFLAGS = $(LIBS) $(INCLUDES) -lm

OUTPUT=hddsmart_test

all: $(TARGET)
	
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(OUTPUT) $(OBJECTS)
	$(call after_build)

main.o: main.cpp
	$(CC) $(CFLAGS) -c $<

clean:
	rm $(OBJECTS)

define after_build
	@echo "make complete $(1)"
endef
