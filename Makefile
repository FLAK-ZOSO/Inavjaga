# Makefile for Inavjaga
IMPLEMENTATIONS = include/sista/ANSI-Settings.cpp include/sista/border.cpp include/sista/coordinates.cpp include/sista/cursor.cpp include/sista/field.cpp include/sista/pawn.cpp

all:
	@echo "Compiling Inavjaga..."
	$(MAKE) compile-sista
	$(MAKE) compile-inavjaga
	$(MAKE) link
	$(MAKE) clean
	@echo "Inavjaga compiled successfully!"
install: all

compile-sista:
	g++ -std=c++17 -Wpedantic -g -c $(IMPLEMENTATIONS)
sista: compile-sista

compile-inavjaga:
	g++ -std=c++17 -Wpedantic -g -c -static inavjaga.cpp -Wno-narrowing
inavjaga: compile-inavjaga

link:
	@echo "Linking with -static (if possible)..."
	@(g++ -std=c++17 -Wpedantic -g -static -lpthread -o inavjaga inavjaga.o ANSI-Settings.o border.o coordinates.o cursor.o pawn.o field.o || \
	  g++ -std=c++17 -Wpedantic -g -lpthread -o inavjaga inavjaga.o ANSI-Settings.o border.o coordinates.o cursor.o pawn.o field.o)

clean:
	rm -f *.o

.PHONY: inavjaga
