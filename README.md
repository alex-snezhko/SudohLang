# Sudoh
Sudoh is a simple programming language that can be executed by first being transpiled down to C++ and then compiled and run from
there on. Its easily understood syntax along with C++'s efficient nature make it well equipped for the task of software prototyping,
even for more computationally intensive tasks.

Sudoh's syntax and many of its features were inspired in large part by common motifs in algorithm pseudocode
examples (at least the ones that I've seen), as well as by languages such as Python and JavaScript. The output C++ code that the
Sudoh compiler `sudohc` generates is structured in a way to be very similar to the input Sudoh code, which allows making modifications
to the C++ code to actually be run very simple.

Documentation for the Sudoh language can be found in **documentation.md**
