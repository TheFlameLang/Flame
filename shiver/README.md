## Shiver is build system for Flame

# Syntax of project.toml

name = "test" // name for project
version = "1.0" // version of project

output = "test" // -o test
license = "MIT" // license
author = "me" // author
github = "none" // github source link
[build] 
cxx = "g++" // compiler for generated C++ code
cxxflags = "-O0 -g -std=c++20 -pipe" // cxx flags
ld = "ld" // currently unused
src = ["testdir"] // directories with .flame files

# Usage

1. init project.toml with `shiver init skip`
2. edit project.toml for your project with src = ["codedir1", "codedir2"]
3. run `shiver build` or `shiver build verbose` for more info
