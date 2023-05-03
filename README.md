# CyGMM

CyGMM is now a defunct repository, everything (including the underlying method) has moved to a more mature software [BioNetGMMFit](https://github.com/jhnwu3/BioNetGMMFit)

However, we do have the original C++ CyGMM code in this repository. There is a linear.cpp file that contains a solver function for all linear systems of ODEs. Within the main.cpp, we have all functions necessary for nonlinear or linear systems of ODES for parameter estimation. However, one should note that if they care to use the code, while there exists a makefile, there are two main software libraries that one should install before using the software, namely boost and Eigen. To install those, please do.

    sudo apt install libeigen3-dev
    sudo apt-get install libboost-all-dev

Then, to compile, please

    make

To use the program, five distinct inputs are needed.

One must define the directory that contains the initial conditions X that will be evolved using 

    ./CyGMM -x <path_to_X_directory>

Next, one must define the directory that contains the observed conditions Y 

    ./CyGMM -y <path_to_y_directory>

Then, define your ODEs in system.cpp (matrix for system of linear equations) or system.hpp (explicit Boost system of ODES). 

    make

If linear and using matrix exponentiation, please note that one must use the "--linear" argument.

    ./CyGMM --linear

Once the system is defined, a set of time steps for evolution is required for running the last part of the program.

    ./CyGMM -t <time_steps_path>

<!-- If simulating rate constants, we have a "--simulate" keyword that notes if one is simulating data at a time point with the model, and thus one also needs to feed it in some explicit rate constants.

    ./CyGMM -r <rate_path> --simulate -->

Then, taking them together, we have.

    ./CyGMM -x <path_to_X_directory> -y <path_to_y_directory> -t <time_steps_path> --simulate

or

    ./CyGMM -x data/X -y data/Y -t data/time_steps.csv -r data/true_rates.csv --simulate

To modify the run tasks, please look in nonlinear.cpp and at the very first line. Note the nRates variable for when you want to write your own ODE system for estimation.