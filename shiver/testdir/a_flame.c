#include <stdio.h>
#include <stdint.h>
#include <oxygen_runtime.h>
#define OXYGEN_REQ_MAJ_V 0
#define OXYGEN_REQ_MIN_V 2
#define OXYGEN_REQ_PAT_V 0
#if OXYGEN_MAJOR_VER < OXYGEN_REQ_MAJ_V
    #error "Using very out-dated version of Oxygen Runtime!"
#elif OXYGEN_MINOR_VER < OXYGEN_REQ_MIN_V
    #warning "Using old version of Oxygen Runtime!"
#elif OXYGEN_PATCH_VER < OXYGEN_REQ_PAT_V
    #warning "Using older release of Oxygen Runtime!"
#endif
const int64_t stddef_flame_I64_MIN=(-9223372036854775807 - 1);
const int64_t stddef_flame_I64_MAX=9223372036854775807;
const int32_t stddef_flame_I32_MIN=-2147483648;
const int32_t stddef_flame_I32_MAX=2147483647;
const int16_t stddef_flame_I16_MIN=-32768;
const int16_t stddef_flame_I16_MAX=32767;
const int8_t stddef_flame_I8_MIN=-128;
const int8_t stddef_flame_I8_MAX=127;
const uint64_t stddef_flame_U64_MAX=-1;
const uint32_t stddef_flame_U32_MAX=4294967295;
const uint16_t stddef_flame_U16_MAX=65535;
const uint8_t stddef_flame_U8_MAX=255;
const uint8_t stddef_flame_EXIT_OK=0;
const uint8_t stddef_flame_EXIT_ERR=1;
int64_t stdlib_flame_abs(int64_t stdlib_flame_a)  {
    if(stdlib_flame_a == stddef_flame_I64_MIN) {
        return 9223372036854775807;
    };
    if(stdlib_flame_a > 0) {
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
    if(stdlib_flame_a < 0) {
        return (stdlib_flame_res - 1);
    };
    return stdlib_flame_res;
}

double stdlib_flame_floor(double stdlib_flame_a)  {
    if((stdlib_flame_a - stdlib_flame_floor_helper(stdlib_flame_a)) < 0.500000) {
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
}
void point_print_cord(point* self)  {
        printf("%s", oxygen_fmt("x is {}, y is {}\n", "ii", self->x, self->y));
    }
void point_inc(point* self, int32_t x_int32_t y_)  {
        x += x_;
        y += y_;
    }
;
float test(float * a)  {
    return *a;
}

int32_t main()  {
    printf("%s", oxygen_fmt("{} is 1\n", "b", 1));
    point* p = malloc(sizeof(point));
    *p = point_flame_def_init();
    point_inc(p, 2, 2);
    point_print_cord(p);
    free(p);
    return 0;
}

