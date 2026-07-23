# Changelog from 11.07.2026

### Flame 0.9.0(11.07.26):

* new module - strings

* string concatation with += or strings.concat(s1, s2)

* new module system

* no more manual free for pointers (it automaticaly frees after exiting scope)

* borrow checker in dev:

* one owner:
    ```js
    i32* a = 3;
    i32* b = a;
    print(a); // <- a no longer exist, its moved to b
    ```

#### Shiver 1.0(11.07.26):

* verbose

* build time


### Flame 0.10.0(21.07.26):

* mutable and not mutable references(**function arguments only**)
* new style of warnings
* now only one mutable refernce can be defined for var
* switching to C backend
* -t flag for only transpilation
* stddef library
* std.fmt()

* **BUGFIXES**:
    
    * Now pointers realy moves if i32* b = a;
    * arguments now can be consts(bug dont allowed it)
    * functions was overlap each other
    * bug was making that argument type and provided variable type even if mismatch it was working
    * string tokens didnt have .str_value
    * accessing string with compile time index 

### Shiver 1.1

* Added Flameflags parameter for build
* minor changes