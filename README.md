# Driver Manager Pro

Cross-platform driver management utility for Windows and Linux.
Provides a graphical interface for viewing, installing, and removing device drivers without using system built-in tools.

---

## About

Driver Manager Pro consists of two components:

- **Java application** — GUI built with JavaFX, handles driver downloads from a remote server and displays currently installed drivers
- **Native daemon** — C/C++ process running with elevated privileges, performs actual driver installation and removal via WinAPI (Windows) or libudev/sysfs (Linux)

Communication between the two components happens over a local socket (IPC).

---

## Requirements

|            | Windows         | Linux        |
| ---------- | --------------- | ------------ |
| Runtime    | Java 21         | Java 21      |
| Privileges | Administrator   | sudo         |
| OS         | Windows 10 / 11 | Kernel 5.15+ |

---

## Installation

### Windows

1. Download the latest release from [Releases](../../releases)
2. Run the installer as Administrator
3. Launch **Driver Manager Pro** from the Start Menu

### Linux

```bash
# Debian / Ubuntu
sudo dpkg -i driver-manager-pro.deb

# Arch Linux
sudo pacman -U driver-manager-pro.pkg.tar.zst

# Start the daemon and application
sudo driver-manager-daemon &
driver-manager-pro
```

---

## Manual Build

### Prerequisites

**Windows:**

| Tool          | Version                              | Purpose                     |
| ------------- | ------------------------------------ | --------------------------- |
| JDK 21        | [jdk21-openjdk](https://openjdk.org) | Compile and run Java        |
| JavaFX 21     | [javafx-sdk-21](https://openjfx.io)  | GUI framework               |
| Maven         | 3.9+                                 | Build orchestration         |
| CMake         | 3.25+                                | Native build system         |
| Visual Studio | 2022 with C++ workload               | MSVC compiler (C11 / C++23) |
| Windows SDK   | 10.0+                                | WinAPI, SetupAPI, CfgMgr32  |

**Linux — Arch:**

| Tool       | Package                | Purpose                              |
| ---------- | ---------------------- | ------------------------------------ |
| JDK 21     | `jdk21-openjdk`        | Compile and run Java                 |
| JavaFX 21  | `java21-openjfx` (AUR) | GUI framework                        |
| Maven      | `maven`                | Build orchestration                  |
| CMake      | `cmake`                | Native build system                  |
| Clang      | `clang` 16+            | C11 / C++23 compiler                 |
| libudev    | `libsystemd`           | Device enumeration + headers         |
| pkg-config | `pkgconf`              | Locate native libraries during build |

**Linux — Debian / Ubuntu:**

| Tool       | Package          | Purpose                              |
| ---------- | ---------------- | ------------------------------------ |
| JDK 21     | `openjdk-21-jdk` | Compile and run Java                 |
| JavaFX 21  | `openjfx`        | GUI framework                        |
| Maven      | `maven`          | Build orchestration                  |
| CMake      | `cmake`          | Native build system                  |
| Clang      | `clang` 16+      | C11 / C++23 compiler                 |
| libudev    | `libudev-dev`    | Device enumeration + headers         |
| pkg-config | `pkg-config`     | Locate native libraries during build |

> **Arch Linux**: After installing `jdk21-openjdk`, set it as the default:
>
> ```bash
> sudo archlinux-java set java-21-openjdk
> ```
>
> Arch does not use `JAVA_HOME` by default. If Maven explicitly requires it:
>
> ```bash
> export JAVA_HOME=/usr/lib/jvm/default
> ```

---

### 1. Clone the repository

```bash
git clone https://github.com/yourname/driver-manager-pro.git
cd driver-manager-pro
```

### 2. Build

The entire project is built with a single command from the root directory.
Maven orchestrates the pipeline automatically:

1. **native/deps** — native dependencies compiled via CMake (cmake-maven-plugin)
2. **native** — C/C++ daemon compiled via CMake against built dependencies
3. **java** — JavaFX application compiled by Maven, packaged with the daemon

**Windows:**

```cmd
mvn clean install -Pwindows
```

**Linux:**

```bash
mvn clean install -Plinux
```

The `-P` flag activates the platform profile which passes the correct compiler and generator to CMake (`Visual Studio 17 2022` on Windows, `Clang` on Linux).

### 3. Run

**Windows** (as Administrator):

```cmd
java -jar target\driver-manager-pro.jar
```

**Linux:**

- (as Administrator):

```cmd
java -jar target\driver-manager-pro.jar
```

**Linux:**

```bash
sudo java -jar target/driver-manager-pro.jar
```

---

## Tech Stack

- **Java 21** · JavaFX 21 · JNA 5
- **C11 / C++23** · Clang · MSVC
- **Windows:** WinAPI, SetupAPI, CfgMgr32
- **Linux:** libudev, sysfs, procfs, ioctl
- **IPC:** Local Sockets (Unix Socket / Named Pipe)

---

## License

MIT
