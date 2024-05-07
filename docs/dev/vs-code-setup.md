# Setting up Visual Studio Code for development with the Cascoda SDK

This guide explains how to set up Visual Studio Code to work with the Cascoda SDK in a highly integrated way. This setup is recommended for developers working on the Cascoda SDK.

At the end of this guide, your VS Code Installation will support advanced IntelliSense features for hosted & embedded C/C++. You will be able to build the SDK from within VS Code, see warnings and errors within the IDE, right-click functions in source code and use 'Go to Definition' to find their implementation without searching by hand, automatically rename functions and variables across multiple files, and much more.

## Required Extensions

Please install the following extensions:

1. CMake Tools by Microsoft: This extension makes VS Code aware of the CMake build system used by the Cascoda SDK. It is used to trigger the CMake build, configure CMake Cache Variables and it supports multiple toolkits, letting you configure the SDK for different platforms (e.g. Posix, Chili2).
2. C/C++ by Microsoft: This extension adds IntelliSense support for the C & C++ languages. Once CMake Tools is configured correctly and you are building your project through VS Code, the IntelliSense features will work automatically.
3. Markdown All in One by Yu Zhang: Useful extension for working on documentation. The auto-generated Table of Contents is used in several places in the SDK, and the auto-complete is great for linking to images or other Markdown pages.
4. (Optional) Remote-SSH by Microsoft: If you would like to build on a remote build server, this extension lets VS Code run as a local client & communicate to a VS Code Server using SSH. This lets you have a Linux build environment while still running VS Code on a Windows machine.

## Extension Set-up

### Default build folder

After installing the extensions, open the Extensions panel on the left, press the gear icon next to the CMake Tools extension and select 'Extension Settings'. Find the 'Cmake: Build Directory' field and make sure that it is set to `${workspaceFolder}/build`.
- Note that this is different than the build directory recommended in the other Cascoda development guides. The other guides suggest creating different build folders for each platform you are working on: `sdk-chili2`, `sdk-posix` and so on. However, VS Code lets you change the configured platform of a build directory with just a few clicks, whereas changing the build directory is harder as you need to go through the extension settings. This configuration also enables VS Code IntelliSense when working on other CMake-based projects, such as the KNX-IoT Stack.
  - If you would still like to use the traditional directory layout, make sure 'Cmake: Build Directory' is set to `${workspaceFolder}/../sdk-chili2` or `${workspaceFolder}/../sdk-posix` as required.

### Preferred generators

In the Extension Settings for CMake Tools, navigate to Preferred Generators and click "Edit in settings.json". Here you can ensure that the Ninja build tool is used whenever it is available, by putting it first in the list (e.g. before MinGW Makefiles)
```json
    "cmake.preferredGenerators": [        
        "Ninja",
        "MinGW Makefiles",
        "Visual Studio 16 2019",
        "Unix Makefiles",
    ]
```

### Toolkit Set-up

In order to build for embedded platforms, we need to make sure VS Code is aware of what toolchain file we are using, and where it is located. This is managed within the Cmake Tools extension through the 'Kit' feature.

Press F1 and type 'Edit User-Local CMake Kits' and select the highlighted option. Add the following entry at the top of the JSON list:
```json
[
  {
    "name": "Chili 2",
    "toolchainFile": "toolchain/arm_gcc_m2351.cmake"
  },
  ...

```

## Using the integrated set-up

Navigate to your local clone of the Cascoda SDK and open the cascoda-sdk folder with VS Code. The first time you do this, you should see a prompt asking you to select a default kit. Please select the newly-created Chili 2 toolkit if you are building the SDK for embedded. If you are working on the POSIX platform, please select one of the GCC-based options with MinGW.
- If you want to build the KNX-IoT Stack using this set-up, you may select the "Unspecified" toolkit, which should lead to a Visual Studio C Compiler based build chain.

If you would like to change the toolkit afterwards, please navigate to the CMake panel by clicking on the CMake Tools icon on the left (CMake triangle with wrench). Hover over the options under the Configure branch and click the pen icon on the one that says "Change Kit". Within the CMake Tools panel you may also change the default build target, under the Build branch.

### Building & resolving errors

You may build your code by pressing the Build button in the bottom toolbar, or by pressing F7. The labels to the left of the Build button show the number of errors & warnings revealed by the build. Clicking the labels will open a full list of errors, which also lets you navigate to the location of the error within the codebase. Errors are also highlighted within currently open files using red squiggly underlines.

### Navigating the codebase and refactoring

Once IntelliSense is set up, hovering over a symbol will reveal its type definition or function signature. Right-clicking on a symbol (variable name, function name) reveals a number of options:
- "Go to definition" is perhaps the most useful, as it navigates to the file & line where said symbol was defined. This can also be accomplished by Control-clicking a symbol, or pressing F12 while hovering.
- "Find all References" reveals a list of all places in which the given symbol was used. Unlike the search bar, it is able to distinguish between valid uses in code & comments.
- "Rename Symbols" & "Change all Occurrences" are useful for refactoring, as they can be used to rename a function or structure member across the entire codebase.

### Modifying the CMake Cache from within VS Code

Strike the F1 key to open the command entry window, and type "Edit CMake Cache". The UI option opens a visual interface similar to cmake-gui that lives in VS Code, while the normal edit opens the CMakeCache.txt file for editing.
