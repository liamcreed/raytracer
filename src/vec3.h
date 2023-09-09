#pragma once

typedef struct
{
    f64 x;
    f64 y;
    f64 z;
}vec3;

typedef struct
{
    f64 x;
    f64 y;
}vec2;

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
vec3 vec3_cross(vec3 v1, vec3 v2)
{
    return (vec3) {
        v1.y* v2.z - v1.z * v2.y,
            v1.z* v2.x - v1.x * v2.z,
            v1.x* v2.y - v1.y * v2.x
    };
}

vec3 vec3_reflect(vec3 v1, vec3 n)
{
    return vec3_subtract(v1, vec3_multiply_f64(n, 2 * vec3_dot(v1, n)));
}
vec3 vec3_refract(vec3 v, vec3 n, f64 i)
{
    f64 cos_theta = fmin(vec3_dot(vec3_multiply_f64(v, -1), n), 1.0);
    vec3 r_out_perp = vec3_multiply_f64(vec3_add(v, vec3_multiply_f64(n, cos_theta)), i);

    vec3 r_out_parallel = vec3_multiply_f64(n, -sqrt(fabs(1.0 - vec3_length_2(r_out_perp))));
    return vec3_add(r_out_perp, r_out_parallel);
}