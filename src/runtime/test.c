#include "../../include/runtime/oxygen_runtime.h"

#include <stdio.h>

int main() {
    string* str = (string*)malloc(sizeof(string));
    new_string(str, "Hi!");
    string_concat(str, " Im man!");
    unique_ptr* p = make_unique(str);
    printf("%s\n", p->val.str->data);
    free(str->data);
    free(str);
    destroy_unique(p);
    return 0;
}