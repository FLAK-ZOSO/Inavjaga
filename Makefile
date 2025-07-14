# Makefile for Inavjaga
CXX = g++
CXXFLAGS = -std=c++17 -Wpedantic -Wno-narrowing -g
LDFLAGS = -lpthread

# List all your source files here
SRC = inavjaga.cpp \
      entity.cpp portal.cpp \
      include/sista/ANSI-Settings.cpp include/sista/border.cpp include/sista/coordinates.cpp include/sista/cursor.cpp include/sista/field.cpp include/sista/pawn.cpp

OBJ = $(SRC:.cpp=.o)

all: inavjaga

inavjaga: $(OBJ)
	@echo "Linking with -static (if possible)..."
	@($(CXX) $(CXXFLAGS) -static -o $@ $^ $(LDFLAGS) || \
	  $(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS))
	@echo "Inavjaga compiled successfully!"

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f *.o inavjaga

.PHONY: all clean
