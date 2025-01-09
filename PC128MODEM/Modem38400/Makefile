INCLUDES ?= 

OPTS ?= -O2 -Wno-const --thommo

BUILD_PATH ?= ./build

TOOLS_PATH ?= ./tools



mo5_ex1: 
	cmoc -c $(OPTS) $(INCLUDES) ./modem384.c
	cmoc -o $(BUILD_PATH)/modem384.bin $(OPTS) $(INCLUDES)  ./modem384.o
	rm ./modem384.o
	$(TOOLS_PATH)/mo5/f2k5 $(BUILD_PATH)/modem384.bin
	./rom_gen $(BUILD_PATH)/modem384.bin "Modem384"
	mv ./FILE.K5 ./modem384.k7
	zip -u modem.zip modem384.k7
	zip -u modem.zip cart.rom
	mv $(BUILD_PATH)/modem384.bin .

	mv ./modem384.bin $(BUILD_PATH)
	mv ./cart.rom $(BUILD_PATH)
	mv ./modem384.k7 $(BUILD_PATH)
