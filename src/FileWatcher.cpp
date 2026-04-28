/*******************************************************************
* Copyright         : 2025 Playful Tones
* Author            : Bence Kovács
* License           : MIT License
/******************************************************************/

namespace PlayfulTones::FileWatcher
{
    // Listener indirection so callbacks already in flight have somewhere safe
    // to land after the owning FileWatcher is gone. Held by shared_ptr and
    // retired to the graveyard below.
    class FileWatcher::CallbackTrampoline : public efsw::FileWatchListener
    {
    public:
        std::atomic<FileWatcher*> owner { nullptr };

        void handleFileAction (efsw::WatchID id, const std::string& dir,
            const std::string& filename, efsw::Action action,
            std::string oldFilename) override
        {
            if (auto* fw = owner.load (std::memory_order_acquire))
                fw->handleFileAction (id, dir, filename, action, std::move (oldFilename));
        }
    };

    namespace
    {
        // efsw::WatcherFSEvents::~WatcherFSEvents tears down the FSEventStream
        // without draining its libdispatch queue, so a callback already
        // dispatched can fire on a freed WatcherFSEvents. Keep the watcher and
        // its listener alive by retiring them here instead of destroying them.
        struct GraveyardEntry
        {
            std::unique_ptr<efsw::FileWatcher> efsw;
            std::shared_ptr<efsw::FileWatchListener> listener;
        };

        struct Graveyard
        {
            juce::CriticalSection lock;
            std::vector<GraveyardEntry> entries;
        };

        Graveyard& getGraveyard()
        {
            static Graveyard g;
            return g;
        }

        void retireToGraveyard (std::unique_ptr<efsw::FileWatcher> efsw,
            std::shared_ptr<efsw::FileWatchListener> listener)
        {
            if (efsw == nullptr && listener == nullptr)
                return;

            auto& graveyard = getGraveyard();
            const juce::ScopedLock lock (graveyard.lock);
            graveyard.entries.push_back ({ std::move (efsw), std::move (listener) });
        }
    } // namespace

    FileWatcher::FileWatcher()
        : fileWatcher (std::make_unique<efsw::FileWatcher>()),
          trampoline (std::make_shared<CallbackTrampoline>()),
          watchId (-1),
          useAsyncUpdates (false)
    {
        trampoline->owner.store (this, std::memory_order_release);
    }

    FileWatcher::~FileWatcher()
    {
        if (trampoline)
            trampoline->owner.store (nullptr, std::memory_order_release);

        retireToGraveyard (std::move (fileWatcher), std::move (trampoline));
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
        if (pathToWatch.isDirectory())
        {
            watchId = fileWatcher->addWatch (pathToWatch.getFullPathName().toStdString(), trampoline.get(), recursive);
        }
        else
        {
            // For files, watch the parent directory
            watchId = fileWatcher->addWatch (pathToWatch.getParentDirectory().getFullPathName().toStdString(), trampoline.get(), false);
        }

        if (watchId != -1)
            fileWatcher->watch();
    }

    void FileWatcher::stopWatching()
    {
        watchedPath = juce::File();
        watchId = -1;

        // Clear any pending async updates
        {
            const juce::ScopedLock lock (pendingActionsLock);
            pendingActions.clear();
        }

        cancelPendingUpdate();

        if (trampoline)
            trampoline->owner.store (nullptr, std::memory_order_release);

        retireToGraveyard (std::move (fileWatcher), std::move (trampoline));

        fileWatcher = std::make_unique<efsw::FileWatcher>();
        trampoline = std::make_shared<CallbackTrampoline>();
        trampoline->owner.store (this, std::memory_order_release);
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
        // If watching a specific file, only process events for that file
        if (watchedPath.existsAsFile() && filename != watchedPath.getFileName().toStdString())
        {
            return;
        }

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
