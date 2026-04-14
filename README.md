# Mint interpreter

Mint is an interpreted scripting language.

## Build Instructions

On Linux:

```shell
cmake --preset=vcpkg
cmake --build build
sudo cmake --install build
```

This will install mint as `/bin/mint`.

To build mint in release mode use `cmake --preset=vcpkg -DCMAKE_BUILD_TYPE=Release`.
For more details about CMake see [`CMake documentation`](https://cmake.org/).

On Windows:

```bat
cmake --preset=vcpkg
cmake --build build
cmake --install build
```

This will install mint as `C:\mint\bin\mint.exe`.

To build mint in release mode use `cmake --preset=vcpkg -DCMAKE_BUILD_TYPE=Release`.
For more details about CMake see [`CMake documentation`](https://cmake.org/).

## First steps

To create a "hello world" script create a new file named ``helloworld.mn``.
Open it and write the following lines:

```mn
#!/bin/mint

print('hello world !\n')
```

You can then run `mint helloworld.mn`.

## What Mint looks like

Here is a commented example that covers a few more core language features:

```mn
#!/bin/mint

// Enums are simple named constants.
enum Tone {
    Friendly
    Formal
}

// Define a class with one private member and a constructor.
class Greeter {
    def new(self, prefix) {
        self.prefix = prefix
        return self
    }

    // Methods receive the instance as their first parameter.
    // Parameters can also have default values.
    def greet(self, name, tone = Tone.Friendly) {
        return switch tone {
        case is Tone.Friendly => '%s, %s!' % (self.prefix, name)
        case is Tone.Formal => 'Greetings, %s.' % name
        }
    }

    // `-` makes the member private.
    - prefix = ''
}

// Functions are introduced with `def`.
// Using `yield` turns this into a generator function.
def makeNames(prefix, count) {
    // `1..count` is a range including `count`.
    for let i in 1..count {
        yield '%s #%d' % (prefix, i)
    }
}

// Create an object by calling the class like a function.
let greeter = Greeter('Hello')

// Hashes store key/value pairs.
let stats = {
    'printed' : 0,
    'skipped' : 0
}

// Generator functions return iterators, so they work naturally with `for`.
for let name in makeNames('Mint', 5) {
    // `in` can be used for containment checks.
    if '3' in name {
        stats['skipped'] += 1
        continue
    }

    // Methods are called with `.`, and strings use `%` formatting.
    let tone = name.endsWith('5') ? Tone.Formal : Tone.Friendly
    print(greeter.greet(name, tone) + '\n')
    stats['printed'] += 1
}

// A `for` loop can destructure iterator values into multiple variables.
for let (key, value) in stats {
    print('%s: %d\n' % (key, value))
}
```

More informations can be found in the [wiki](https://github.com/Palamecia/mint/wiki) section.

## IDE integration

This repo provides packages to enable syntax highlighting and other features for several IDE.

### Sublime Text

This package provides:

* Syntax highlighting

#### Linux

To install run the following command:

```shell
cp -r ./share/subl ~/.config/sublime-text/Packages/Mint
```

#### Windows

To install run the following command:

```bat
copy .\share\subl -destination "~\AppData\Roaming\Sublime Text\Packages\Mint" -recurse
```

### Visual Studio Code

This package provides:

* Syntax highlighting

#### Linux

To install run the following command:

```shell
cp -r ./share/vscode ~/.vscode/extensions/mint
```

#### Windows

To install run the following command:

```bat
copy .\share\vscode -destination ~\.vscode\extensions\mint -recurse
```

### Qt Creator / Kate

This package provides:

* Syntax highlighting

#### Linux

To install for Qt Creator run the following command:

```shell
cp -r ./share/kate/* ~/.config/QtProject/qtcreator/generic-highlighter/syntax
```

To install for Kate run the following command:

```shell
cp -r ./share/kate/* ~/.local/share/org.kde.syntax-highlighting/syntax
```

#### Windows

To install for Qt Creator run the following command:

```bat
copy .\share\kate\* -destination ~\AppData\Roaming\QtProject\qtcreator\generic-highlighter\syntax -recurse
```

To install for Kate run the following command:

```bat
copy .\share\kate\* -destination ~\AppData\Local\org.kde.syntax-highlighting\syntax -recurse
```
