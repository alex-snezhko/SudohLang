# Sudoh
Sudoh is a simple programming language that transpiles down to C++ and then may be compiled and run from there on. Its
easily understandable syntax along with C++'s efficient nature make it well equipped for the task of software prototyping,
even for more computationally intensive tasks.

Sudoh can be described as a strongly-typed, dynamically type-checked, garbage-collected, procedural, high-level language.
Sudoh's syntax and many of its features were inspired in large part by common motifs that I've seen in algorithm pseudocode
examples, as well as by languages such as Python and JavaScript. The output C++ code that the Sudoh transpiler `sudohc`
generates is structured in a way to be very similar to the input Sudoh code, which allows making modifications to the actual
C++ code to be run quite simple.

Here is a simple program that prints the first `n` fibonacci numbers specified by standard input, written in Sudoh:
```
procedure fibonacci <- n
	num1 <- 0
	num2 <- 1
	for i <- 1 to n do
		printLine(n1)
		printLine(n2)

		temp <- num1
		num1 <- num2
		num2 <- num1 + num2

n <- number(input())
fibonacci(n)
```

Documentation for the Sudoh language can be found in **documentation.md**
