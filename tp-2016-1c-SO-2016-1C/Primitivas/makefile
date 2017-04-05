RM=rm -rf
CC=gcc

C_SRCS=$(shell find . -iname "*.c" | tr '\n' ' ')
OBJS=$(C_SRCS:./%.c=build/%.o)

all: build/Primitivas

create-dirs:
	mkdir -p build/.

build/Primitivas: dependents create-dirs $(OBJS)
	$(CC) -Wall -L"../parser/build" -o "build/Primitivas" $(OBJS) -lparser-ansisop -lcommons

build/%.o: ./%.c
	$(CC) -I"../parser" -c -fmessage-length=0 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"

debug: CC += -DDEBUG -g
debug: all

clean:
	$(RM) build

dependents:
	-cd ../parser/ && $(MAKE) all

.PHONY: all create-dirs clean
