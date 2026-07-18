#include <stdio.h>
#include <stdint.h>
#include <oxygen_runtime.h>
struct point {
    int32_t x=1;
    int32_t y=1;
};
float test(float * a)  {
    return *a;
}

int32_t main()  {
    string s;
oxygen_new_string( &s, "hi");
    int32_t a=10;
    a ^= 20;
    return 0;
}

