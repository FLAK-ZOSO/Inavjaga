# Makefile for Inavjaga
IMPLEMENTATIONS = include/sista/ANSI-Settings.cpp include/sista/border.cpp include/sista/coordinates.cpp include/sista/cursor.cpp include/sista/field.cpp include/sista/pawn.cpp

all:
	@echo "Compiling Inavjaga..."
	$(MAKE) compile-sista
	$(MAKE) compile-inavjaga
	$(MAKE) link
	@echo "Inavjaga compiled successfully!"
install: all

compile-sista:
	g++ -std=c++17 -Wall -g -c $(IMPLEMENTATIONS)
sista: compile-sista

compile-inavjaga:
	g++ -std=c++17 -Wall -g -c -static inavjaga.cpp -Wno-narrowing
inavjaga: compile-inavjaga

link:
	g++ -std=c++17 -Wall -g -static -lpthread -o inavjaga inavjaga.o ANSI-Settings.o border.o coordinates.o cursor.o pawn.o field.o

clean:
	rm -f *.o

.PHONY: inavjaga
