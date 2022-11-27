CC=gcc

UNAME := $(shell uname)

DEBUG ?= 0

ifeq ($(DEBUG), 1)
	CFLAGS=-O0 $(INCLUDES) -Wall -g -DDEBUG -I ../src/common/
	LDFLAGS= -lc -lm -ldl -lpthread -lasan
else
	CFLAGS=-O3 $(INCLUDES) -Wall -I ../src/common/
	LDFLAGS= -lc -lm -ldl -s
endif

ifeq ($(UNAME), Darwin)
CFLAGS += -arch x86_64 -mmacosx-version-min=10.9
LDFLAGS += -arch x86_64 -mmacosx-version-min=10.9
endif

ifeq ($(UNAME), Linux)
LDFLAGS += -Wl,-rpath=.
endif

ifeq ($(findstring CYGWIN,$(shell uname)),CYGWIN)
endif

EXEC=broadcast_fm rf_jammer

all: $(EXEC)

broadcast_fm:  broadcast_fm.o rds.o rds_stream_dump.o wave.o modulator.o FIR_Audio_Filter_Filter.o FM_Baseband_Filter.o AudioPreemphasis_Filter.o FIR_RDS_Passband_Filter.o hxcmod.o rand_gen.o
	$(CC) -o $@    $^ $(LDFLAGS)

rf_jammer:  rf_jammer.o wave.o modulator.o rand_gen.o
	$(CC) -o $@    $^ $(LDFLAGS)

broadcast_fm.o: ../src/broadcast_fm/broadcast_fm.c
	$(CC) -o $@ -c $< $(CFLAGS)

rf_jammer.o: ../src/rf_jammer/rf_jammer.c
	$(CC) -o $@ -c $< $(CFLAGS)

rds.o: ../src/broadcast_fm/rds.c
	$(CC) -o $@ -c $< $(CFLAGS)

rds_stream_dump.o: ../src/broadcast_fm/rds_stream_dump.c
	$(CC) -o $@ -c $< $(CFLAGS)

utils.o: ../src/common/utils.c
	$(CC) -o $@ -c $< $(CFLAGS)

wave.o: ../src/common/wave.c
	$(CC) -o $@ -c $< $(CFLAGS)

modulator.o: ../src/common/modulator.c
	$(CC) -o $@ -c $< $(CFLAGS)

FIR_Audio_Filter_Filter.o: ../src/broadcast_fm/fir_filters/FIR_Audio_Filter_Filter.c
	$(CC) -o $@ -c $< $(CFLAGS)

FM_Baseband_Filter.o: ../src/broadcast_fm/fir_filters/FM_Baseband_Filter.c
	$(CC) -o $@ -c $< $(CFLAGS)

FIR_RDS_Passband_Filter.o: ../src/broadcast_fm/fir_filters/FIR_RDS_Passband_Filter.c
	$(CC) -o $@ -c $< $(CFLAGS)

AudioPreemphasis_Filter.o: ../src/broadcast_fm/fir_filters/AudioPreemphasis_Filter.c
	$(CC) -o $@ -c $< $(CFLAGS)

hxcmod.o: ../src/common/hxcmod/hxcmod.c
	$(CC) -o $@ -c $< $(CFLAGS)

rand_gen.o: ../src/common/rand_gen.c
	$(CC) -o $@ -c $< $(CFLAGS)

clean:
	rm -rf *.o
	rm -rf *.so

mrproper: clean
	rm -rf $(EXEC)

.PHONY: clean mrproper
