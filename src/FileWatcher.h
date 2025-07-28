/*******************************************************************
* Copyright         : 2025 Playful Tones
* Author            : Bence Kovács
* License           : MIT License
/******************************************************************/

#pragma once

namespace PlayfulTones::FileWatcher
{
    /** @brief A cross-platform file system watcher that integrates efsw with JUCE.
        
        This class provides file system monitoring capabilities by wrapping the efsw library
        and integrating it with JUCE's event system. It supports both synchronous and 
        asynchronous notification modes and can monitor directories recursively.
        
        The class inherits from ChangeBroadcaster to provide general change notifications
        and AsyncUpdater to support thread-safe asynchronous callbacks.
    */
    class FileWatcher : public juce::ChangeBroadcaster,
                        public juce::AsyncUpdater,
                        private efsw::FileWatchListener
    {
    public:
        /** @brief Constructs a new FileWatcher instance. */
        FileWatcher();

        /** @brief Destructor. Automatically stops watching if still active. */
        ~FileWatcher() override;

        /** @brief Starts watching a file or directory for file system changes.
            
            @param pathToWatch The file or directory to monitor. It must be a valid path.
            @param useAsync If true, callbacks will be made asynchronously on the message thread.
                           If false, callbacks are made synchronously from the file watcher thread.
            @param recursive If true and watching a directory, subdirectories will also be monitored recursively.
                            This parameter is ignored when watching individual files.
        */
        void startWatching (const juce::File& pathToWatch, bool useAsync = false, bool recursive = true);

        /** @brief Stops watching the current directory and clears all pending notifications. */
        void stopWatching();

        /** @brief Returns true if currently watching a directory.
            
            @return true if a directory is being monitored, false otherwise
        */
        bool isWatching() const noexcept { return watchId != -1; }

        /** @brief Returns the file or directory currently being watched.
            
            @return The file or directory being monitored, or an invalid File if not watching
        */
        juce::File getWatchedPath() const noexcept { return watchedPath; }

        /** @brief Adds a listener to receive file action notifications.
            
            @param listener The listener to add. Must not be null.
        */
        void addListener (FileWatcherListener* listener);

        /** @brief Removes a previously added listener.
            
            @param listener The listener to remove.
        */
        void removeListener (FileWatcherListener* listener);

        /** @brief Handles asynchronous updates when in async mode.
            
            This method is called automatically by the JUCE message system
            when async updates are triggered.
        */
        void handleAsyncUpdate() override;

    private:
        // efsw::FileWatchListener
        void handleFileAction (efsw::WatchID watchid, const std::string& dir, const std::string& filename, efsw::Action action, std::string oldFilename = "") override;

        std::unique_ptr<efsw::FileWatcher> fileWatcher;
        efsw::WatchID watchId;
        juce::File watchedPath;
        bool useAsyncUpdates;
        juce::ListenerList<FileWatcherListener> listeners;

        /** @brief Structure to hold file action information for async processing. */
        struct PendingFileAction
        {
            juce::File file; /**< The file that was affected */
            juce::File oldFile; /**< For move operations, the original file location */
            FileAction action; /**< The type of action that occurred */
        };

        juce::Array<PendingFileAction> pendingActions;
        juce::CriticalSection pendingActionsLock, listenersLock;

        static FileAction convertEfswAction (efsw::Action action);
        void notifyListeners (const juce::File& file, const juce::File& oldFile, FileAction action);

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FileWatcher)
    };
}
