#include "scene/sample.h"

int clampi(int x, int min, int max) {
    if (x < min) return min;
    if (x > max) return max;
    return x;
}

void environment_uv(Vec dir, double *u, double *v) {
    dir = normalize(dir);

    double theta = atan2(dir.y, dir.x);
    double phi = asin(clampd(dir.z, -1.0, 1.0));

    *u = theta / (2.0 * M_PI) + 0.5;
    *v = 0.5 - phi / M_PI;
}

Vec sample(Vec N, const Texture *map) {
    double u, v;

    environment_uv(N, &u, &v);

    double x = u * map->w;
    double y = v * (map->h - 1);

    int x0 = floor(x);
    int y0 = floor(y);

    double fx = x - x0;
    double fy = y - y0;

    x0 = (x0 % map->w + map->w) % map->w;

    int x1 = (x0 + 1) % map->w;

    y0 = clampi(y0, 0, map->h - 1);
    
    int y1 = clampi(y0 + 1, 0, map->h - 1);

    float *data = (float *)map->ptr;

    Vec c00 = vec(
        data[3 * map->w * y0 + 3 * x0 + 0],
        data[3 * map->w * y0 + 3 * x0 + 1],
        data[3 * map->w * y0 + 3 * x0 + 2]
    );

    Vec c01 = vec(
        data[3 * map->w * y0 + 3 * x1 + 0],
        data[3 * map->w * y0 + 3 * x1 + 1],
        data[3 * map->w * y0 + 3 * x1 + 2]
    );

    Vec c10 = vec(
        data[3 * map->w * y1 + 3 * x0 + 0],
        data[3 * map->w * y1 + 3 * x0 + 1],
        data[3 * map->w * y1 + 3 * x0 + 2]
    );

    Vec c11 = vec(
        data[3 * map->w * y1 + 3 * x1 + 0],
        data[3 * map->w * y1 + 3 * x1 + 1],
        data[3 * map->w * y1 + 3 * x1 + 2]
    );

    Vec c0 = v_add(scale(c00, 1.0 - fx), scale(c01, fx));
    Vec c1 = v_add(scale(c10, 1.0 - fx), scale(c11, fx));

    return v_add(scale(c0, 1.0 - fy), scale(c1, fy));
}

Texture create_texture(uint32_t w, uint32_t h, uint32_t s) {
    return (Texture) {
        .w = w,
        .h = h,
        .ptr = malloc(w * h * s)
    };
} 

double radical_inverse(unsigned k) {
    double res = 0.0;
    double w = 0.5;

    while (k > 0) {
        if (k & 1) res += w;

        k >>= 1;
        w *= 0.5;
    }
    
    return res;
}

void hammersley(uint32_t i, uint32_t n, double * restrict u, double * restrict v) {
    *u = (double)i / (double)n;
    *v = radical_inverse(i);
}



void make_coord_space(const Vec N, Vec * restrict T, Vec * restrict B) {
    Vec up;

    if (fabs(N.z) < 0.999) up = vec(0.0, 0.0, 1.0);
    else up = vec(0.0, 1.0, 0.0);

    *T = normalize(cross(up, N));
    *B = cross(N, *T);
}

Vec cosine_sample_hemisphere(double u1, double u2, Vec N) {
    double r = sqrt(u1);
    double phi = 2.0 * M_PI * u2;

    double x = r * cos(phi);
    double y = r * sin(phi);

    double z = sqrt(fmax(0.0, 1.0 - u1));

    Vec T, B;
    make_coord_space(N, &T, &B);

    return normalize(
        v_add(
            v_add(
                scale(T, x),
                scale(B, y)
            ),
            scale(N, z)
        )
    );
}

