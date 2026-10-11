CC ?= cc
CXX ?= c++
CFLAGS ?= -O2 -Wall
CXXFLAGS ?= $(CFLAGS)
TARGET ?= stuns

CSRC = Deflate/puff.c PKWare/blast.c LZO/minilzo.c UCL/n2b_ds.c UCL/n2d_ds.c
CXXSRC = Main.cpp FBaseDecoder.cpp \
         Deflate/FDeflateDecoder.cpp PKWare/FPKWareDecoder.cpp LZO/FLZODecoder.cpp \
         UCL/FUCL2BDecoder.cpp UCL/FUCL2DDecoder.cpp
OBJ = $(CSRC:.c=.o) $(CXXSRC:.cpp=.o)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJ) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -IUCL -c -o $@ $<

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -IUCL -c -o $@ $<

$(OBJ): $(wildcard *.h */*.h UCL/ucl/*.h UCL/n2?_d.c)

test: $(TARGET)
	sh tests/run.sh ./$(TARGET)

clean:
	rm -f $(TARGET) $(TARGET).exe $(OBJ)

.PHONY: test clean
