# Copyright (c) 2014 by Steve Delahunty
# All Rights Reserved
# 
# Simple Makefile for creating the sample MyLaps application

CFLAGS += -I./
EXTRA_CFLAGS=
EXE=main
CC=cc

SRCS= main.c

all:main

main: $(SRCS)
	$(CC)  $(CFLAGS) $(EXTRA_FLAGS) $^ -o $(EXE)

