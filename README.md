## How to build?

### Step 0: Download CMake

This can be done using 
Mac command
```
brew install cmake
```

Windows go to that link and follow the steps

https://cmake.org/download/

download the source distribution and then use the default install settings.

### Step 0.5: Install Vulkan SDK

Download the Vulkan SDK from here: https://vulkan.lunarg.com/sdk/home

After installing, you need to enable the environment variables so CMake and the application can find the Vulkan headers and libraries. 

(For Mac users) Add the following to your shell configuration (e.g., `~/.zshrc` or `~/.bashrc`), replacing the path with your actual installation path and version:

```sh
source ".../VulkanSDK/<VulkanSDK-version>/setup-env.sh"
```

### Step 1: Download and set up vcpkg

Download vcpkg anywhere on your computer (not in our repo):
```git clone https://github.com/microsoft/vcpkg.git```

Then run the bootstrap script
```cd vcpkg && ./bootstrap-vcpkg.sh```

Then setup the following variables
```
export VCPKG_ROOT=/path/to/vcpkg
export PATH=$VCPKG_ROOT:$PATH
```
I would suggest you try to make the above variables persistent, 
but you might possibly run into issues with VS Code CMake extension not recognizing these.

### Step 2: Clone the repo

git clone https://github.com/P0k3rf4ce/SauceEngine.git

### Step 3: Set up a CMakeUserPresets.json file

In the root directory of the repo, create a file names `CMakeUserPresets.json`.
This file should have the following content:
```
{
    "version": 2,
    "configurePresets": [{
        "name": "default",
        "generator": "Ninja",
        "inherits": "vcpkg",
        "environment": {
            "VCPKG_ROOT": "<ENTER PATH TO VCPKG HERE>"
        }
    }]
}
```
where you must replace the path to vcpkg obviously.
The existing `CMakePresets.json` file uses the environment setup by `CMakeUserPresets.json`.


Note that you might need to change up the generator that you use, this is what Nimish did which worked for him. The generator was changed from Ninja for him to Unix makefiles 

CMakePresets file 
```
{
  "version": 2,
  "configurePresets": [
    {
      "name": "vcpkg",
      "binaryDir": "${sourceDir}/build",
      "cacheVariables": {
        "CMAKE_TOOLCHAIN_FILE": "$env{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
      }
    }
  ]
}

```

CMakeUserPresets file
```
{
    "version": 2,
    "configurePresets": [{
        "name": "default",
        "generator": "Unix Makefiles",
        "inherits": "vcpkg",
        "environment": {
            "VCPKG_ROOT": "<PATH TO vcpkg>"
        }
    }]
}

```
### Step 4: Configure, Build, and Run!

I highly recommend using the VS Code CMake extension to do this.
Otherwise, I suggest looking online on how to compile with CMake.
Note that the current setup uses the `default` preset 
(i.e. you'll need to configure with `--preset default` or something).

The compiled executable will be in the `build` directory.

### References
- https://learn.microsoft.com/en-us/vcpkg/get_started/get-started?pivots=shell-bash
