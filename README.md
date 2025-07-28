# FileWatcher Module for JUCE

A JUCE user module that provides cross-platform file system watching capabilities by integrating the [efsw (Entropia File System Watcher)](https://github.com/SpartanJ/efsw) library with JUCE's event system.

## How it Works

This module wraps the `efsw` library to provide file system monitoring in a JUCE-idiomatic way. It offers:

- **Cross-platform file watching** - Works on Windows, macOS, and Linux
- **Recursive directory monitoring** - Watch entire directory trees
- **Multiple notification modes** - Synchronous or asynchronous callbacks, using `ChangeBroadcaster` and `AsyncUpdater` patterns
- **Thread-safe** - Safe to use from multiple threads with proper synchronization

The module detects file system events (add, delete, modify, move) and notifies listeners through JUCE's standard listener pattern.

## How to use

### Basic Usage

```cpp
#include <playfultones_filewatcher/playfultones_filewatcher.h>

class MyFileWatcherListener : public PlayfulTones::FileWatcher::FileWatcherListener
{
public:
    void fileActionPerformed (const juce::File& file, 
                             const juce::File& oldFile,
                             PlayfulTones::FileWatcher::FileAction action) override
    {
        switch (action)
        {
            case PlayfulTones::FileWatcher::FileAction::Add:
                DBG ("File added: " + file.getFullPathName());
                break;
            case PlayfulTones::FileWatcher::FileAction::Delete:
                DBG ("File deleted: " + file.getFullPathName());
                break;
            case PlayfulTones::FileWatcher::FileAction::Modified:
                DBG ("File modified: " + file.getFullPathName());
                break;
            case PlayfulTones::FileWatcher::FileAction::Moved:
                DBG ("File moved from: " + oldFile.getFullPathName() + " to: " + file.getFullPathName());
                break;
        }
    }
};

// In your application
PlayfulTones::FileWatcher::FileWatcher fileWatcher;
MyFileWatcherListener listener;

// Add listener
fileWatcher.addListener (&listener);

// Start watching a directory
juce::File directoryToWatch = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory);
fileWatcher.startWatching (directoryToWatch, true, false); // recursive=true, async=false

// Stop watching
fileWatcher.stopWatching();
```

### Using ChangeBroadcaster

The `FileWatcher` also inherits from `ChangeBroadcaster`, so you can listen for changes without implementing the specific interface:

```cpp
class MyComponent : public juce::Component,
                   public juce::ChangeListener
{
public:
    MyComponent()
    {
        fileWatcher.addChangeListener (this);
    }
    
    void changeListenerCallback (juce::ChangeBroadcaster*) override
    {
        // File system change detected
        DBG ("File system changed in: " + fileWatcher.getWatchedDirectory().getFullPathName());
    }
    
private:
    PlayfulTones::FileWatcher::FileWatcher fileWatcher;
};
```

### Asynchronous Mode

For better performance in GUI applications, use asynchronous mode:

```cpp
// Enable async updates - callbacks will be made on the message thread
fileWatcher.startWatching (directoryToWatch, true, true); // useAsync=true
```

## Integrating with your project

### Step 1: Add efsw dependency

### FetchContent

Add efsw to your CMake project using FetchContent:

```cmake
include(FetchContent)

FetchContent_Declare(
    efsw
    GIT_REPOSITORY https://github.com/SpartanJ/efsw.git
    GIT_TAG master  # or specific version tag
)

FetchContent_MakeAvailable(efsw)

# Link efsw to your target
target_link_libraries(YourTarget PRIVATE efsw)
```

### CPM

If you are using CPM, add the following to your `CMakeLists.txt`:

```cmake
CPMAddPackage(
    NAME efsw
    GITHUB_REPOSITORY SpartanJ/efsw
    GIT_TAG master  # or specific version tag
)
target_link_libraries(YourTarget PRIVATE efsw)
```

### Step 2: Add the JUCE module

1. Copy the `playfultones_filewatcher` directory to your JUCE modules folder
2. Add the module to your project:

**Projucer:**
- Add `playfultones_filewatcher` to your modules in Projucer

**CMake:**
```cmake
juce_add_modules(YourTarget
    MODULES
        # ... other modules
        playfultones_filewatcher
)
```

### Step 3: Include and use

```cpp
#include <playfultones_filewatcher/playfultones_filewatcher.h>

// Use the classes in the PlayfulTones::FileWatcher namespace
```

## License

This module is released under the MIT License. See the individual source files for license details.

The efsw library is also MIT licensed - please check the [efsw repository](https://github.com/SpartanJ/efsw) for details.

## Contributing

Contributions are welcome! Please feel free to open issues or submit pull requests.

### Reporting Issues

Please report issues with:
- Your platform (Windows/macOS/Linux)
- JUCE version
- efsw version
- Clear reproduction steps
- Expected vs actual behavior
