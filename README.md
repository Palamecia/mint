# Mint interpreter

Mint is an interpreted scripting language.

## Build Instructions

On Linux :
```shell
mkdir build
cd build
cmake ..
make
sudo make install
```

This will install mint as `/bin/mint`.

To build mint in release mode use `cmake -DCMAKE_BUILD_TYPE=Release ..`.
For more details about CMake see [`CMake documentation`](https://cmake.org/).

On Windows :
```bat
mkdir build
cd build
cmake ..
cmake --build . --target ALL_BUILD
cmake --build . --target INSTALL
```

This will install mint as `C:\mint\bin\mint.exe`.

To build mint in release mode use `cmake --build . --target ALL_BUILD --config Release` and `cmake --build . --target INSTALL --config Release`.

## First steps

To create a "hello world" script create a new file named ``helloworld.mn``.
Open it and write the following lines :
```mint
#!/bin/mint

print {
    'hello world !\n'
}
```

You can then run `mint helloworld.mn`.

More informations can be found in the [wiki](https://github.com/Palamecia/mint/wiki) section.

## IDE integration

This repo provides packages to enable syntax highlighting and other features for several IDE.

### Sublime Text

This package provides :
* Syntax highlighting

#### Linux

To install run the following command :

```shell
cp -r ./share/subl ~/.config/sublime-text/Packages/Mint
```

#### Windows

To install run the following command :

```bat
copy .\share\subl -destination "~\AppData\Roaming\Sublime Text\Packages\Mint" -recurse
```

### Visual Studio Code

This package provides :
* Syntax highlighting

#### Linux

To install run the following command :

```shell
cp -r ./share/vscode ~/.vscode/extensions/mint
```

#### Windows

To install run the following command :

```bat
copy .\share\vscode -destination ~\.vscode\extensions\mint -recurse
```

### Qt Creator / Kate

This package provides :
* Syntax highlighting

#### Linux

To install for Qt Creator run the following command :

```shell
cp -r ./share/kate/* ~/.config/QtProject/qtcreator/generic-highlighter/syntax
```

To install for Kate run the following command :

```shell
cp -r ./share/kate/* ~/.local/share/org.kde.syntax-highlighting/syntax
```

#### Windows

To install for Qt Creator run the following command :

```bat
copy .\share\kate\* -destination ~\AppData\Roaming\QtProject\qtcreator\generic-highlighter\syntax -recurse
```

To install for Kate run the following command :

```bat
copy .\share\kate\* -destination ~\AppData\Local\org.kde.syntax-highlighting\syntax -recurse
```
