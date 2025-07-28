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

        watchedPath = pathToWatch;
        useAsyncUpdates = useAsync;

        // For individual files, efsw watches the parent directory but filters for the specific file
        // The recursive parameter is ignored for individual files
        watchId = fileWatcher->addWatch (pathToWatch.getFullPathName().toStdString(), this, recursive);

        if (watchId != -1)
            fileWatcher->watch();
    }

    void FileWatcher::stopWatching()
    {
        if (watchId != -1)
        {
            fileWatcher->removeWatch (watchId);
            watchId = -1;
        }

        watchedPath = juce::File();

        // Clear any pending async updates
        {
            const juce::ScopedLock lock (pendingActionsLock);
            pendingActions.clear();
        }

        cancelPendingUpdate();
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

    void FileWatcher::handleFileAction (efsw::WatchID, const std::string& dir, const std::string& filename, efsw::Action action, std::string oldFilename)
    {
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
        const juce::ScopedLock lock (listenersLock);
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
