# VSCode Quick Start Guide for SauceEngine

## 🎯 First Time Setup

### 1. Open the Project
```bash
cd /home/noah/Desktop/projects/SauceEngine
code .
```

### 2. Install Required VSCode Extensions
- **CMake Tools** (ms-vscode.cmake-tools)
- **C/C++** (ms-vscode.cpptools)

### 3. Select Configure Preset
1. Open Command Palette: `Ctrl+Shift+P`
2. Type: **CMake: Select Configure Preset**
3. Choose: `default`

### 4. Configure the Project
1. Command Palette (`Ctrl+Shift+P`)
2. Run: **CMake: Configure**
3. Wait for "Configuring done" message

### 5. Build
- Press `F7` OR
- Command Palette → **CMake: Build**

## ⌨️ Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| `Ctrl+Shift+P` | Open Command Palette |
| `F7` | Build project |
| `Shift+F7` | Clean rebuild |
| `Ctrl+F5` | Run without debugging |
| `F5` | Start debugging |
| `Ctrl+Shift+B` | Run build task |

## 🔄 Common Workflows

### Making Code Changes
1. Edit your code
2. Press `F7` to build
3. Check "Output" panel for build results
4. Fix any errors and repeat

### Switching Build Types
1. Command Palette → **CMake: Select Configure Preset**
2. Choose: `default` (Debug) or `release` (Release)
3. Command Palette → **CMake: Delete Cache and Reconfigure**
4. Build with `F7`

### Cleaning Build
1. Command Palette → **CMake: Clean**
2. Or manually: `rm -rf build && cmake --preset=default`

### Viewing Build Output
- **Output Panel** (bottom): Click "CMake/Build" dropdown
- Or `Ctrl+Shift+U` to open Output panel

## 🐛 If Things Go Wrong

### "CMake Tools is not configured"
1. Delete `build` directory: `rm -rf build`
2. Close and reopen VSCode
3. Select configure preset: `default`
4. Run CMake: Configure

### "No build configuration selected"
1. Look at bottom status bar
2. Click on the build configuration selector
3. Choose `default` or another preset

### "IntelliSense errors"
1. Make sure project is configured (`F7` builds will trigger this)
2. Check that `build/compile_commands.json` exists
3. Reload window: Command Palette → **Developer: Reload Window**

### Build fails with "Ninja/Makefiles error"
1. Delete build folder: `rm -rf build`
2. Reconfigure: Command Palette → **CMake: Configure**
3. Build: `F7`

## 📍 VSCode Status Bar

At the bottom of VSCode, you'll see:

```
[CMake Icon] [default] [Build: Debug] [Target: all] [Launch: SauceEngine]
```

Click each to:
- **Configure Preset**: Switch between default/release/makefiles
- **Build Type**: View current build configuration
- **Target**: Select what to build (usually "all")
- **Launch Target**: Select which executable to run

## 🎨 Customizing VSCode

### Settings Location
`.vscode/settings.json` - Project-specific settings

### CMake Presets
`CMakeUserPresets.json` - Add custom build configurations

Example: Add a RelWithDebInfo preset:
```json
{
  "name": "relwithdebinfo",
  "generator": "Ninja",
  "cacheVariables": {
    "CMAKE_TOOLCHAIN_FILE": "/home/noah/vcpkg/scripts/buildsystems/vcpkg.cmake",
    "CMAKE_BUILD_TYPE": "RelWithDebInfo"
  }
}
```

## 📊 Build Progress

Watch the bottom status bar for:
- 🔨 **Configuring**: CMake is generating build files
- ⚙️ **Building**: Compiling your code
- ✅ **Build finished**: Success!
- ❌ **Build failed**: Check Output panel

## 🚀 Pro Tips

1. **Fast builds**: Ninja automatically uses all CPU cores
2. **IntelliSense**: Wait for initial configure to complete for best experience
3. **Errors**: Click on errors in Problems panel to jump to code
4. **Terminal**: Use integrated terminal (`Ctrl+``) for manual commands
5. **Git**: Built-in source control in left sidebar

## 📖 CMake Commands (via Command Palette)

- **CMake: Select Configure Preset** - Choose build configuration
- **CMake: Configure** - Generate build files
- **CMake: Build** - Compile project (F7)
- **CMake: Clean** - Remove build artifacts
- **CMake: Delete Cache and Reconfigure** - Fresh start
- **CMake: Select Launch Target** - Choose executable to run
- **CMake: Debug** - Start debugging session

## ✨ Next Steps

1. Try building: Press `F7`
2. Check build output in Output panel
3. Run the application: `./build/SauceEngine`
4. Make changes and rebuild automatically

Happy coding! 🎉
