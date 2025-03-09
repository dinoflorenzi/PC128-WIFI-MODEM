INCLUDES ?= mo5_lib.c

OPTS ?= -O2 -Wno-const --thommo

BUILD_PATH ?= ./build

TOOLS_PATH ?= ./tools



mo5_ex1: 
	cmoc -c $(OPTS) $(INCLUDES) ./modem.c
	cmoc -o $(BUILD_PATH)/modem.bin $(OPTS) $(INCLUDES)  ./modem.o
	rm ./modem.o
	$(TOOLS_PATH)/mo5/f2k5 $(BUILD_PATH)/modem.bin
	./rom_gen $(BUILD_PATH)/modem.bin "Modem"
	mv ./FILE.K5 ./modem.k7
	zip -u modem.zip modem.k7
	zip -u modem.zip cart.rom
	mv $(BUILD_PATH)/modem.bin .

	mv ./modem.bin $(BUILD_PATH)
	mv ./cart.rom $(BUILD_PATH)
	mv ./modem.k7 $(BUILD_PATH)/modem2.k7
	cat $(BUILD_PATH)/starter.k7 $(BUILD_PATH)/modem2.k7 > $(BUILD_PATH)/modem.k7