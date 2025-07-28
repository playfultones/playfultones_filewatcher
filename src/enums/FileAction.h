/*******************************************************************
* Copyright         : 2025 Playful Tones
* Author            : Bence Kovács
* License           : MIT License
/******************************************************************/

#pragma once

namespace PlayfulTones::FileWatcher
{
    /** @brief Enumeration of file system actions that can be detected by the FileWatcher.
        
        These values correspond to the different types of file system events
        that can occur when monitoring a directory.
    */
    enum class FileAction {
        /** @brief A new file or directory was created. */
        Add = 1,

        /** @brief A file or directory was deleted. */
        Delete,

        /** @brief A file or directory was modified. */
        Modified,

        /** @brief A file or directory was moved/renamed. */
        Moved
    };
}
