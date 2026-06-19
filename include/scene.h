#include "geometry/object.h"
#include "geometry/mesh/rectangle.h"
#include "geometry/mesh/box.h"
#include "geometry/vec.h"
#include "graphics/light.h"

typedef struct Spheres {
    Sphere *sp;
    size_t count;
    size_t capacity;
} Spheres;

typedef struct Planes {
    Plane *pp;
    size_t count;
    size_t capacity;
} Planes;

typedef struct Triangles {
    Triangle *tp;
    size_t count;
    size_t capacity;
} Triangles;


typedef struct Scene {
    DirectionalLight dir_light;
    Vec global_ambient;
    Spheres spheres;
    Planes planes;
    Triangles triangles;
} Scene;

Scene create_scene();

Sphere *add_sphere(Scene *scene, Vec center, double radious, Material *material);

Plane *add_plane(Scene *scene, Vec point, Vec normal, Material *material);

Triangle *add_triangle(Scene *scene, Vec a, Vec b, Vec c, Material *material);
Triangle *copy_triangle(Scene *scene, Triangle *triangle);

Rectangle add_rectangle(Scene *scene, Vec position, Vec rotation, Vec size, Material *m);
Rectangle add_rectangle_by_points(Scene *scene, Vec a, Vec b, Vec c, Vec d, Material *m);
Rectangle copy_rectangle(Scene *scene, Rectangle *rect);

Box add_box(Scene *scene, Vec position, Vec rotation, Vec size, Material *m);
