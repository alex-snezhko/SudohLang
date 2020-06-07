# make static library
g++ -c sudoh/variable.cpp sudoh/sudoh.cpp sudoh/runtime_ex.cpp
ar rcs libsudoh.a variable.o sudoh.o runtime_ex.o
rm -f variable.o sudoh.o runtime_ex.o

g++ -I ./transpiler/ transpiler/sudoht.cpp transpiler/name_manager.cpp transpiler/parser.cpp transpiler/token_iter.cpp transpiler/transpiled_buf.cpp -o sudoht
