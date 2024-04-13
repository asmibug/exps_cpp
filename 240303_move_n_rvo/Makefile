CXXFLAGS := -std=c++20 -Wpedantic -g3 \
-Wall -Wextra \
-fsanitize=address,undefined

default: main.exe

test: main.exe
	./$<

main.exe: main.o
	$(CXX) $(CXXFLAGS) $^ -o $@

clean:
	-rm *.o *.exe

.PHONY: test clean
.DELETE_ON_ERROR:
