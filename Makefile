CC=gcc
CFLAGS=-Wall
SOURCES=us_numbers.c
EXECUTABLE=us_numbers
T_IN=$(wildcard *.test)
INPUT=$(T_IN:.test=.in)
TESTS=$(T_IN:.test=)

all: compile
compile: $(SOURCES)
	$(CC) $(CFLAGS) -o $(EXECUTABLE) $(SOURCES)
compile_debug: $(SOURCES)
	$(CC) $(CFLAGS) -g -o $(EXECUTABLE) $(SOURCES)
debug: compile_debug
	gdb $(EXECUTABLE)
test: compile $(TESTS) ;
$(TESTS): compile $(INPUT)
	$(EXECUTABLE) $@.in > $@.mout
	sort $@.in > $@.sout
	diff -q $@.mout $@.sout && echo "ok $@"
%.in: %.test
	chmod +x $<
	$< > $@
clean:
	rm -rf $(EXECUTABLE) $(INPUT) *.sout *.mout
