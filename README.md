# Sudoh
Sudoh is a simple programming language that transpiles down to C++ and then may be compiled and run from there on. Its
easily understandable syntax along with C++'s efficient nature make it well equipped for the task of software prototyping,
even for more computationally intensive tasks.

Sudoh can be described as a strongly-typed, dynamically type-checked, garbage-collected, procedural, high-level language.
Sudoh's syntax and many of its features were inspired in large part by common motifs that I've seen in algorithm pseudocode
examples, as well as by languages such as Python and JavaScript. The output C++ code that the Sudoh transpiler `sudoht`
generates is structured in a way to be very similar to the input Sudoh code, which allows making modifications to the actual
C++ code to be run quite simple.

Here is a simple program that prints the first `n` fibonacci numbers specified by standard input, written in Sudoh:
```
procedure fibonacci <- n
    num1 <- 0
    num2 <- 1
    for i <- 1 to n do
        printLine(num1)

        temp <- num1
        num1 <- num2
        num2 <- temp + num2

n <- number(input())
fibonacci(n)
```

Documentation for the Sudoh language can be found in **documentation.md**, and more examples can be found in the
**examples** directory


## Getting started with Sudoh

This is a tutorial on how to compile Sudoh programs on a Linux platform:

1.  Clone this repository into a directory of your choice.
    ```
    $ git clone https://github.com/alexsnezhko3/SudohLang.git
    ```

2.  Navigate into the SudohLang project and then execute the `setup.sh` bash script to set up the required binaries for
    Working with Sudoh. This will create a new executable program `sudoht` and a new static library `libsudoh.a` in the
    current directory. Note: `setup.sh` uses `g++` to compile the C++ source files.
    ```
    $ cd SudohLang/SudohLang
    $ ./setup.sh
    ```

3.  Create a Sudoh program. All Sudoh programs must have `.sud` file extensions. When done, transpile the Sudoh source code
    into C++ code with `sudoht`
    ```
    $ ./sudoht <source_file>.sud
    ```
    This will then create new `.cpp` and `.h` files containing the C++ source code, with one `.cpp`/`.h` pair for each Sudoh source
    file (if the specified Sudoh source file `includes` any additional files, those will be transpiled as well).

4.  Compile the C++ source files generated and link with `libsudoh.a`. This is an example using `g++`.
    ```
    $ g++ <cpp_source_files> -o <output_executable> -Isudoh -L. -lsudoh
    ```

You have now successfully compiled a Sudoh program! Have fun programming in Sudoh!
