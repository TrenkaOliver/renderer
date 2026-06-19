CC = gcc
CFLAGS = -Wall -Wextra -O3 -Iinclude

SRC = 	main.c \
		src/geometry/mesh/box.c \
		src/geometry/mesh/rectangle.c \
		src/geometry/object.c \
		src/geometry/vec.c \
		src/graphics/pixel.c \
		src/graphics/render.c \
		src/graphics/trace.c \
		src/camera.c \
		src/scene.c \

OUT = app

all:
	$(CC) $(CFLAGS) $(SRC) -o $(OUT)

clean:
	del /Q $(OUT).exe