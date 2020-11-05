# Mint interpreter

Mint is an interpreted scripting language.

## Build Instructions

On Linux :

    mkdir build
    cd build
    cmake ..
    make
    sudo make install
    
This will install mint as ``/bin/mint``.

To build mint in release mode use ``cmake -DCMAKE_BUILD_TYPE=Release ..``.
For more details about CMake see [`CMake documentation`](https://cmake.org/).

On Windows :

    mkdir build
    cd build
    cmake ..
    cmake --build . --target ALL_BUILD
    cmake --build . --target INSTALL
    
This will install mint as ``C:\mint\bin\mint.exe``.

To build mint in release mode use ``cmake --build . --target ALL_BUILD --config Release`` and ``cmake --build . --target INSTALL --config Release``.

## First steps

To create a "hello world" script create a new file named ``helloworld.mn``.
Open it and write the following lines :

    #!/bin/mint
    
    print {
        'hello world !\n'
    }
    
You can then run ``mint helloworld.mn``.

More informations can be found in the [wiki](https://github.com/Palamecia/mint/wiki) section.
