#include "QEAssetImportManager.h"

#include <filesystem>
#include <exception>
#include <iostream>

#include <QEProjectManager.h>

QEAssetImportManager& QEAssetImportManager::Get()
{
    static QEAssetImportManager instance;
    return instance;
}

QEAssetImportManager::QEAssetImportManager()
{
    _worker = std::thread(&QEAssetImportManager::WorkerLoop, this);
}

QEAssetImportManager::~QEAssetImportManager()
{
    _running.store(false, std::memory_order_relaxed);
    _cv.notify_all();

    if (_worker.joinable())
    {
        _worker.join();
    }
}

std::shared_ptr<QEImportJob> QEAssetImportManager::EnqueueMeshImport(const std::string& sourcePath)
{
    auto job = std::make_shared<QEImportJob>();
    job->Id = _nextId.fetch_add(1, std::memory_order_relaxed);
    job->SourcePath = sourcePath;
    job->DisplayName = std::filesystem::path(sourcePath).filename().string();
    job->State.store(QEImportJobState::Queued, std::memory_order_relaxed);
    job->SetProgress(0.0f, "Queued", "Waiting in queue");

    {
        std::lock_guard<std::mutex> lock(_mutex);
        _jobs.push_back(job);
        _pending.push(job);
    }

    _cv.notify_one();
    return job;
}

void QEAssetImportManager::UpdateMainThread()
{
    std::queue<std::shared_ptr<QEImportJob>> finishedLocal;

    {
        std::lock_guard<std::mutex> lock(_mutex);
        std::swap(finishedLocal, _finished);
    }

    while (!finishedLocal.empty())
    {
        const auto& job = finishedLocal.front();

        if (job)
        {
            QEImportJobState state = job->State.load(std::memory_order_relaxed);

            if (state == QEImportJobState::Succeeded)
            {
                job->SetProgress(1.0f, "Completed", "Import finished");
            }
            else if (state == QEImportJobState::Failed)
            {
                job->SetProgress(job->Progress.Value.load(std::memory_order_relaxed), "Failed", "Import failed");
            }
        }

        finishedLocal.pop();
    }
}

std::vector<std::shared_ptr<QEImportJob>> QEAssetImportManager::GetJobsSnapshot() const
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _jobs;
}

bool QEAssetImportManager::HasActiveJobs() const
{
    std::lock_guard<std::mutex> lock(_mutex);

    if (!_pending.empty())
        return true;

    for (const auto& job : _jobs)
    {
        if (!job)
            continue;

        QEImportJobState state = job->State.load(std::memory_order_relaxed);
        if (state == QEImportJobState::Queued || state == QEImportJobState::Running)
            return true;
    }

    return false;
}

void QEAssetImportManager::WorkerLoop()
{
    while (true)
    {
        std::shared_ptr<QEImportJob> job;

        {
            std::unique_lock<std::mutex> lock(_mutex);

            _cv.wait(lock, [this]()
                {
                    return !_running.load(std::memory_order_relaxed) || !_pending.empty();
                });

            if (!_running.load(std::memory_order_relaxed) && _pending.empty())
                return;

            job = _pending.front();
            _pending.pop();
        }

        if (!job)
            continue;

        try
        {
            job->State.store(QEImportJobState::Running, std::memory_order_relaxed);
            job->SetProgress(0.05f, "Starting", "Preparing import");

            QEProjectManager::ImportMeshFile(job->SourcePath);

            job->SetResultPath(job->SourcePath);
            job->State.store(QEImportJobState::Succeeded, std::memory_order_relaxed);
            job->SetProgress(1.0f, "Completed", "Import finished");
        }
        catch (const std::exception& e)
        {
            job->SetError(e.what());
            job->State.store(QEImportJobState::Failed, std::memory_order_relaxed);
            job->SetProgress(job->Progress.Value.load(std::memory_order_relaxed), "Failed", e.what());
        }
        catch (...)
        {
            job->SetError("Unknown import error");
            job->State.store(QEImportJobState::Failed, std::memory_order_relaxed);
            job->SetProgress(job->Progress.Value.load(std::memory_order_relaxed), "Failed", "Unknown import error");
        }

        {
            std::lock_guard<std::mutex> lock(_mutex);
            _finished.push(job);
        }
    }
}
