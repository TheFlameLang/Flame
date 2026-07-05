### Flame
Flame is programming language that transpiles into C++



#### Pros of using Flame:

1. Modern syntax:
   ```
   i32 x = 42;
   func inc(ref i32 var) void {
     var++;
   }
   ```

2. Memory safety:
   - no use-after-free
   - no double free
   - no nullptr
  
3. Good comptime diagnostic:

### Cons of using Flame:

1. Slow full compile time(transpilation + g++ compiling)
2. Weak comptime features compered to Zig:
   ```
   comptime i32 x = 10;
   comptime i32 y = 32+x;
   ```
3. Small standard library
