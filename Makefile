# Makefile pre zadanie 1 
CC := clang
BIN_NAME := main

build: tester.c memsim.h memsim_internal.h memsim.c memsimdbg.c memsimdbg.h concolors.h
	$(CC) tester.c memsim.c memsimdbg.c -o $(BIN_NAME)

all: build
