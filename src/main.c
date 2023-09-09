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

#include "vec3.h"

//---------------------------//

typedef struct
{
    vec3 albedo;
    f64 specular;
    bool reflective;
    bool refractive;
}material_t;

typedef struct
{
    vec3 location;
    f64 radius;
    material_t* material;
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

typedef struct
{
    bool intersection;
    vec3 location;
    vec3 normal;
    f64 t;
    material_t* material;
}intersection_t;


intersection_t ray_hit_sphere(ray_t* ray, sphere_t* sphere)
{
    vec3 oc = vec3_subtract(ray->origin, sphere->location);

    f64 a = vec3_dot(ray->direction, ray->direction);
    f64 half_b = vec3_dot(oc, ray->direction);
    f64 c = vec3_length_2(oc) - sphere->radius * sphere->radius;
    f64 D = (half_b * half_b) - a * c;

    f64 t = INFINITY;

    if (D >= 0)
        t = (-half_b - sqrt(D)) / a;

    intersection_t result;
    result.t = t;
    if (result.t > 1e-7 && result.t != INFINITY)
    {
        result.location = vec3_add(ray->origin, vec3_multiply_f64(ray->direction, result.t));
        result.normal = vec3_normalize(vec3_subtract(result.location, sphere->location));
        result.material = sphere->material;
        result.intersection = true;
    }
    else
        result.intersection = false;

    return result;
}

typedef struct
{
    vec3 position;
    vec3 normal;
    vec2 uv;
}vertex_t;

typedef struct
{
    vertex_t vertices[3];
    material_t* material;
}triangle_t;


intersection_t ray_hit_triangle(ray_t* ray, triangle_t* triangle)
{
    intersection_t result;
    result.t = 0;
    
    if (result.t > 1e-7 && result.t != INFINITY)
    {
        result.location = vec3_add(ray->origin, vec3_multiply_f64(ray->direction, result.t));
        result.normal = vec3_normalize(result.location);
        result.material = triangle->material;
        result.intersection = true;
    }
    else
        result.intersection = false;
    return result;
}

triangle_t triangle =
{
    .vertices[0] = {{0.5,1.0,0.0},{},{}},
    .vertices[1] = {{1.0, -1.0, 0.0},{},{}},
    .vertices[2] = {{-1.0,-1.0, 0.0},{},{}},
};

vec3 ray_get_color(ray_t* ray, scene_t* scene, i32 ray_count)
{
    if (ray_count <= 0)
        return (vec3) { 1, 1, 1 };

    intersection_t intersection = { .t = INFINITY };

    for (i32 s = 0; s < scene->sphere_count; s++)
    {
        intersection_t i = ray_hit_sphere(ray, &scene->spheres[s]);
        if (i.t < intersection.t && i.t > 1e-7)
        {
            intersection = i;
        }
    }

    material_t triangle_mat = {.albedo = {1.0,0.0,0.0}};
    triangle.material = &triangle_mat;
    intersection_t triangle_intersection = ray_hit_triangle(ray, &triangle);

    if (triangle_intersection.intersection && triangle_intersection.t < intersection.t)
        intersection = triangle_intersection;
        

    vec3 color = {};
    if (intersection.intersection)
    {
        color = intersection.material->albedo;

        vec3 light_dir = vec3_normalize((vec3) { -1, -1, -1 });

        f64 ambient_factor = 0.3;
        vec3 ambient = vec3_multiply_f64(intersection.material->albedo, ambient_factor);

        f64 shinyness;

        if (intersection.material->reflective)
        {
            ray_t reflection_ray =
            {
                .origin = intersection.location,
                .direction = vec3_reflect(vec3_normalize(ray->direction), intersection.normal)
            };

            color = vec3_multiply(ray_get_color(&reflection_ray, scene, ray_count - 1), color);
            shinyness = 256;
        }
        else
        {
            color = intersection.material->albedo;
            f64 diffuse = fmax(vec3_dot(intersection.normal, vec3_multiply_f64(light_dir, -1)), 0.0);

            color = vec3_multiply_f64(color, diffuse * (1.0 - ambient_factor));
            color = vec3_add(color, ambient);
            shinyness = 256 * intersection.material->specular;
        }

        vec3 view_dir = vec3_normalize(vec3_subtract(ray->origin, intersection.location));
        vec3 reflecion = vec3_reflect(light_dir, intersection.normal);
        float spec = pow(fmax(vec3_dot(view_dir, reflecion), 0.0), shinyness);
        vec3 specular = vec3_multiply_f64((vec3) { .5, .5, .5 }, spec* intersection.material->specular);
        color = vec3_add(color, specular);


        ray_t shadow_ray =
        {
            .origin = intersection.location,
            .direction = vec3_multiply_f64(light_dir, -1)
        };
        bool in_shadow = false;
        for (i32 s = 0; s < scene->sphere_count; s++)
        {
            intersection_t i = ray_hit_sphere(&shadow_ray, &scene->spheres[s]);
            if (i.intersection && !intersection.material->reflective)
                in_shadow = true;
        }
        if (in_shadow)
            color = vec3_multiply_f64(color, ambient_factor);
    }
    else
    {
        // background
        f64 a = ray->direction.y * 0.5 + 0.5;
        color = (vec3){ 1 - a + 0.5, 1 - a + 0.5, 1.0 };
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
        .height = 1080 ,
        .aspect = 1440.0 / 1080.0
    };
    FILE* file = fopen("images/image.ppm", "wt");
    fprintf(file, "P6\n%d %d\n255\n", image.width, image.height);

    scene_t scene =
    {
        .camera =
        {
            .focal_length = 3.0,
            .location = {0,2,15.0}
        }
    };
    material_t material1 = { .albedo = { 1.0, 0.05,1.0 }, .specular = 1.0, .reflective = false, };
    material_t material2 = { .albedo = { 1.0,1.0,1.0 }, .specular = 1.0, .reflective = true, };
    material_t material3 = { .albedo = { 0.4,0.6,1.0 }, .specular = 1.0, .reflective = true, };
    material_t material4 = { .albedo = { 1.0,1.0,1.0 }, .specular = 0.0, .reflective = false, };

    scene_add_sphere(&scene, (sphere_t) { .material = &material1, .location = { -2.0,0.0,1.0 }, .radius = 0.5 });
    scene_add_sphere(&scene, (sphere_t) { .material = &material2, .location = { 1.0,2.5,0.0 }, .radius = 1.4 });
    scene_add_sphere(&scene, (sphere_t) { .material = &material3, .location = { 2.0,0.5,0 }, .radius = 0.5 });
    scene_add_sphere(&scene, (sphere_t) { .material = &material4, .location = { 0.0,-1000.5,0.0 }, .radius = 1000.0 }); // plane

    clock_t begin_clock = clock();
    i32 i = 0;
    for (i32 y = 0; y < image.height; y++)
    {
        i += 1;
        if (i > image.width/100.0)
        {
            printf("\rprogress %.2f", (f64)(y) / (f64)(image.height));
            fflush(stdout);
            i = 0;
        }

        for (i32 x = 0; x < image.width; x++)
        {
            ray_t ray =
            {
                .origin = scene.camera.location,
                .direction =
                {
                    (((f64)x / (f64)image.width) * 2 - 1) * image.aspect,
                    -(((f64)y / (f64)image.height) * 2 - 1),
                    -scene.camera.focal_length}
            };

            vec3 color = vec3_clamp(ray_get_color(&ray, &scene, 5), (vec3) { 0, 0, 0 }, (vec3) { 1.0, 1.0, 1.0 });

            u8 rgb[3] = { pow(color.x,1 / GAMMA) * 255, pow(color.y,1 / GAMMA) * 255, pow(color.z,1 / GAMMA) * 255 };
            fwrite(rgb, 1, 3, file);
        }
    }


    clock_t end_clock = clock();
    printf("[time]: %f s\n", ((f64){end_clock - begin_clock} / CLOCKS_PER_SEC));

    fclose(file);

    return 0;
}

