## hand2
> hand2 is modified from [invisible_hand](https://github.com/ianchen-tw/invisible-hand),
> and is intended to integrate the functionality of invisible_hand and other tools into a single program.

hand2 helps you manage your classroom under GitHub organization, it utilizes `Google Sheets` and `GitHub`.
The program depends on libcurl to invoke RESTful API, and use libgit2 to perform git operation.

1. Use `hand --help` to see the usage of hand2.
2. `hand` will create a template config and exit when no existing config can be found.

## Download
x64 prebuilt binaries can be downloaded from [release](https://github.com/Compiler-s24/hand2/releases).

### Build

#### Windows
1. Run `build.bat` under `code\` to build the program on Windows. In default, it tries to build with MSVC, GCC, and clang.
2. To build with MSVC, you must run `build.bat` under MSVC x64 native tools command prompt. Note that it is required to call `vcvarsall.bat x64` to setup the developer prompt to x64 mode.\
   (See also [Visual Studio Developer Command Prompt and Developer PowerShell](https://learn.microsoft.com/en-us/visualstudio/ide/reference/command-prompt-powershell) and [How to: Enable a 64-Bit, x64 hosted MSVC toolset on the command line](https://learn.microsoft.com/en-us/cpp/build/how-to-enable-a-64-bit-visual-cpp-toolset-on-the-command-line).)
3. To build with GCC or clang, you must add GCC or clang to `PATH` enviroment variable.
4. You can find the built program under `build\`

#### Linux
> [!note]
> A docker enviroment is provided under `dependency_linux/`, see how to use it under [Build Dependencies - Linux](#Linux-1)

1. Run `build.sh` under `code/` to build the program on Linux, In default, it tries to build with GCC and clang.
2. To build with GCC or clang, you must add GCC or clang to `PATH` enviroment variable.
3. You can find the built program under `build/`

## Build Dependencies

#### Windows
1. Run `dependency_win32\lib_build.bat` to automatically build and install the required dependencies on Windows, you must run under MSVC x64 native tools command prompt.
2. To automatically download and extract the dependencies, `curl` and `tar` are required in your `PATH`. However, you can also download the dependencies manually, the url and the required directory structure will be provided by `lib_build.bat`.
3. The include files and the libraries are output to `code\include_win32` and `code\lib_win32` respectively.
4. The folder `dependency_win32\deps` is used to store the source files and build files for the dependencies. It's quite large, you can safely delete it after the build.
5. The folder `dependency_win32\log` is used to store the log during build. To see how the logs are generated, look into `dependency_win32\lib_build.bat`.

#### Linux
1. The linux dependencies are built with Docker, run `dependency_linux/docker_build.sh` to build and store the dependencies in a docker image.
2. Run `dependency_linux/docker_output_library.sh` to copy the include files and the libraries from docker image to `code/include_linux` and `code/lib_linux` respectively.
3. The folder `dependency_linux/log` is used to store the log during build. To see how the logs are generated, look into `dependency_linux/Dockerfile`.
4. To build hand2 inside the docker image, run `dependency_linux/hand_build.sh`.
5. To debug hand2 with GDB inside the docker image, run `dependency_linux/hand_debug.sh`.
6. If you do not intend to use the docker image to build or debug the program, you can safely delete the docker image `hand2_linux_image` after the build.

## Getting Started

#### Create config file
1. Run `hand` without an existing config file will automatically create a template config file at 'config.txt'.
2. Follow the comments in `config.txt` to configure the program.

## Features and TODOs
❌: TODO, ⚠️: Implemented, ✔: Tested

- ❌ config-check: check if the config is filled in correctly
- ✔ invite-students: invite students to organization.
    - TODO: Get the student list from the spreadsheet, currently it needs to be copied and sent to program as a file.
- ⚠️ collect-homework: output late submissions.
    - TODO: Get the student list from the spreadsheet, and output in a spreadsheet-friendly format (direct copy-pasta).
- ❌ grade-homework: replacing HW-manager, grade students' homework
    - TODO: Get the student list from the spreadsheet, and output in a spreadsheet-friendly format (direct copy-pasta).
- ⚠️ announce-grade: retrieve grade from the spreadsheet, and announce it in students' issue.
    - TODO: Support HW-manager format
    - TODO: Provide detailed description on the feedback format (or use old HW-manager format)
- ⚠️ patch-homework: create pull request to sync repo's default branch to a branch of tmpl-hw
    - TODO: Does Fork situation change anything?
- TODOs
    - More log!
    - Memory leak: now they are all over the place!
    - Update certificate: how are certificates outdated? try to find it in current system? tell user to download a new one?
    - Support ARM
    - Support mac-os (maybe never)
    - Support x86 (maybe never)





    