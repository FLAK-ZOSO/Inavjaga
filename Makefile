# Makefile for Inavjaga
CXX = g++
CXXFLAGS = -std=c++17 -Wpedantic -Wall -Wno-narrowing -g
DEPFLAGS = -MMD -MP

# Set USE_LOCAL_SISTA=1 to compile Sista from vendored sources in include/sista.
# Windows defaults to local sources to avoid relying on system-wide library installs.
ifeq ($(OS),Windows_NT)
USE_LOCAL_SISTA ?= 1
THREAD_LDFLAGS =
else
USE_LOCAL_SISTA ?= 0
THREAD_LDFLAGS = -lpthread
endif

SISTA_REPO = https://github.com/FLAK-ZOSO/Sista.git
SISTA_CACHE_DIR = .deps/Sista
SISTA_LOCAL_DIR = include/sista
SISTA_IMPL = ansi.cpp border.cpp coordinates.cpp cursor.cpp field.cpp pawn.cpp
SISTA_LOCAL_SRC = $(addprefix $(SISTA_LOCAL_DIR)/,$(SISTA_IMPL))

ifeq ($(USE_LOCAL_SISTA),1)
CXXFLAGS += -Iinclude
LDFLAGS = $(THREAD_LDFLAGS)
SRC = inavjaga.cpp $(wildcard src/*.cpp) $(SISTA_LOCAL_SRC)
else
# Ensure the runtime linker can find libSista.dylib installed to /usr/local/lib.
LDFLAGS = -L/usr/local/lib -L/usr/lib $(THREAD_LDFLAGS) -lSista -Wl,-rpath,/usr/local/lib
SRC = inavjaga.cpp $(wildcard src/*.cpp)
endif

# Set STATIC=0 to prefer dynamic linking, otherwise static linking is used
STATIC ?= 1

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

$(SISTA_LOCAL_SRC): | prepare-local-sista

%.o: %.cpp Makefile
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) -c $< -o $@

-include $(wildcard $(DEP))

prepare-local-sista:
ifeq ($(USE_LOCAL_SISTA),1)
	@if [ -f "$(SISTA_LOCAL_DIR)/sista.hpp" ]; then \
		echo "Using vendored Sista sources from $(SISTA_LOCAL_DIR)"; \
	else \
		echo "Vendoring Sista sources into $(SISTA_LOCAL_DIR)..."; \
		mkdir -p .deps include; \
		if [ ! -d "$(SISTA_CACHE_DIR)/.git" ]; then \
			git clone --depth 1 "$(SISTA_REPO)" "$(SISTA_CACHE_DIR)"; \
		else \
			git -C "$(SISTA_CACHE_DIR)" pull --ff-only; \
		fi; \
		rm -rf "$(SISTA_LOCAL_DIR)"; \
		cp -r "$(SISTA_CACHE_DIR)/include/sista" "$(SISTA_LOCAL_DIR)"; \
	fi
else
	@echo "USE_LOCAL_SISTA=0, skipping local Sista vendoring"
endif

clean:
	rm -f *.o
	rm -f src/*.o
	rm -f include/sista/*.o
	rm -f *.d
	rm -f src/*.d
	rm -f include/sista/*.d

.PHONY: all clean prepare-local-sista
