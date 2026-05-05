/*******************************************************************
* Copyright         : 2025 Playful Tones
* Author            : Bence Kovács
* License           : MIT License
/******************************************************************/

namespace PlayfulTones::FileWatcher
{
    FileWatcher::FileWatcher()
        : fileWatcher (std::make_unique<efsw::FileWatcher>()), watchId (-1), useAsyncUpdates (false)
    {
    }

    FileWatcher::~FileWatcher()
    {
        stopWatching();

        // Join the efsw thread before base-class destructors rewind the vptr.
        fileWatcher.reset();
    }

    void FileWatcher::startWatching (const juce::File& pathToWatch, bool useAsync, bool recursive)
    {
        stopWatching();

        if (!pathToWatch.exists())
        {
            /**
             * If you hit this assertion, it means that the file or directory you are trying to watch does not exist.
             * Please ensure that the path is correct and that the file/directory is accessible.
             */
            jassertfalse;
            return;
        }

        useAsyncUpdates = useAsync;

        // Publish the path before addWatch so callbacks fired during registration see it.
        {
            const juce::ScopedLock lock (stateLock);
            watchedPath = pathToWatch;
        }

        // For individual files, efsw watches the parent directory and we filter by name.
        const auto efswPath = pathToWatch.isDirectory()
                                  ? pathToWatch.getFullPathName().toStdString()
                                  : pathToWatch.getParentDirectory().getFullPathName().toStdString();
        const bool efswRecursive = pathToWatch.isDirectory() ? recursive : false;
        const auto newWatchId = fileWatcher->addWatch (efswPath, this, efswRecursive);

        {
            const juce::ScopedLock lock (stateLock);
            watchId = newWatchId;
            if (newWatchId == -1)
                watchedPath = juce::File();
        }

        if (newWatchId != -1)
            fileWatcher->watch();
    }

    void FileWatcher::stopWatching()
    {
        efsw::WatchID idToRemove = -1;

        {
            const juce::ScopedLock lock (stateLock);
            idToRemove = watchId;
            watchId = -1;
            watchedPath = juce::File();
        }

        if (idToRemove != -1)
            fileWatcher->removeWatch (idToRemove);

        // Clear any pending async updates
        {
            const juce::ScopedLock lock (pendingActionsLock);
            pendingActions.clear();
        }

        cancelPendingUpdate();
    }

    bool FileWatcher::isWatching() const noexcept
    {
        const juce::ScopedLock lock (stateLock);
        return watchId != -1;
    }

    juce::File FileWatcher::getWatchedPath() const noexcept
    {
        const juce::ScopedLock lock (stateLock);
        return watchedPath;
    }

    void FileWatcher::addListener (FileWatcherListener* listener)
    {
        const juce::ScopedLock lock (listenersLock);
        listeners.add (listener);
    }

    void FileWatcher::removeListener (FileWatcherListener* listener)
    {
        const juce::ScopedLock lock (listenersLock);
        listeners.remove (listener);
    }

    void FileWatcher::handleFileAction (efsw::WatchID, const std::string& dir, const std::string& filename, efsw::Action action, const std::string& oldFilename)
    {
        juce::File capturedPath;

        {
            const juce::ScopedLock lock (stateLock);
            if (watchedPath == juce::File())
                return; // stopWatching has cleared the path; this callback is stale
            capturedPath = watchedPath;
        }

        // If watching a specific file, only process events for that file
        if (capturedPath.existsAsFile() && filename != capturedPath.getFileName().toStdString())
            return;

        const auto file = juce::File (juce::String (dir) + juce::File::getSeparatorString() + juce::String (filename));
        const auto oldFile = oldFilename.empty() ? juce::File()
                                                 : juce::File (juce::String (dir) + juce::File::getSeparatorString() + juce::String (oldFilename));
        const auto fileAction = convertEfswAction (action);

        if (useAsyncUpdates)
        {
            {
                const juce::ScopedLock lock (pendingActionsLock);
                pendingActions.add ({ file, oldFile, fileAction });
            }
            triggerAsyncUpdate();
        }
        else
        {
            notifyListeners (file, oldFile, fileAction);
        }
    }

    void FileWatcher::handleAsyncUpdate()
    {
        const juce::ScopedLock listenerLock (listenersLock);
        juce::Array<PendingFileAction> actionsToProcess;

        {
            const juce::ScopedLock lock (pendingActionsLock);
            actionsToProcess.swapWith (pendingActions);
        }

        for (const auto& action : actionsToProcess)
            notifyListeners (action.file, action.oldFile, action.action);
    }

    FileAction FileWatcher::convertEfswAction (efsw::Action action)
    {
        switch (action)
        {
            case efsw::Actions::Add:
                return FileAction::Add;
            case efsw::Actions::Delete:
                return FileAction::Delete;
            case efsw::Actions::Modified:
                return FileAction::Modified;
            case efsw::Actions::Moved:
                return FileAction::Moved;
            default:
                return FileAction::Modified;
        }
    }

    void FileWatcher::notifyListeners (const juce::File& file, const juce::File& oldFile, FileAction action)
    {
        const juce::ScopedLock lock (listenersLock);
        listeners.call ([&] (FileWatcherListener& l) {
            l.fileActionPerformed (file, oldFile, action);
        });

        sendChangeMessage();
    }
}
