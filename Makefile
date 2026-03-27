NAM = IMEstate
DST = .$(VSCMD_ARG_TGT_ARCH)

# Define compiler and flags
CL = cl
CFLAGS = /Zi /EHsc- /utf-8 /nologo /std:c++20 /W2 /O1 /GL /Gy /Gw /GS- /GR- /Zc:inline /MT /D NDEBUG /DEBUG:NO
LFLAGS = /link /LTCG /OPT:REF,ICF

# Define source files and output
OUT = $(NAM).exe
CPP = $(NAM).cpp

# Build rules
all: $(DST)\$(OUT)

$(DST)\$(OUT): $(CPP) Makefile
    CD $(DST) && $(CL) $(CFLAGS) /Fe$(OUT) ..\$(CPP) $(LFLAGS)

clean:
    CD $(DST) && DEL *.pdb && del $(OUT) *.obj
