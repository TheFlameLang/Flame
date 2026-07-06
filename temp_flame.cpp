#include <iostream>
#include <cstdint>
#include <array>
#include <vector>
namespace stdlib {
#include <iostream>
#include <cstdint>
#include <array>
#include <vector>
int64_t abs(int64_t a)  {
    if((a > 0)) {
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
    int64_t res=a;
    if(((a < 0) && (a != res))) {
        return (res - 1);
    };
    return res;
}

double floor(double a)  {
    if(((a - floor_helper(a)) < 0.500000)) {
        return floor_helper(a);
    };
    return floor_helper((a + 1));
}

double fmod(double a, double b)  {
    double res=(a - (b * floor((a / b))));
    return res;
}

double pow(double a, int32_t b)  {
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

}
void printw(std::string s)  {
    std::cout << s;
}

struct point {
    int32_t x=0;
    int32_t y=0;
    void inc()  {
        x++;
    }
;
};
int32_t main()  {
    printw("hi");
    point p;
    p.x=3;
    float x=-2;
    x=stdlib::PI;
    std::cout << x;
    return 0;
}

