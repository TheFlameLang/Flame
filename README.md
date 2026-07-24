### Flame
Flame is programming language that transpiles into C++(switch to C soon :D)



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
   - borrow checker (in dev)
  
3. Good comptime diagnostic:

### Cons of using Flame:

1. Slow full compile time(transpilation + g++ compiling)
2. Weak comptime features compered to Zig:
   ```
   comptime i32 x = 10;
   comptime i32 y = 32+x;
   ```
3. Small standard library
4. Slow backend(currently switch to C)

# Installation 

Read step by step(recommended):

https://flame-lang.mintlify.site/Installation

Or download Linux release:

https://github.com/TheFlameLang/Flame/releases/tag/0.10.0-dev3-hotfix (Binary only, without stdlib and runtime for C)

# Testing

1. Create file

2. Write simple code:

```
func add(i32 x, i32 y) i32 {
   return x+y;
}

func main() i32 {
   print(add(3, 4), "\n"); // 7
   return 0;
}
```

3. run with `flame -CXX myfile.flame -o test && test`
