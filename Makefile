
CCFLAGS := -std=c++17 -Ofast -fopenmp -Wall -Wextra -Wpedantic

LDFLAGS := -lsfml-graphics -lsfml-window -lsfml-system

all:
	g++ $(CCFLAGS) $(wildcard src/*.cpp) $(LDFLAGS)