void generate_irradiance_map(const Texture * restrict environment, Texture * restrict irradiance) {
    const uint32_t w = 64;
    const uint32_t h = 32;

    const Texture texture = create_texture(w, h, 3 * sizeof(float));

    for (uint32_t i = 0; i < h; i++) {
        double v = ((double)i + 0.5) / (double)h;
        double phi = (0.5 - v) * M_PI;

        double sin_phi = sin(phi);
        double cos_phi = cos(phi);

        for (uint32_t j = 0; j < w; j++) {            
            double u = ((double)j + 0.5) / (double)w;
            double theta = (u - 0.5) * 2.0 * M_PI ;
            
            Vec N = vec(
                cos_phi * cos(theta),
                cos_phi * sin(theta),
                sin_phi
            );
                        
            Vec irradiance = vec(0.0, 0.0, 0.0);

            for (int k = 0; k < IRRADIANCE_SAMPLES; k++) {
                double su, sv;

                hammersley(k, IRRADIANCE_SAMPLES, &su, &sv);

                Vec L = cosine_sample_hemisphere(su, sv, N);

                double NdotL = fmax(0.0, dot(N, L));

                Vec c = sample(L, environment);

                irradiance = v_add(irradiance, scale(c, NdotL));
            }

            irradiance = scale(irradiance, M_PI / (double)IRRADIANCE_SAMPLES);

            ((float *)texture.ptr)[i * w * 3 + j * 3 + 0] = irradiance.x;
            ((float *)texture.ptr)[i * w * 3 + j * 3 + 1] = irradiance.y;
            ((float *)texture.ptr)[i * w * 3 + j * 3 + 2] = irradiance.z;
        }
    }

    *irradiance = texture;
}


Vec sample_prefiltered(Vec R, double roughness, Texture maps[]) {
    roughness = clampd(roughness, 0.0, 1.0);

    double level = roughness * (ENV_MIP_COUNT - 1);

    int i0 = floor(level);

    int i1 = clampi(i0 + 1, 0, ENV_MIP_COUNT - 1);

    double f = level - i0;

    Vec c0 = sample(R, maps + i0);
    Vec c1 = sample(R, maps + i1);

    return v_add(
        scale(c0, 1.0 - f),
        scale(c1, f)
    );
}

Vec importance_sample_GGX(double u1, double u2, double roughness, Vec N) {
    double alpha = roughness * roughness;
    double alpha2 = alpha * alpha;

    double phi = 2.0 * M_PI * u1;

    double cos_theta2 = (1.0 - u2) / (1.0 + (alpha2 - 1.0) * u2);
    double cos_theta = sqrt(cos_theta2);
    double sin_theta = sqrt(fmax(0.0, 1.0 - cos_theta2));

    Vec H_local = vec(
        sin_theta * cos(phi),
        sin_theta * sin(phi),
        cos_theta
    );

    Vec T, B;
    make_coord_space(N, &T, &B);

    Vec H = v_add(
        scale(T, H_local.x),
        v_add(
            scale(B, H_local.y),
            scale(N, H_local.z)
        )
    );

    return normalize(H);
}


void generate_prefiltered_layer(const Texture * restrict environment, Texture * restrict prefiltered, const double roughness) {
    uint32_t w = 256u >> (uint32_t)(roughness * (5));
    uint32_t h = 128u >> (uint32_t)(roughness * (5));

    if (w < 8) w = 0;
    if (h < 4) h = 4;

    Texture texture = create_texture(w, h, 3 * sizeof(float));

    float *data = (float *)texture.ptr;

    for (uint32_t i = 0; i < h; i++) {
        double v = ((double)i + 0.5) / (double)h;
        double phi = (0.5 - v) * M_PI;

        double sin_phi = sin(phi);
        double cos_phi = cos(phi);

        for (uint32_t j = 0; j < w; j++) {            
            double u = ((double)j + 0.5) / (double)w;
            double theta = (u - 0.5) * 2.0 * M_PI ;
            
            Vec R = vec(
                cos_phi * cos(theta),
                cos_phi * sin(theta),
                sin_phi
            );
            
            Vec N = R;
            Vec V = R;

            Vec color = vec(0.0, 0.0, 0.0);
            double total_weight = 0.0;

            if (roughness < 0.001) {
                color = sample(R, environment);
            } else {
                for (int k = 0; k < PREFILTER_SAMPLES; k++) {
                    double su, sv;

                    hammersley(k, PREFILTER_SAMPLES, &su, &sv);

                    Vec H = importance_sample_GGX(su, sv, roughness, N);

                    Vec L = v_sub(
                        scale(H, 2.0 * dot(V, H)),
                        V
                    );

                    L = normalize(L);

                    double NdotL = fmax(0.0, dot(N, L));

                    if (NdotL <= 0) continue;

                    Vec c = sample(L, environment);

                    color = v_add(color, scale(c, NdotL));

                    total_weight += NdotL;
                }

                if (total_weight > 0.0) {
                    color = scale(color, 1.0 / total_weight);
                }
            }

            data[i * w * 3 + j * 3 + 0] = color.x;
            data[i * w * 3 + j * 3 + 1] = color.y;
            data[i * w * 3 + j * 3 + 2] = color.z;
        }
    }

    *prefiltered = texture;
}

