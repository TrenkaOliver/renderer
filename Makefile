CC = gcc

CFLAGS = -Wall -Wextra -O3 -Iinclude -MMD -MP

OUT = app

SRC = \
	main.c \
	src/accel/bvh.c \
	src/accel/cmp.c \
	src/array/array.c \
	src/camera/camera.c \
	src/geometry/aabb.c \
	src/geometry/box.c \
	src/geometry/mesh.c \
	src/geometry/object.c \
	src/geometry/plane.c \
	src/geometry/sphere.c \
	src/geometry/triangle.c \
	src/image/image.c \
	src/light/material.c \
	src/math/vec.c \
	src/math/ray.c \
	src/render/pixel.c \
	src/render/render.c \
	src/render/trace.c \
	src/scene/scene.c

OBJ = $(addprefix build/,$(SRC:.c=.o))
DEP = $(OBJ:.o=.d)

all: $(OUT)

$(OUT): $(OBJ)
	$(CC) $(OBJ) -o $(OUT)

build/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	if exist build rmdir /S /Q build
	if exist $(OUT).exe del $(OUT).exe
	if exist $(OUT) del $(OUT)

re: clean all

-include $(DEP)

.PHONY: all clean re