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
 * Object count
 * ============================================================
 */

#define TOTAL_OBJECTS 500000


/*
 * ============================================================
 * Grid
 * ============================================================
 *
 * 100 x 100 x 50 = 500,000 triangles
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

        .max_depth = 2,

        .aa_samples = 1
    };


    /*
     * ============================================================
     * Camera
     * ============================================================
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
     * Triangle generation
     * ============================================================
     *
     * Exactly 500,000 triangles:
     *
     *     100 x 100 x 50
     *
     * The spatial distribution is the same as the triangle
     * portion of the previous test scene.
     *
     * Materials are distributed deterministically:
     *
     *     index % 4 == 0 -> sphere material
     *     index % 4 == 1 -> box material
     *     index % 4 == 2 -> triangle material
     *     index % 4 == 3 -> ground material
     *
     * This gives approximately 125,000 triangles of each
     * material.
     */

    int material_counts[4] = {0, 0, 0, 0};


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
                 */

                px += random_double(-20.0, 20.0);
                py += random_double(-20.0, 20.0);
                pz += random_double(-20.0, 20.0);


                /*
                 * ------------------------------------------------
                 * Triangle dimensions
                 * ------------------------------------------------
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
                 * ------------------------------------------------
                 * Triangle geometry
                 * ------------------------------------------------
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


                /*
                 * ------------------------------------------------
                 * Material distribution
                 * ------------------------------------------------
                 */

                Material *material;

                switch (index % 3) {

                    case 0:
                        material = &sphere_mat;
                        material_counts[0]++;
                        break;

                    case 1:
                        material = &box_mat;
                        material_counts[1]++;
                        break;

                    case 2:
                        material = &triangle_mat;
                        material_counts[2]++;
                        break;

                    default:
                        material = &ground_mat;
                        material_counts[3]++;
                        break;
                }


                add_triangle(
                    &scene,
                    v0,
                    v1,
                    v2,
                    material
                );
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
        "Triangles: %d\n",
        TOTAL_OBJECTS
    );

    printf(
        "Sphere material:   %d\n",
        material_counts[0]
    );

    printf(
        "Box material:      %d\n",
        material_counts[1]
    );

    printf(
        "Triangle material: %d\n",
        material_counts[2]
    );

    printf(
        "Ground material:   %d\n",
        material_counts[3]
    );

    printf(
        "Total:              %d\n",
        material_counts[0]
        + material_counts[1]
        + material_counts[2]
        + material_counts[3]
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