Vec sample_brdf_lut(double NdotV, double roughness, Texture *map) {
    NdotV = clampd(NdotV, 0.0, 1.0);
    roughness = clampd(roughness, 0.0, 1.0);

    double x = NdotV * (map->w - 1);
    double y = roughness * (map->h - 1);

    int x0 = floor(x);
    int y0 = floor(y);

    int x1 = clampi(x0 + 1, 0, map->w - 1);
    int y1 = clampi(y0 + 1, 0, map->h - 1);

    double fx = x - x0;
    double fy = y - y0;

    float *data = (float *)map->ptr;

    Vec c00 = {
        .x = data[y0 * map->w * 2 + x0 * 2 + 0],
        .y = data[y0 * map->w * 2 + x0 * 2 + 1],
        .z = 0.0
    };

    Vec c01 = {
        .x = data[y0 * map->w * 2 + x1 * 2 + 0],
        .y = data[y0 * map->w * 2 + x1 * 2 + 1],
        .z = 0.0
    };

    Vec c10 = {
        .x = data[y1 * map->w * 2 + x0 * 2 + 0],
        .y = data[y1 * map->w * 2 + x0 * 2 + 1],
        .z = 0.0
    };

    Vec c11 = {
        .x = data[y1 * map->w * 2 + x1 * 2 + 0],
        .y = data[y1 * map->w * 2 + x1 * 2 + 1],
        .z = 0.0
    };

    Vec c0 = v_add(scale(c00, 1.0 - fx), scale(c01, fx));
    Vec c1 = v_add(scale(c10, 1.0 - fx), scale(c11, fx));

    return v_add(scale(c0, 1.0 - fy), scale(c1, fy));
}

double geometry_schlick_GGX(double NdotV, double roughness) {
    double k = (roughness + 1.0) * (roughness + 1.0) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

void integrate_brdf(double NdotV, double roughness, double *A, double *B) {
    Vec V = vec(
        sqrt(fmax(0.0, 1.0 - NdotV * NdotV)), 
        0.0, 
        NdotV
    );

    Vec N = vec(0.0, 0.0, 1.0);

    double A_sum = 0.0;
    double B_sum = 0.0;

    for (uint32_t i = 0; i < BRDF_SAMPLES; i++) {
        double su, sv;

        hammersley(i, BRDF_SAMPLES, &su, &sv);

        Vec H = importance_sample_GGX(su, sv, roughness, N);

        Vec L = v_sub(
            scale(H, 2.0 * dot(V, H)),
            V
        );

        L = normalize(L);

        double NdotL = fmax(0.0, L.z);
        double NdotH = fmax(0.0, H.z);
        double VdotH = fmax(0.0, dot(V, H));

        if (NdotL > 0.0) {
            double G =  
            geometry_schlick_GGX(NdotV, roughness) * 
            geometry_schlick_GGX(NdotL, roughness);
            
            double denominator = NdotH * NdotV;

            if (denominator > 1e-8) {
                double G_vis = G * VdotH / denominator;

                double Fc = pow(1.0 - VdotH, 5.0);

                A_sum += (1.0 - Fc) * G_vis;
                B_sum += Fc * G_vis;
            }
        }
    }

    *A = A_sum / (double)BRDF_SAMPLES;
    *B = B_sum / (double)BRDF_SAMPLES;
}

void generate_brdf_lut(Texture *brdf_lut) {
    int w = 256;
    int h = 256;

    Texture texture = create_texture(w, h, 2 * sizeof(float));

    float *data = (float *)texture.ptr;

    for (int i = 0; i < h; i++) {
        double roughness = ((double)i + 0.5) / (double)h;
        
        for (int j = 0; j < w; j++) {
            double NdotV = ((double)j + 0.5) / (double)w;

            double A = 0.0;
            double B = 0.0;

            integrate_brdf(NdotV, roughness, &A, &B);

            data[i * w * 2 + j * 2 + 0] = A;
            data[i * w * 2 + j * 2 + 1] = B;
        }
    }

    *brdf_lut = texture;
}
