# Makefile for Inavjaga
CXX = g++
CXXFLAGS = -std=c++17 -Wpedantic -Wno-narrowing -g
LDFLAGS = -L/usr/lib -lpthread -lSista

# List all your source files here
SRC = inavjaga.cpp \
      src/inventory.cpp src/entity.cpp src/portal.cpp src/player.cpp src/archer.cpp src/worm.cpp src/wall.cpp src/bullet.cpp src/chest.cpp src/mine.cpp src/enemyBullet.cpp \

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
	rm -f include/sista/*.o

.PHONY: all clean
