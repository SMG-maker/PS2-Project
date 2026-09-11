#
# Makefile para EasyRPG Launcher no PS2DEV SDK
#

EE_BIN = ps2_rpgmaker.elf
EE_OBJS = main.o

EE_INCS = -I$(PS2DEV)/gsKit/include -I$(PS2SDK)/ee/include -I$(PS2SDK)/common/include
EE_LDFLAGS = -L$(PS2DEV)/gsKit/lib -L$(PS2SDK)/ee/lib
EE_LIBS = -lgskit -ldmakit -laudsrv -lpad -lkernel -lmc -lc -lstdc++

all: $(EE_BIN)

clean:
	rm -f $(EE_OBJS) $(EE_BIN)

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
