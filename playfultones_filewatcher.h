/** BEGIN_JUCE_MODULE_DECLARATION
ID:               playfultones_filewatcher
vendor:           Playful Tones
version:          1.0.0
name:             Playful Tones File Watcher
description:      JUCE integration for file watching capabilities via efsw.
website:          https://playfultones.com
license:          MIT
dependencies:     juce_events
END_JUCE_MODULE_DECLARATION
*/
#pragma once
#define PLAYFULTONES_FILEWATCHER_H_INCLUDED

#include <efsw/efsw.hpp>
#include <juce_events/juce_events.h>

#include "src/enums/FileAction.h"
#include "src/interfaces/FileWatcherListener.h"

#include "src/FileWatcher.h"
