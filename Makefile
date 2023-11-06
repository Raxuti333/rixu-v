CC=cc
CSRC=src/*
CFLAGS=-O2 -march=native -mtune=native -Iinc -flto
LFLAGS=
TARGET=rixu-v

build:
	${CC} ${CFLAGS} -o ${TARGET} ${CSRC} ${LFLAGS}