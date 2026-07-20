#include "../../include/runtime/oxygen_runtime.h"

#include <stdio.h>

int main() {
    string str;
    new_string(&str, "Hi!");
    string_concat(&str, " Im man!");
    printf("%s\n", str.data);
    free(str.data);
    return 0;
}