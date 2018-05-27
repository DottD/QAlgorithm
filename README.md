# QAlgorithm

QAlgorithm is an abstract class the contains the logic for a generic algorithm written in C++ and using Qt Libraries. Using this library you will be able to quickly write algorithms, with highly readable structure; furthermore the algorithms can be combined together to form a tree that can run in parallel, using Qt Concurrent engine.

## Getting Started

Please try to follow the example provided in the *Examples* folder and the Doxygen documentation available at https://dottd.github.io/QAlgorithm/

### Prerequisites

Before building QAlgorithm you need to install the following:
- [CMake](https://cmake.org)
- [Qt Core](https://www.qt.io)

### Installing

Use the typical building step for CMake projects:
- create a *build-folder*
- use CMake GUI to configure the project
- open the terminal
	- cd *build-folder*
	- make
	- make install

### Validating the installation

Try to run one of the examples provided along with the code.

## Contributing

Any contribution to this library is welcome. It was part of a bigger project and I only developed the features I needed; hence there is a lot of work to do to improve it.

## Testing

No test was performed on the library, except for the use I did in my project. Some possible tests that need to be done are:
- performance test
- performance improvement of the *improveTree* method
- reliability test, checking if every algorithm runs properly and if every property is correctly passed to the connected algorithms

## Authors

* **Filippo Santarelli** - *Initial work*

## License

This project is licensed under the GNU Lesser General Public License v3.0 - see the [LICENSE.md](LICENSE.md) file for details
