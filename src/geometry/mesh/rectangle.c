#include "geometry/mesh/rectangle.h"

void move_rectangle(Rectangle *rect, Vec v) {
    move_triangle(rect->abd, v);
    move_triangle(rect->bcd, v);
}


void rotate_rectangle(Rectangle *rect, Vec rotation) {
    Vec ab, ad, nb, nd;

    ab = v_sub(rect->abd->b, rect->abd->a);
    ad = v_sub(rect->abd->c, rect->abd->a);

    ab = rotate(ab, rotation);
    ad = rotate(ad, rotation);

    nb = v_add(rect->abd->a, ab);
    nd = v_add(rect->abd->a, ad);

    rect->abd->b = nb;
    rect->abd->c = nd;
    rect->bcd->a = nb;
    rect->bcd->c = v_add(rect->abd->a, v_add(ab, ad));
    rect->bcd->c = nd;
}