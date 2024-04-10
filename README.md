## hand2
> [!note]
> The docs is not up-to-date currently.

> hand2 is modified from [invisible_hand](https://github.com/ianchen-tw/invisible-hand),
> and is intended to integrate the functionality of invisible_hand and other tools into a single program.

hand2 helps you manage your classroom under GitHub organization, it utilizes `Google Sheets` and `GitHub`.
The program depends on libcurl to invoke RESTful API, and use libgit2 to perform git operation.

1. Use `hand --help` to see the usage of hand2.
2. `hand` will create a template config and exit when no existing config can be found.

## Download
x64 prebuilt binaries can be downloaded from [release](https://github.com/Compiler-s24/hand2/releases).

## Build

#### Windows
1. Run `build.bat` under `code\` to build the program on Windows. In default, it tries to build with MSVC, GCC, and clang.
2. To build with MSVC, you must run `build.bat` under MSVC x64 native tools command prompt. Note that it is required to call `vcvarsall.bat x64` to setup the developer prompt to x64 mode.\
   (See also [Visual Studio Developer Command Prompt and Developer PowerShell](https://learn.microsoft.com/en-us/visualstudio/ide/reference/command-prompt-powershell) and [How to: Enable a 64-Bit, x64 hosted MSVC toolset on the command line](https://learn.microsoft.com/en-us/cpp/build/how-to-enable-a-64-bit-visual-cpp-toolset-on-the-command-line).)
3. To build with GCC or clang, you must add GCC or clang to `PATH` enviroment variable.
4. You can find the built program under `build\`

#### Linux
1. Run `./build.sh` under `code/` to build the program on Linux, In default, it tries to build with GCC and clang.
2. To build with GCC or clang, you must add GCC or clang to `PATH` enviroment variable.
3. You can find the built program under `build/`

#### Linux Docker
1. Run `docker/build.sh` to build the program inside docker, the program is built with GCC and clang.
2. You can find the built program under `build/`
3. Run `docker/run.sh [args] ...` to execute the program under the docker.
4. To debug hand2 with GDB inside the docker image, run `docker/debug.sh [args] ...`.
5. Any calls to `build.sh`, `run.sh` or `debug.sh` will automatically build the docker image.

## Build Dependencies

#### Windows
1. Run `win32_external\build.bat` to automatically build and install the required dependencies on Windows, you must run under MSVC x64 native tools command prompt.
2. To automatically download and extract the dependencies, `curl` and `tar` are required in your `PATH`. However, you can also download the dependencies manually, the url and the required directory structure will be provided by `build.bat`.
3. The include files and the libraries are output to `code\include_win32` and `code\lib_win32` respectively.
4. The folder `win32_external\deps` is used to store the source files and build files for the dependencies. It's quite large, you can safely delete it after the build.
5. The folder `win32_external\log` is used to store the log during build. To see how the logs are generated, look into `win32_external\build.bat`.

#### Linux
1. The linux dependencies are built with Docker, run `linux_external/build.sh` to build the docker image, and copy the include files and the libraries to `code/include_linux` and `code/lib_linux`, respectively.
2. The docker image will be automatically removed after the build.
3. The folder `linux_external/log` is used to store the log during build. To see how the logs are generated, look into `linux_external/Dockerfile`.

## Getting Started

#### Create config file
1. Run `hand` without an existing config file will automatically create a template config file at 'config.txt'.
2. Follow the comments in `config.txt` to configure the program.

## Features and TODOs
❌: TODO, ⚠️: Implemented, ✔: Tested

- ✔ clean: delete cache and log folder
- ✔ config-check: check if the config is filled in correctly
- ✔ invite-students: invite students to organization.
- ✔ collect-homework: output late submissions.
- ✔ grade-homework: replacing HW-manager, grade students' homework
- ✔ announce-grade: retrieve grade from the spreadsheet, and announce it in students' issue.
    - TODO: Provide detailed description on the feedback format (or use old HW-manager format)
- TODOs
    - More log!
    - format_string() panic on failed!
    - Write actual docs





    