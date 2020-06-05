CC = g++
CFLAGS = -Wall -g
OBJS = sudohc.o parser.o name_manager.o token_iter.o transpiled_buf.o

sudohc: $(OBJS)

sudohc.o: sudohc.cpp parser.h

parser.o: parser.cpp parser.h syntax_ex.h

name_manager.o: name_manager.cpp name_manager.h syntax_ex.h

token_iter.o: token_iter.cpp token_iter.h syntax_ex.h

transpiled_buf.o: transpiled_buf.cpp transpiled_buf.h

clean:
	rm -f $(OBJS)

