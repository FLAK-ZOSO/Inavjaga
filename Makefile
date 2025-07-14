# Makefile for Inavjaga
CXX = g++
CXXFLAGS = -std=c++17 -Wpedantic -Wno-narrowing -g
LDFLAGS = -lpthread

# List all your source files here
SRC = inavjaga.cpp \
      src/inventory.cpp src/entity.cpp src/portal.cpp src/player.cpp src/archer.cpp src/worm.cpp src/wall.cpp src/bullet.cpp src/chest.cpp src/mine.cpp src/enemyBullet.cpp \
      include/sista/ANSI-Settings.cpp include/sista/border.cpp include/sista/coordinates.cpp include/sista/cursor.cpp include/sista/field.cpp include/sista/pawn.cpp

OBJ = $(SRC:.cpp=.o)

all: inavjaga clean

inavjaga: $(OBJ)
	@echo "Linking with -static (if possible)..."
	@($(CXX) $(CXXFLAGS) -static -o $@ $^ $(LDFLAGS) || \
	  $(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS))
	@echo "Inavjaga compiled successfully!"

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f *.o
	rm -f src/*.o

.PHONY: all clean
