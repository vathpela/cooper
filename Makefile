include Make.defaults

all : cooper.a test

cooper.a : init.o rsp.o
	$(PRE)$(AR) cr $@ $^

test : test.c cooper.a
	$(PRE)$(CC) -ggdb -Wall -Werror -o $@ $^ cooper.a -lrt

%.o : %.c
	$(PRE)$(CC) $(FLAGS) -ggdb -c -o $@ $^

.PHONY: clean all
clean :
	@rm -vf *.o *.a *.E test
