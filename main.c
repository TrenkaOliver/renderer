#include <stdio.h>
#include <stdint.h>
#include <time.h>

#include "render/render.h"
#include "light/material.h"
#include "scene/scene.h"
#include "camera/camera.h"
#include "math/vec.h"


/*
 * ============================================================
 * Primitive toggles
 * ============================================================
 *
 * 1 = generate
 * 0 = don't generate
 *
 * Every cell is still processed regardless of these settings.
 *
 * Therefore, disabling a primitive simply leaves its cells empty.
 *
 * With all three set to 1, the generated scene is identical
 * to the original scene.
 */

#define GENERATE_SPHERES   1
#define GENERATE_BOXES     1
#define GENERATE_TRIANGLES 1


/*
 * ============================================================
 * Object counts
 * ============================================================
 */

#define TOTAL_OBJECTS 500000

#define SPHERE_COUNT   166667
#define BOX_COUNT      166667
#define TRIANGLE_COUNT 166666


/*
 * ============================================================
 * Grid
 * ============================================================
 *
 * 100 x 100 x 50 = 500,000 cells
 *
 * Each cell gets assigned a primitive type.
 * Disabled primitive types simply leave their cell empty.
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
         */
        .max_depth = 2,

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
        vec(0.0, -1000.0, 500.0),
        vec(0.0, 0.0, 500.0),
        1.0472
    );


    /*
     * ============================================================
     * Directional light
     * ============================================================
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
     * IMPORTANT:
     *
     * The loop ALWAYS processes all 500,000 cells.
     *
     * The primitive type is ALWAYS selected using:
     *
     *     index % 3
     *
     * exactly as in the original scene.
     *
     * If the selected primitive is disabled, nothing is spawned.
     *
     * Random numbers are still consumed before the decision,
     * keeping the RNG sequence identical between configurations.
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
                 * ------------------------------------------------
                 * Positional jitter
                 * ------------------------------------------------
                 *
                 * These random calls happen regardless of which
                 * primitive is enabled.
                 */

                px += random_double(-20.0, 20.0);
                py += random_double(-20.0, 20.0);
                pz += random_double(-20.0, 20.0);


                /*
                 * ------------------------------------------------
                 * Object size
                 * ------------------------------------------------
                 *
                 * Also generated regardless of primitive type.
                 */

                double size =
                    random_double(15.0, 35.0);


                /*
                 * =================================================
                 * Select primitive
                 * =================================================
                 *
                 * This is exactly the original distribution:
                 *
                 *   index % 3 == 0 -> sphere
                 *   index % 3 == 1 -> box
                 *   index % 3 == 2 -> triangle
                 *
                 * Disabled types simply result in no object.
                 */

                int type = index % 3;


                /*
                 * =================================================
                 * SPHERE
                 * =================================================
                 */

                if (type == 0) {

#if GENERATE_SPHERES

                    add_sphere(
                        &scene,
                        vec(px, py, pz),
                        size,
                        &sphere_mat
                    );

                    spheres++;

#endif
                }


                /*
                 * =================================================
                 * BOX
                 * =================================================
                 */

                else if (type == 1) {

#if GENERATE_BOXES

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

#endif
                }


                /*
                 * =================================================
                 * TRIANGLE
                 * =================================================
                 */

                else {

#if GENERATE_TRIANGLES

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
                     * Randomly deform triangle in 3D.
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

#endif
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