CC=gcc
UNAME_S := $(shell uname -s)
CCFLAGS=`sdl2-config --cflags --libs` -O2

ifeq ($(UNAME_S),Linux)
    CCFLAGS += -lm -lGLESv2 -lSDL2 -DUSE_ES2
endif
ifeq ($(UNAME_S),Darwin)
    CCFLAGS += -framework OpenGL -framework SDL2
endif


app: app1.c
	$(CC) $(÷)  app1.c $(CCFLAGS) -o app
