/** BEGIN_JUCE_MODULE_DECLARATION
ID:               playfultones_filewatcher
vendor:           Playful Tones
version:          1.0.0
name:             Playful Tones File Watcher
description:      JUCE integration for file watching capabilities via efsw.
website:          https://playfultones.com
license:          MIT
dependencies:     juce_core
END_JUCE_MODULE_DECLARATION
*/
#pragma once
#define PLAYFULTONES_FILEWATCHER_H_INCLUDED

#include <efsw/efsw.hpp>
#include <juce_core/juce_core.h>

#include "src/FileWatcher.h"
#include "src/enums/FileAction.h"
#include "src/interfaces/FileWatcherListener.h"
