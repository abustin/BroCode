CC=gcc
UNAME_S := $(shell uname -s)
CCFLAGS=-O2

ifeq ($(UNAME_S),Linux)
    CCFLAGS += -lm -lGLESv2
endif
ifeq ($(UNAME_S),Darwin)
    CCFLAGS += -framework OpenGL -framework SDL2
endif


app: app1.c
	$(CC) $(÷) $(CCFLAGS) app1.c -o app
