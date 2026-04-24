# Makefile for Inavjaga
CXX = g++
CXXFLAGS = -std=c++17 -Wpedantic -Wall -Wno-narrowing -g
DEPFLAGS = -MMD -MP
# Ensure the runtime linker can find libSista.dylib installed to /usr/local/lib
# Add /usr/local/lib to the library search path and embed an rpath into the binary.
LDFLAGS = -L/usr/local/lib -L/usr/lib -lpthread -lSista -Wl,-rpath,/usr/local/lib

# Set STATIC=0 to prefer dynamic linking, otherwise static linking is used
STATIC ?= 1

# List all your source files here
SRC = inavjaga.cpp $(wildcard src/*.cpp)

OBJ = $(SRC:.cpp=.o)
DEP = $(OBJ:.o=.d)

all: inavjaga

ifeq ($(STATIC),1)
inavjaga: $(OBJ)
	@echo "Trying to link statically..."
	@($(CXX) $(CXXFLAGS) -static -o $@ $^ $(LDFLAGS) || \
	  $(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS))
	@echo "Inavjaga compiled successfully!"
else
inavjaga: $(OBJ)
	@echo "Linking dynamically..."
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Inavjaga compiled successfully!"
endif

%.o: %.cpp Makefile
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) -c $< -o $@

-include $(DEP)

clean:
	rm -f *.o
	rm -f src/*.o
	rm -f *.d
	rm -f src/*.d

.PHONY: all clean
