CFLAGS = -std=gnu99 -Wall -Wextra -Werror -pedantic
LFLAGS = -lrt -lpthread

.PHONY : proj2 clean
all : clean proj2

debug : CFLAGS += -g
debug : all
    
proj2 : proj2.o
	gcc $(CFLAGS) -o $@ $< $(LFLAGS)

clean :
	rm -f proj2 proj2.o proj2.out
	
%.o : %.c
	gcc $(CFLAGS) -o $@ -c $<
