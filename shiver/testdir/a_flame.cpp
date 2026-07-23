#include <iostream>
#include <cstdint>
#include <array>
#include <vector>
#include <memory>
namespace stdlib {
#include <iostream>
#include <cstdint>
#include <array>
#include <vector>
#include <memory>
namespace stddef {
#include <iostream>
#include <cstdint>
#include <array>
#include <vector>
#include <memory>
const int64_t I64_MIN=(-9223372036854775807 - 1);
const int64_t I64_MAX=9223372036854775807;
const int32_t I32_MIN=-2147483648;
const int32_t I32_MAX=2147483647;
const int16_t I16_MIN=-32768;
const int16_t I16_MAX=32767;
const int8_t I8_MIN=-128;
const int8_t I8_MAX=127;
const uint64_t U64_MAX=-1;
const uint32_t U32_MAX=4294967295;
const uint16_t U16_MAX=65535;
const uint8_t U8_MAX=255;
const uint8_t EXIT_OK=0;
const uint8_t EXIT_ERR=1;
uint8_t test=0;
}
int64_t abs(int64_t a)  {
    if(a == stddef::I64_MIN) {
        return 9223372036854775807;
    };
    if(a > 0) {
        return a;
    };
    return -a;
}

const double PI=3.140000;
double sqrt(double a)  {
    double x=1.000000;
     for(int32_t i=0;(i <= 6);i++) {
        x=(0.500000 * (x + (a / x)));
    };
    return x;
}

double floor_helper(double a)  {
    double res=a;
    if(a < 0) {
        return (res - 1);
    };
    return res;
}

double floor(double a)  {
    if((a - floor_helper(a)) < 0.500000) {
        return floor_helper(a);
    };
    return floor_helper((a + 1));
}

double fmod(double a, double b)  {
    double res=(a - (b * floor((a / b))));
    return res;
}

double pow(double a, int8_t b)  {
    double res=a;
     for(int32_t i=1;(i < b);i++) {
        res *= a;
    };
    return res;
}

double fshiftr(double a, int8_t n)  {
    return (a / pow(2, n));
}

double fshiftl(double a, int8_t n)  {
    return (a * pow(2, n));
}

void println(std::string s)  {
    return;
}

}
struct point {
    int32_t x=1;
    int32_t y=1;
    void print_cord()  {
        std::cout << "x is" << x << " y is " << y;
    }
;
    void inc(int32_t x_, int32_t y_)  {
        x += x_;
        y += y_;
    }
;
};
float test_(const float &a)  {
    return a;
}

int32_t main()  {
    std::unique_ptr<point> p= std::make_unique<point>();
    p->inc(2, 2);
    p->print_cord();
    return 0;
}

