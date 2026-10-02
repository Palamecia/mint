# 🍃 Mint Programming Language

**Mint** is a modern, interpreted scripting language engineered to be **highly expressive, lazy by design, and natively asynchronous**—all while maintaining a **familiar, readable, and proven syntax**.

Mint eliminates boilerplate code, allowing you to seamlessly transition from **rapid, friction-free prototyping** to building **production-grade, robust, and bulletproof APIs**.

---

## ✨ Key Features

### 📈 Scalable Architecture: Prototype to Production

Mint adapts to your workflow. You can start with zero-friction prototyping (implicit variables, dynamic typing) and progressively harden your codebase into a strict, secure API using `final`, `const`, and explicit variable bindings:

```mn
// 1. Rapid Prototyping Phase
class AnalyticsPipeline {
    dataset
    def process(self) {
        cleanData = self.dataset.clean()
        cleanData += 42
        return cleanData
    }
}

// 2. Production-Ready API Refinement
class AnalyticsPipeline {
    - final dataset = []
    final const def process(const self) {
        let cleanData = self.dataset.clean()
        cleanData += 42
        return cleanData
    }
}
```

### ⚡ Expressive Lazy Evaluation

Handle infinite data streams or transform complex datasets on the fly without heavy memory overhead. Mint's natural fusion of generators and the `yield` keyword turns pipeline manipulation into a single, elegant expression:

```mn
def streamSensorData(payloads) {
    yield for let payload in payloads => switch typeof payload {
        case 'telemetry' => sanitizeTelemetry(payload)
        case 'heartbeat' => processHeartbeat(payload)
        default          => handleUnknownPayload(payload)
    }
}
```

### 🌐 Fluent Native Asynchrony

Break free from promise chains and verbose asynchronous loops. Mint natively unifies `async/await` mechanics with generator functions to stream asynchronous operations effortlessly:

```mn
async def listenToStream(networkServer) {
    yield while let package = await networkServer.nextChunk() => if package.isSecure() => package.decrypt()
}
```

---

## 🚀 Language at a Glance

Here is a look at Mint's clean object-oriented design, pattern matching, and destructured looping syntax:

```mn
#!/bin/mint

enum LogLevel {
    Debug
    Critical
}

class SystemLogger {
    def new(self, environment) {
        self.environment = environment
        return self
    }

    def formatLog(self, message, level = LogLevel.Debug) {
        return switch level {
            case is LogLevel.Debug    => '[%s] 🔧 Debug: %s' % (self.environment, message)
            case is LogLevel.Critical => '[%s] 🚨 CRITICAL: %s' % (self.environment, message)
        }
    }

    - environment = '' // Private class member
}

// Lazy generator producing dynamic strings
def generateMockLogs(prefix, count) {
    for let i in 1..count {
        yield '%s event sequential ID: %d' % (prefix, i)
    }
}

let logger = SystemLogger('Production')
let metrics = { 'processed': 0, 'dropped': 0 }

for let logMessage in generateMockLogs('CoreEngine', 5) {
    if 'ID: 3' in logMessage {
        metrics['dropped'] += 1
        continue
    }

    let severity = logMessage.endsWith('5') ? LogLevel.Critical : LogLevel.Debug
    print(logger.formatLog(logMessage, severity) + '\n')
    metrics['processed'] += 1
}

// Destructuring an iterator (Hash/Dictionary keys and values)
for let (metricName, countValue) in metrics {
    print('%s events: %d\n' % (metricName, countValue))
}
```

---

## 📦 Compilation & Installation

### 🐧 Linux

```shell
cmake --preset=vcpkg
cmake --build build
sudo cmake --install build
```

*Installs the executable as `/bin/mint`*.

### 🪟 Windows

```bat
cmake --preset=vcpkg
cmake --build build
cmake --install build
```

*Installs the executable as `C:\mint\bin\mint.exe`*.

💡 *To build Mint in release mode, append `-DCMAKE_BUILD_TYPE=Release` to the initial setup command.*

---
## 💻 IDE Integration

Official packages are provided within this repository to add rich syntax highlighting for major editors. Expand the sections below to see the installation commands for your environment:

<details>
<summary><b>Visual Studio Code</b></summary>

#### Linux

```shell
cp -r ./share/vscode ~/.vscode/extensions/mint
```

#### Windows

```bat
copy .\share\vscode -destination ~\.vscode\extensions\mint -recurse
```

</details>

<details>
<summary><b>Sublime Text</b></summary>

#### Linux

```shell
cp -r ./share/subl ~/.config/sublime-text/Packages/Mint
```

#### Windows

```bat
copy .\share\subl -destination "~\AppData\Roaming\Sublime Text\Packages\Mint" -recurse
```

</details>

<details>
<summary><b>Qt Creator / Kate</b></summary>

#### Linux

* **Qt Creator:**

  ```shell
  cp -r ./share/kate/* ~/.config/QtProject/qtcreator/generic-highlighter/syntax
  ```

* **Kate:**

  ```shell
  cp -r ./share/kate/* ~/.local/share/org.kde.syntax-highlighting/syntax
  ```

#### Windows

* **Qt Creator:**

  ```bat
  copy .\share\kate\* -destination ~\AppData\Roaming\QtProject\qtcreator\generic-highlighter\syntax -recurse
  ```

* **Kate:**

  ```bat
  copy .\share\kate\* -destination ~\AppData\Local\org.kde.syntax-highlighting\syntax -recurse
  ```

</details>

---

*For deep structural deep-dives and full language reference guides, head over to the official [Mint Wiki](https://github.com/Palamecia/mint/wiki).*
