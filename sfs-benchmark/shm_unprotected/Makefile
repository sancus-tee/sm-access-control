SANCUS_SUPPORT_DIR = /usr/local/share/sancus-support
DEVICE             = /dev/ttyUSB0
VENDOR_KEY         = 4078d505d82099ba

CC                 = sancus-cc
LD                 = sancus-ld
CRYPTO             = sancus-crypto
LOAD               = sancus-loader
RM                 = rm -f

CFLAGS_NO_OPTI     = -I$(SANCUS_SUPPORT_DIR)/include/ -g --verbose -Wfatal-errors
CFLAGS             = $(CFLAGS_NO_OPTI) -O 3
LDFLAGS            = --standalone --verbose --ram-size 24K --rom-size 32K
LIBS               = -L$(SANCUS_SUPPORT_DIR)/lib -ldev-uart -ldev-spi
CRYPTOFLAGS        = --key $(VENDOR_KEY)
LOADFLAGS          = -device $(DEVICE) -baudrate 115200

OBJECTS            = my_malloc.o shm-benchmark.o ../benchmark.o ../common.o
TARGET             = shm-benchmark.elf
TARGET_NO_MACS     = $(TARGET)-no-macs

$(TARGET): $(TARGET_NO_MACS)
	$(CRYPTO) $(CRYPTOFLAGS) -o $@ $<

$(TARGET_NO_MACS): $(OBJECTS)
	$(LD) $(LDFLAGS) -o $@ $^ $(LIBS)

# common cannot be compiled with optimalisations (weird putchar signature llvm error)
../common.o:
	$(CC) $(CFLAGS_NO_OPTI) -c -o ../common.o ../common.c

.PHONY: load
load: $(TARGET)
	$(LOAD) $(LOADFLAGS) $<

.PHONY: clean
clean:
	$(RM) $(TARGET) $(TARGET_NO_MACS) $(OBJECTS)
