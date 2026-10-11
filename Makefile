CC ?= cc
CXX ?= c++
CFLAGS ?= -O2 -Wall
CXXFLAGS ?= $(CFLAGS)
CXXSTD = -std=c++20
TARGET ?= stuns

CSRC = Deflate/puff.c PKWare/blast.c LZO/minilzo.c UCL/n2b_ds.c UCL/n2d_ds.c UCL/n2e_ds.c
CXXSRC = Main.cpp FAnalysis.cpp \
         Deflate/FDeflateDecoder.cpp PKWare/FPKWareDecoder.cpp LZO/FLZODecoder.cpp \
         UCL/FUCLDecoder.cpp LZ4/FLZ4Decoder.cpp LZMA/FLZMADecoder.cpp aPLib/FAPLibDecoder.cpp
OBJ = $(CSRC:.c=.o) $(CXXSRC:.cpp=.o)

# MinGW: link the C++ runtime in, an older libstdc++ DLL found first on PATH breaks C++20
ifeq ($(OS),Windows_NT)
LDFLAGS += -static
endif

$(TARGET): $(OBJ)
	$(CXX) $(CXXSTD) $(CXXFLAGS) -o $@ $(OBJ) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -IUCL -c -o $@ $<

%.o: %.cpp
	$(CXX) $(CXXSTD) $(CXXFLAGS) -IUCL -c -o $@ $<

$(OBJ): $(wildcard *.h */*.h UCL/ucl/*.h UCL/n2?_d.c)

test: $(TARGET)
	sh tests/run.sh ./$(TARGET)

clean:
	rm -f $(TARGET) $(TARGET).exe $(OBJ)

.PHONY: test clean
