include Make.defaults

all : cooper.a test

cooper.a : init.o rsp.o
	$(PRE)$(AR) cr $@ $^

test : test.c cooper.a
	$(PRE)$(CC) -Wall -Werror -o $@ $^ cooper.a

%.o : %.c
	$(PRE)$(CC) $(FLAGS) -ggdb -c -o $@ $^

.PHONY: clean all
clean :
	@rm -vf *.o *.a *.E test
