#include <stdio.h>
#include <stdint.h>
#include <oxygen_runtime.h>
const int64_t stddef_flame_I64_MIN=(-9223372036854775807 - 1);
int64_t stdlib_flame_abs(int64_t stdlib_flame_a)  {
    if((stdlib_flame_a == stddef_flame_I64_MIN)) {
        return 9223372036854775807;
    };
    if((stdlib_flame_a > 0)) {
        return stdlib_flame_a;
    };
    return -stdlib_flame_a;
}

const double stdlib_flame_PI=3.140000;
double stdlib_flame_sqrt(double stdlib_flame_a)  {
    double stdlib_flame_x=1.000000;
     for(int32_t stdlib_flame_i=0;(stdlib_flame_i <= 6);stdlib_flame_i++) {
        stdlib_flame_x=(0.500000 * (stdlib_flame_x + (stdlib_flame_a / stdlib_flame_x)));
    };
    return stdlib_flame_x;
}

double stdlib_flame_floor_helper(double stdlib_flame_a)  {
    double stdlib_flame_res=stdlib_flame_a;
    if((stdlib_flame_a < 0)) {
        return (stdlib_flame_res - 1);
    };
    return stdlib_flame_res;
}

double stdlib_flame_floor(double stdlib_flame_a)  {
    if(((stdlib_flame_a - stdlib_flame_floor_helper(stdlib_flame_a)) < 0.500000)) {
        return stdlib_flame_floor_helper(stdlib_flame_a);
    };
    return stdlib_flame_floor_helper((stdlib_flame_a + 1));
}

double stdlib_flame_fmod(double stdlib_flame_a, double stdlib_flame_b)  {
    double stdlib_flame_res=(stdlib_flame_a - (stdlib_flame_b * stdlib_flame_floor((stdlib_flame_a / stdlib_flame_b))));
    return stdlib_flame_res;
}

double stdlib_flame_pow(double stdlib_flame_a, int8_t stdlib_flame_b)  {
    double stdlib_flame_res=stdlib_flame_a;
     for(int32_t stdlib_flame_i=1;(stdlib_flame_i < stdlib_flame_b);stdlib_flame_i++) {
        stdlib_flame_res *= stdlib_flame_a;
    };
    return stdlib_flame_res;
}

double stdlib_flame_fshiftr(double stdlib_flame_a, int8_t stdlib_flame_n)  {
    return (stdlib_flame_a / stdlib_flame_pow(2, stdlib_flame_n));
}

double stdlib_flame_fshiftl(double stdlib_flame_a, int8_t stdlib_flame_n)  {
    return (stdlib_flame_a * stdlib_flame_pow(2, stdlib_flame_n));
}

void stdlib_flame_println(string stdlib_flame_s)  {
    return;
}

typedef struct point {
    int32_t x;
    int32_t y;
} point;
point point_flame_def_init() {
    point Temp;
    Temp.x = 1;
    Temp.y = 1;
    return Temp;
};
float test(float * a)  {
    return *a;
}

int32_t main()  {
    int32_t* t = malloc(sizeof(int32_t));
    *t = 3;
    point p = point_flame_def_init();
    int32_t x[3] = {1, 2, 3};
    string s;
    oxygen_new_string( &s, "hi");
    int32_t a=10;
    a ^= 20;
    a=stdlib_flame_PI;
    oxygen_string_destroy(&s);
    free(t);
    return a;
}

