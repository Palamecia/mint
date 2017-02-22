Mint interpreter
================

Mint is an interpreted scripting language.

Build Instructions
------------------

On Linux :

    mkdir build
    cd build
    cmake ..
    make
    sudo make install
    
This will install mint as ``/bin/mint``.

To build mint in debug mode use ``cmake .. -DCMAKE_BUILD_TYPE=Debug``.
For more details about CMake see `CMake documentation`_.

.. _CMake documentation: https://cmake.org/

On Windows :

The Windows target is not ready yet.

First steps
-----------

To create a "hello world" script create a new file named ``helloworld.mn``.
Open it and write the following lines :

    #!/bin/mint
    
    print {
        'hello world !\n'
    }
    
You can then run ``mint helloworld.mn``.
