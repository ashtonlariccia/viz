# vizc: the viz compiler (iteration 0, emits C)
#
#   make            build ./vizc
#   make test       build, then run every test in tests/m*/
#   make test M=m1  run only one milestone's tests
#   make run F=x.vz compile + run one file, print its exit code
#   make emit F=x.vz same, but also print the C that vizc generated
#   make asan       rebuild with AddressSanitizer + UBSan (catches memory bugs)
#   make clean      remove build output

CC      ?= cc
CFLAGS  ?= -std=c11 -Wall -Wextra -Wpedantic -Wshadow -g
SRC     := $(wildcard src/*.c)
OBJ     := $(SRC:src/%.c=build/%.o)
BIN     := vizc

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

build/%.o: src/%.c | build
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

build:
	@mkdir -p build

# auto-generated header dependencies: editing a .h rebuilds the .c files that include it
-include $(OBJ:.o=.d)

test: $(BIN)
	@./run_tests.sh $(if $(M),tests/$(M))

run: $(BIN)
	@./scripts/vzrun.sh $(F)

emit: $(BIN)
	@./scripts/vzrun.sh -c $(F)

asan:
	@$(MAKE) --no-print-directory clean
	@$(MAKE) --no-print-directory CFLAGS="$(CFLAGS) -fsanitize=address,undefined -fno-omit-frame-pointer"

clean:
	rm -rf build $(BIN) tests/out

.PHONY: all test run emit asan clean
