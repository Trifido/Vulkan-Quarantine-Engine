#pragma once
#ifndef QE_ASSET_IMPORTER_MANAGER
#define QE_ASSET_IMPORTER_MANAGER

#include <atomic>
#include <string>
#include <memory>
#include <vector>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>

enum class QEImportJobState
{
    Queued,
    Running,
    Succeeded,
    Failed
};

struct QEImportProgress
{
    std::atomic<float> Value{ 0.0f };

    mutable std::mutex Mutex;
    std::string Stage;
    std::string Message;
};

struct QEImportJob
{
    uint64_t Id = 0;
    std::string SourcePath;
    std::string DisplayName;
    std::string TargetFolder;

    std::atomic<QEImportJobState> State{ QEImportJobState::Queued };
    QEImportProgress Progress;

    mutable std::mutex ResultMutex;
    std::string ResultPath;
    std::string Error;

    void SetProgress(float value, const std::string& stage, const std::string& message)
    {
        Progress.Value.store(value, std::memory_order_relaxed);
        std::lock_guard<std::mutex> lock(Progress.Mutex);
        Progress.Stage = stage;
        Progress.Message = message;
    }

    void SetError(const std::string& error)
    {
        std::lock_guard<std::mutex> lock(ResultMutex);
        Error = error;
    }

    void SetResultPath(const std::string& path)
    {
        std::lock_guard<std::mutex> lock(ResultMutex);
        ResultPath = path;
    }
};

class QEAssetImportManager
{
public:
    static QEAssetImportManager& Get();

    QEAssetImportManager();
    ~QEAssetImportManager();

    std::shared_ptr<QEImportJob> EnqueueMeshImport(
        const std::string& sourcePath,
        const std::string& targetFolder);

    void UpdateMainThread();
    std::vector<std::shared_ptr<QEImportJob>> GetJobsSnapshot() const;
    bool HasActiveJobs() const;
    int ConsumeFinishedSuccessfulImports();

private:
    void WorkerLoop();

    QEAssetImportManager(const QEAssetImportManager&) = delete;
    QEAssetImportManager& operator=(const QEAssetImportManager&) = delete;

private:
    std::thread _worker;
    std::atomic<bool> _running{ true };
    std::atomic<int> _pendingSuccessfulRefreshes{ 0 };

    mutable std::mutex _mutex;
    std::condition_variable _cv;

    std::queue<std::shared_ptr<QEImportJob>> _pending;
    std::vector<std::shared_ptr<QEImportJob>> _jobs;
    std::queue<std::shared_ptr<QEImportJob>> _finished;

    std::atomic<uint64_t> _nextId{ 1 };
};

#endif
