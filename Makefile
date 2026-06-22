CC = gcc
CFLAGS = -Wall -Wextra -O3 -Iinclude

SRC = 	main.c \
		src/accel/bvh.c \
		src/accel/cmp.c \
		src/camera/camera.c \
		src/geometry/aabb.c \
		src/geometry/box.c \
		src/geometry/plane.c \
		src/geometry/sphere.c \
		src/geometry/triangle.c \
		src/math/vec.c \
		src/math/ray.c \
		src/render/pixel.c \
		src/render/render.c \
		src/render/trace.c \
		src/scene/scene.c \

OUT = app

all:
	$(CC) $(CFLAGS) $(SRC) -o $(OUT)

clean:
	del /Q $(OUT).exe