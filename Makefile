
# CXX to simplify compilation from makefile
CXX = g++

# target all means all targets currently defined in this file 
all: CyGMM 

# target dependencies for main program - also equally messy code inside!
CyGMM: nonlinear.o fileIO.o system.o calc.o cli.o linear.o
	g++ nonlinear.o fileIO.o system.o calc.o cli.o linear.o -o CyGMM -fopenmp -lstdc++fs
nonlinear.o: nonlinear.cpp nonlinear.hpp main.hpp fileIO.hpp system.hpp calc.hpp cli.hpp 
	g++ -c -O3 -fopenmp nonlinear.cpp 
fileIO.o: fileIO.cpp main.hpp fileIO.hpp
	g++ -c fileIO.cpp
system.o: system.cpp main.hpp system.hpp
	g++ -c system.cpp
calc.o: calc.cpp main.hpp calc.hpp
	g++ -c calc.cpp
cli.o: cli.hpp main.hpp
	g++ -c cli.cpp
linear.o: linear.hpp main.hpp 
	g++ -c linear.cpp
# this target deletes all files produced from the Makefile
# so that a completely new compile of all items is required
clean:
	rm -rf *.o CyGMM
