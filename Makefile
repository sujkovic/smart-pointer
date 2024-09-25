all: assignment3

assignment3: SharedPtr_test.o
	g++ -o assignment3 SharedPtr_test.o

SharedPtr_test.o:
	g++ -c -g -ldl -pthread SharedPtr_test.cpp

clean:
	rm -f assignment3 *.o