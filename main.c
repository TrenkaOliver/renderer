#include <stdio.h>
#include <stdint.h>
#include <time.h>

#include "render/render.h"
#include "light/material.h"
#include "scene/scene.h"
#include "camera/camera.h"
#include "math/vec.h"

#define TOTAL_OBJECTS 500000

#define SPHERE_COUNT   166667
#define BOX_COUNT      166667
#define TRIANGLE_COUNT 166666

/*
 * Grid:
 *
 * 100 x 100 x 50 = 500,000 objects
 *
 * Each object gets one cell.
 */
#define GRID_X 100
#define GRID_Y 100
#define GRID_Z 50

#define CELL_X 100.0
#define CELL_Y 100.0
#define CELL_Z 100.0


/*
 * ============================================================
 * Deterministic random generator
 * ============================================================
 */

static uint32_t rng_state = 0x12345678u;

static uint32_t rng_u32(void)
{
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 17;
    rng_state ^= rng_state << 5;

    return rng_state;
}

static double random_double(double min, double max)
{
    double t =
        (double)rng_u32() /
        (double)UINT32_MAX;

    return min + t * (max - min);
}


/*
 * ============================================================
 * Materials
 * ============================================================
 */

Material sphere_mat = {
    .diffuse = {
        .x = 0.08,
        .y = 0.30,
        .z = 0.95
    },

    .specular = {
        .x = 0.9,
        .y = 0.9,
        .z = 0.9
    },

    .shininess = 64.0,
    .reflectivity = 0.30,

    .diffuse_map = (size_t)-1,
    .normal_map = (size_t)-1,
    .splecular_map = (size_t)-1,
};


Material box_mat = {
    .diffuse = {
        .x = 0.95,
        .y = 0.18,
        .z = 0.04
    },

    .specular = {
        .x = 0.5,
        .y = 0.5,
        .z = 0.5
    },

    .shininess = 32.0,
    .reflectivity = 0.15,

    .diffuse_map = (size_t)-1,
    .normal_map = (size_t)-1,
    .splecular_map = (size_t)-1,
};


Material triangle_mat = {
    .diffuse = {
        .x = 0.05,
        .y = 0.85,
        .z = 0.20
    },

    .specular = {
        .x = 0.7,
        .y = 0.7,
        .z = 0.7
    },

    .shininess = 48.0,
    .reflectivity = 0.20,

    .diffuse_map = (size_t)-1,
    .normal_map = (size_t)-1,
    .splecular_map = (size_t)-1,
};


Material ground_mat = {
    .diffuse = {
        .x = 0.55,
        .y = 0.55,
        .z = 0.55
    },

    .specular = {
        .x = 0.1,
        .y = 0.1,
        .z = 0.1
    },

    .shininess = 8.0,
    .reflectivity = 0.20,

    .diffuse_map = (size_t)-1,
    .normal_map = (size_t)-1,
    .splecular_map = (size_t)-1,
};


