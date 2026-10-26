CC = gcc

# CFLAGS = -g -O3 -fno-omit-frame-pointer -mavx2 -mfma -Wall -Wextra -Ithird_party/json -Ithird_party/image -Iinclude
CFLAGS = -Wall -Wextra -Iinclude -Ithird_party/json -Ithird_party/image -MMD -MP -g -O3 -mavx2 -mfma -flto=16 -ffast-math -fno-omit-frame-pointer

LDLIBS = -lm

OUT = app

SRC = \
	main.c \
	src/accel/bvh.c \
	src/camera/camera.c \
	src/geometry/mesh.c \
	src/geometry/triangle.c \
	src/math/vec.c \
	src/math/ray.c \
	src/render/render.c \
	src/render/trace.c \
	src/render/traversal.c \
	src/scene/scene.c \
	src/scene/sample.c \
	third_party/image/stb_image.c \
	third_party/json/yyjson.c

OBJ = $(addprefix build/,$(SRC:.c=.o))
DEP = $(OBJ:.o=.d)

all: $(OUT)

$(OUT): $(OBJ)
	$(CC) $(OBJ) -o $(OUT) $(LDLIBS)

build/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf build
	rm -f $(OUT)

re: clean all

-include $(DEP)

.PHONY: all clean re