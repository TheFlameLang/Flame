#include <iostream>
#include <cstdint>
#include <array>
#include <vector>
#include <memory>
int32_t pow2(int32_t a)  {
    return (a * a);
}

int32_t main()  {
     for(int32_t i=0;(i < 100);i += 2) {
        std::cout << pow2(i);
    };
    int32_t a=0;
    a += 2;
    int32_t pow3(int32_t a)  {
        return ((a * a) * a);
    }
;
    std::cout << a;
    return 0;
}

