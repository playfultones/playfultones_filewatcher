/*******************************************************************
* Copyright         : 2025 Playful Tones
* Author            : Bence Kovács
* License           : MIT License
/******************************************************************/

#pragma once

namespace PlayfulTones::FileWatcher
{
    /** @brief Interface for classes that want to receive file system change notifications.
        
        Implement this interface to receive callbacks when files or directories
        are added, deleted, modified, or moved within a watched directory.
    */
    class FileWatcherListener
    {
    public:
        /** @brief Virtual destructor. */
        virtual ~FileWatcherListener() = default;

        /** @brief Called when a file system action is detected.
            
            @param file The file or directory that was affected by the action
            @param oldFile For move operations, the original location of the file. 
                          Empty for other actions.
            @param action The type of action that occurred (Add, Delete, Modified, Moved)
        */
        virtual void fileActionPerformed (const juce::File& file,
            const juce::File& oldFile,
            FileAction action) = 0;
    };
}
