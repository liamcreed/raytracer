#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <time.h>
#include <assert.h>

typedef float f32;
typedef double f64;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef struct
{
    f64 x;
    f64 y;
    f64 z;
}vec3;

vec3 vec3_multiply(vec3 v1, vec3 v2)
{
    return (vec3) { v1.x* v2.x, v1.y* v2.y, v1.z* v2.z };
}
vec3 vec3_add(vec3 v1, vec3 v2)
{
    return (vec3) { v1.x + v2.x, v1.y + v2.y, v1.z + v2.z };
}
vec3 vec3_subtract(vec3 v1, vec3 v2)
{
    return (vec3) { v1.x - v2.x, v1.y - v2.y, v1.z - v2.z };
}
vec3 vec3_divide(vec3 v1, vec3 v2)
{
    return (vec3) { v1.x / v2.x, v1.y / v2.y, v1.z / v2.z };
}
vec3 vec3_multiply_f64(vec3 v1, f64 f)
{
    return (vec3) { v1.x* f, v1.y* f, v1.z* f };
}
f64 vec3_dot(vec3 v1, vec3 v2)
{
    return (v1.x * v2.x + v1.y * v2.y + v1.z * v2.z);
}
f64 vec3_length_2(vec3 v1)
{
    return (v1.x * v1.x + v1.y * v1.y + v1.z * v1.z);
}
f64 vec3_length(vec3 v1)
{
    return sqrt(v1.x * v1.x + v1.y * v1.y + v1.z * v1.z);
}
vec3 vec3_normalize(vec3 v1)
{
    f64 length = vec3_length(v1);
    return (vec3) { v1.x / length, v1.y / length, v1.z / length };
}
vec3 vec3_clamp(vec3 v1, vec3 min, vec3 max)
{
    vec3 r = v1;
    if (v1.x > max.x)
        r.x = max.x;
    else if (v1.x < min.x)
        r.x = min.x;

    if (v1.y > max.y)
        r.y = max.y;
    else if (v1.y < min.y)
        r.y = min.y;

    if (v1.z > max.z)
        r.z = max.z;
    else if (v1.z < min.z)
        r.z = min.z;
    return r;
}

f64 f64_max(f64 f1, f64 f2)
{
    if (f1 > f2)
        return f1;
    return f2;
}

//--------------------------------//

typedef struct
{
    vec3 location;
    f64 radius;
    vec3 albedo;
    f64 roughness;
}sphere_t;

typedef struct
{
    vec3 location;
    f64 focal_length;
}camera_t;


#define MAX_SPHERE_COUNT 32
typedef struct
{
    u32 sphere_count;
    sphere_t spheres[MAX_SPHERE_COUNT];

    camera_t camera;
}scene_t;

void scene_add_sphere(scene_t* scene, sphere_t sphere)
{
    scene->spheres[scene->sphere_count] = sphere;
    scene->sphere_count++;
}

//--------------------------------//

typedef struct
{
    vec3 origin;
    vec3 direction;
}ray_t;

f64 ray_hit_sphere(ray_t* ray, sphere_t* sphere)
{
    vec3 oc = vec3_subtract(ray->origin, sphere->location);

    f64 a = vec3_dot(ray->direction, ray->direction);
    f64 b = 2.0 * vec3_dot(oc, ray->direction);
    f64 c = vec3_dot(oc, oc) - sphere->radius * sphere->radius;
    f64 D = (b * b) - 4.0 * a * c;

    f64 t = INFINITY;

    if (D >= 0)
        t = (-b - sqrt(D)) / (2.0 * a);
    return t;
}

vec3 ray_get_color(ray_t* ray, scene_t* scene)
{
    f64 closest_t = INFINITY;
    sphere_t* closest_sphere = NULL;
    for (i32 i = 0; i < scene->sphere_count; i++)
    {
        f64 t = ray_hit_sphere(ray, &scene->spheres[i]);
        if (t < closest_t && t > 0.0)
        {
            closest_t = t;
            closest_sphere = &scene->spheres[i];
        }
    }

    vec3 color = {};
    if (closest_sphere != NULL && closest_t > 1e-4)
    {
        vec3 hit_pos = vec3_add(ray->origin, vec3_multiply_f64(ray->direction, closest_t));
        vec3 normal = vec3_normalize(vec3_subtract(hit_pos, closest_sphere->location));
        
        f32 diffuse = f64_max(vec3_dot(normal, (vec3){1,1,1}), 0.0);
        color = vec3_multiply_f64(closest_sphere->albedo, diffuse);

        ray_t shadow_ray =
        {
            .origin = hit_pos,
            .direction = {1,1,0}
        };
        bool in_shadow = false;
        for (i32 i = 0; i < scene->sphere_count; i++)
        {
            f64 t = ray_hit_sphere(&shadow_ray, &scene->spheres[i]);
            if(t != INFINITY && t > 1e-4)
                in_shadow = true;
        }
        if(in_shadow)
            return (vec3){0,0,0};
        else
            return color;
        
    }
    else
    {
        f64 a = ray->direction.y * 0.5 + 0.5;
        color = (vec3){ (1.0 - a) + 0.5,a + 0.4,1.0 };
    }
    return color;
}

//--------------------------------//

typedef struct
{
    i32 width;
    i32 height;
    f64 aspect;
}image_t;

#define GAMMA 2.2

int main(int argc, char const* argv[])
{
    srand(time(NULL));

    image_t image =
    {
        .width = 1440,
        .height = 1080,
        .aspect = 1440.0 / 1080.0
    };
    FILE* file = fopen("image.ppm", "wt");
    fprintf(file, "P6\n%d %d\n255\n", image.width, image.height);

    scene_t scene =
    {
        .camera =
        {
            .focal_length = 3.0,
            .location = {0,0,9.0}
        }
    };

    sphere_t sphere = { .albedo = {1.0,0.0,1.0}, .location = {-1.0,0.0,0.0}, .radius = 0.5 };
    sphere_t sphere2 = { .albedo = {0.0,1.0,1.0}, .location = {1.0,0.0,4.0}, .radius = 0.5 };
    sphere_t plane = { .albedo = {0.1,0.3,1.0}, .location = {0.0,-1000.5,0.0}, .radius = 1000.0 };
    scene_add_sphere(&scene, sphere);
    scene_add_sphere(&scene, sphere2);
    scene_add_sphere(&scene, plane);

    clock_t begin_clock = clock();
    for (i32 y = 0; y < image.height; y++)
    {
        for (i32 x = 0; x < image.width; x++)
        {
            ray_t ray =
            {
                .origin = scene.camera.location,
                .direction = {
                    (((f64)x / (f64)image.width) * 2 - 1) * image.aspect,
                    -(((f64)y / (f64)image.height) * 2 - 1),
                    -scene.camera.focal_length}
            };

            vec3 color = vec3_clamp(ray_get_color(&ray, &scene), (vec3) { 0, 0, 0 }, (vec3) { 1.0, 1.0, 1.0 });

            u8 rgb[3] = { pow(color.x,1 / GAMMA) * 255, pow(color.y,1 / GAMMA) * 255, pow(color.z,1 / GAMMA) * 255 };
            fwrite(rgb, 1, 3, file);
        }
    }
    clock_t end_clock = clock();
    printf("[time]: %f s\n", ((f64){end_clock - begin_clock} / CLOCKS_PER_SEC));

    fclose(file);

    return 0;
}