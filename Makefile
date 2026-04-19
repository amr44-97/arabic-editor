
CC = clang++

LFLAG = -lSDL3 -lSDL3_ttf

CFLAGS = -std=c++26 -g -fno-omit-frame-pointer -O1 -Wno-unused-variable -Wall 

EXE_DIR = bin
EXE = $(EXE_DIR)/run.exe

.PHONY: $(EXE) clean

$(EXE): src/main.cpp
	$(CC) $(LFLAG) $(CFLAGS) -o $(EXE) $^

clean:
	rm bin/*
