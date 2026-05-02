test: main.cpp
	g++ $^ -o $@ -std=c++17

proactor_test: proactor_main.cpp
	g++ $^ -o $@ -std=c++17 -luring -lpthread

.PHONY: clean
clean:
	rm -f test proactor_test