#include <iostream>
#include <cstdint>
#include <array>
#include <vector>
#include <memory>
int32_t main()  {
    int32_t i=0;
    while((i < 4))  {
        std::cout << i;
        if(i == 2) {
            break;
        };
        i=(i + 1);
    };
    return 0;
}

