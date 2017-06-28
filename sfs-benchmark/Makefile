SANCUS_SUPPORT_DIR = /usr/local/share/sancus-support
DEVICE             = /dev/ttyUSB0
VENDOR_KEY         = 4078d505d82099ba

CC                 = sancus-cc
LD                 = sancus-ld
CRYPTO             = sancus-crypto
LOAD               = sancus-loader
RM                 = rm -f

ACL_BENCHMARK      = -DRUN_ACL_BENCHMARK -DMAX_ACL_BENCHMARK_LENGTH=3
FILES_BENCHMARK    = -DRUN_FILES_BENCHMARK -DNB_BENCHMARK_FILES=3 -DINIT_BENCHMARK_FILE_SIZE=100
BENCHMARK_TYPE     = $(FILES_BENCHMARK) #$(FILES_BENCHMARK) #-DDO_DUMP #$(ACL_BENCHMARK) #FIXME turn this into a make target...

DEBUG_LEVEL        = -DNODEBUG #-DSFS_DEBUG
CFS_BACKEND        = #-DCFS_BACKEND_PROTECTED #-DMEASURE_CFS_BACKEND #-DNO_CFS_FORMAT #-DMEASURE_CFS_BACKEND

CFLAGS_NO_OPTI     = -I$(SANCUS_SUPPORT_DIR)/include/ --verbose -Wfatal-errors $(DEBUG_LEVEL) $(CFS_BACKEND) $(BENCHMARK_TYPE) #-g
CFLAGS             = $(CFLAGS_NO_OPTI) #-O s
LDFLAGS            = --verbose --ram-size 24K --rom-size 32K --standalone
LIBS               = -L$(SANCUS_SUPPORT_DIR)/lib -ldev-uart -ldev-spi
CRYPTOFLAGS        = --key $(VENDOR_KEY)
LOADFLAGS          = -device $(DEVICE) -baudrate 115200

SFS_DUMMY          = ../sfs/sfs-dummy.o
SFS_CFS_DUMMY      = ../sfs/sfs-ram.o  ../sfs/cfs/cfs-dummy.o
SFS_SHM            = ../sfs/sfs-ram.o ../sfs/shm/shared-mem.o ../sfs/shm/my_malloc.o
SFS_COFFEE         = ../sfs/sfs-ram.o ../sfs/cfs/cfs-coffee.o
SFS                = $(SFS_DUMMY)

OBJECTS            = main.o sfs-benchmark.o ../../benchmark.o $(SFS) ../../common.o
TARGET             = benchmark.elf
TARGET_NO_MACS     = $(TARGET)-no-macs

$(TARGET): $(TARGET_NO_MACS)
	$(CRYPTO) $(CRYPTOFLAGS) -o $@ $<

$(TARGET_NO_MACS): $(OBJECTS)
	$(LD) $(LDFLAGS) -o $@ $^ $(LIBS)

# always compile sfs ram with optimisations (as it uses a lot of non-inlined helper functions)
#../sfs/sfs-ram.o:
	$(CC) $(CFLAGS) -O 3 -c -o ../sfs/sfs-ram.o ../sfs/sfs-ram.c

# always compile shared-mem with optimisations
../sfs/shm/shared-mem.o:
	$(CC) $(CFLAGS) -O 3 -c -o ../sfs/shm/shared-mem.o ../sfs/shm/shared-mem.c

# common cannot be compiled with optimalisations (weird putchar signature llvm error)
../../common.o:
	$(CC) $(CFLAGS_NO_OPTI) -c -o ../../common.o ../../common.c

.PHONY: load
load: $(TARGET)
	$(LOAD) $(LOADFLAGS) $<

.PHONY: clean
clean:
	$(RM) $(TARGET) $(TARGET_NO_MACS) $(OBJECTS)