int main(void)
{
    clock_t start = clock();

    const int width = 640;
    const int height = 360;
    const double zoom = 0.01;

    FILE *f = fopen("result.ppm", "wb");

    if (!f)
        return 1;


    /*
     * ============================================================
     * Scene
     * ============================================================
     */

    Scene scene = create_scene();


    RenderSettings settings = {
        .width = width,
        .height = height,

        /*
         * Primary rays only.
         *
         * Use max_depth = 3 later when you want to benchmark
         * reflections/secondary rays as well.
         */
        .max_depth = 2  ,

        .aa_samples = 1
    };


    /*
     * ============================================================
     * Camera
     * ============================================================
     *
     * Looking down the long Y axis.
     */

    Camera cam = create_look_at_camera(
        vec(0.0, -4000.0 * zoom, 3000.0 * zoom),
        vec(0.0, 2500.0, 400.0),
        1.0472
    );


    /*
     * Directional light.
     */

    scene.dir_light.dir =
        normalize(vec(
            -0.4,
            -0.7,
            1.0
        ));


    /*
     * ============================================================
     * Object generation
     * ============================================================
     *
     * Every primitive gets its own cell.
     *
     * This means objects are:
     *
     *   - mixed together
     *   - spatially separated
     *   - deterministic
     *   - varied in size
     *
     * No primitive type gets its own region.
     */

    int spheres = 0;
    int boxes = 0;
    int triangles = 0;


    for (int x = 0; x < GRID_X; x++) {

        for (int y = 0; y < GRID_Y; y++) {

            for (int z = 0; z < GRID_Z; z++) {

                int index =
                    x * GRID_Y * GRID_Z +
                    y * GRID_Z +
                    z;


                /*
                 * ------------------------------------------------
                 * Cell center
                 * ------------------------------------------------
                 */

                double px =
                    (x - GRID_X / 2) * CELL_X;

                double py =
                    y * CELL_Y;

                double pz =
                    40.0 + z * CELL_Z;


                /*
                 * Small positional jitter.
                 *
                 * Maximum ±20 while cells are 100 units apart.
                 */
                px += random_double(-20.0, 20.0);
                py += random_double(-20.0, 20.0);
                pz += random_double(-20.0, 20.0);


                /*
                 * Different objects have different sizes.
                 *
                 * Maximum diameter is still considerably smaller
                 * than the 100-unit cell spacing.
                 */
                double size =
                    random_double(15.0, 35.0);


                /*
                 * =================================================
                 * Select primitive
                 * =================================================
                 *
                 * Exactly:
                 *
                 * 166667 spheres
                 * 166667 boxes
                 * 166666 triangles
                 */

                int type;

                if (spheres < SPHERE_COUNT &&
                    boxes < BOX_COUNT &&
                    triangles < TRIANGLE_COUNT) {

                    type = index % 3;

                } else if (spheres < SPHERE_COUNT) {

                    type = 0;

                } else if (boxes < BOX_COUNT) {

                    type = 1;

                } else {

                    type = 2;
                }


                /*
                 * =================================================
                 * SPHERE
                 * =================================================
                 */

                if (type == 0) {

                    add_sphere(
                        &scene,
                        vec(px, py, pz),
                        size,
                        &sphere_mat
                    );

                    spheres++;
                }


                /*
                 * =================================================
                 * BOX
                 * =================================================
                 */

                else if (type == 1) {

                    /*
                     * Non-uniform dimensions.
                     */

                    double sx =
                        random_double(
                            20.0,
                            2.0 * size
                        );

                    double sy =
                        random_double(
                            20.0,
                            2.0 * size
                        );

                    double sz =
                        random_double(
                            20.0,
                            2.0 * size
                        );


                    /*
                     * Random rotation.
                     *
                     * Assumes your add_box() rotation argument
                     * uses Euler angles like the rest of your
                     * renderer.
                     */

                    Vec rotation = vec(
                        random_double(
                            -0.5,
                            0.5
                        ),

                        random_double(
                            -0.5,
                            0.5
                        ),

                        random_double(
                            -0.5,
                            0.5
                        )
                    );


                    add_box(
                        &scene,

                        vec(
                            px - sx * 0.5,
                            py - sy * 0.5,
                            pz - sz * 0.5
                        ),

                        rotation,

                        vec(
                            sx,
                            sy,
                            sz
                        ),

                        &box_mat
                    );

                    boxes++;
                }


                /*
                 * =================================================
                 * TRIANGLE
                 * =================================================
                 */

                else {

                    /*
                     * Variable triangle dimensions.
                     */

                    double w =
                        random_double(
                            20.0,
                            60.0
                        );

                    double h =
                        random_double(
                            20.0,
                            60.0
                        );


                    /*
                     * Randomly deform the triangle in 3D.
                     */

                    Vec v0 = vec(
                        px - w * 0.5,
                        py - h * 0.5,
                        pz
                    );


                    Vec v1 = vec(
                        px + w * 0.5,
                        py,
                        pz + random_double(
                            -20.0,
                             20.0
                        )
                    );


                    Vec v2 = vec(
                        px,
                        py + h * 0.5,
                        pz + random_double(
                            -20.0,
                             20.0
                        )
                    );


                    add_triangle(
                        &scene,
                        v0,
                        v1,
                        v2,
                        &triangle_mat
                    );

                    triangles++;
                }
            }
        }
    }


    /*
     * ============================================================
     * Ground plane
     * ============================================================
     */

    add_plane(
        &scene,
        vec(0.0, 0.0, 0.0),
        vec(0.0, 0.0, 1.0),
        &ground_mat
    );


    /*
     * ============================================================
     * Scene creation timing
     * ============================================================
     */

    clock_t end = clock();

    printf(
        "Spheres:   %d\n",
        spheres
    );

    printf(
        "Boxes:     %d\n",
        boxes
    );

    printf(
        "Triangles: %d\n",
        triangles
    );

    printf(
        "Total:     %d\n",
        spheres + boxes + triangles
    );

    printf(
        "Scene creation: %.3f s\n",
        (double)(end - start) /
        CLOCKS_PER_SEC
    );


    /*
     * ============================================================
     * Render
     * ============================================================
     */

    render(
        f,
        &scene,
        &cam,
        &settings
    );


    fclose(f);

    return 0;
}