#include "plugins/BaseFileWatcherPlugin.hpp"

namespace plugins {

BaseFileWatcherPlugin::BaseFileWatcherPlugin(const QStringList& nameFilters, QObject* parent)
    : QObject(parent), m_nameFilters(nameFilters) {}

BaseFileWatcherPlugin::~BaseFileWatcherPlugin() {
    shutdown();
}

bool BaseFileWatcherPlugin::init(const DeviceConfig& config, 
          std::function<void(const std::string&, const std::map<std::string, double>&, const std::string&, const std::string&)> callback) {
    m_config = config;
    m_callback = callback;

    QDir dir(QString::fromStdString(config.pathOrPort));
    if (!dir.exists()) {
        spdlog::warn("BaseFileWatcherPlugin: Directory does not exist, attempting to create: {}", config.pathOrPort);
        if (!dir.mkpath(".")) {
            spdlog::error("BaseFileWatcherPlugin: Failed to create directory.");
            return false;
        }
    }

    m_watcher = new QFileSystemWatcher(this);
    m_watcher->addPath(QString::fromStdString(m_config.pathOrPort));

    connect(m_watcher, &QFileSystemWatcher::directoryChanged, this, &BaseFileWatcherPlugin::onDirectoryChanged);
    connect(m_watcher, &QFileSystemWatcher::fileChanged, this, &BaseFileWatcherPlugin::onFileChanged);

    spdlog::info("BaseFileWatcherPlugin: Initialized. Watching directory: {}", m_config.pathOrPort);
    
    // Initial scan for already existing files
    onDirectoryChanged(QString::fromStdString(m_config.pathOrPort));
    
    return true;
}

void BaseFileWatcherPlugin::shutdown() {
    if (m_watcher) {
        delete m_watcher;
        m_watcher = nullptr;
    }
}

void BaseFileWatcherPlugin::readMeasurement() {
    // Event-driven via QFileSystemWatcher, no manual poll needed.
}

void BaseFileWatcherPlugin::onDirectoryChanged(const QString &path) {
    QDir dir(path);
    dir.setNameFilters(m_nameFilters);
    
    QFileInfoList list = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
    for (const QFileInfo& fileInfo : list) {
        handleFile(fileInfo.absoluteFilePath());
    }
}

void BaseFileWatcherPlugin::onFileChanged(const QString &path) {
    bool matchesFilter = false;
    for (const auto& filter : m_nameFilters) {
        // Simple wildcard match just for the suffix check here if needed,
        // but QDir name filters are usually enough. We just check if it matches the expected extension.
        QString ext = filter;
        ext.remove("*");
        if (path.endsWith(ext, Qt::CaseInsensitive)) {
            matchesFilter = true;
            break;
        }
    }
    
    if (matchesFilter) {
        handleFile(path);
    }
}

void BaseFileWatcherPlugin::handleFile(const QString& filePath) {
    // Robust file opening with debounce and retry loop
    auto tryProcess = [this, filePath]() {
        // If the file was already processed and deleted by an earlier event, abort.
        if (!QFile::exists(filePath)) {
            return;
        }

        QFile file(filePath);
        
        // Try to open with ReadWrite. If the external device is still writing, this usually fails on Windows.
        // QIODevice::ReadWrite creates the file if it doesn't exist, which is why we check QFile::exists first!
        if (!file.open(QIODevice::ReadWrite | QIODevice::Text)) {
            spdlog::debug("BaseFileWatcherPlugin: File {} is locked or still being written. Retrying in 1s...", filePath.toStdString());
            // Retry again in 1 second
            QTimer::singleShot(1000, [this, filePath]() { handleFile(filePath); });
            return;
        }

        QByteArray fileData = file.readAll();
        file.close();

        // Delete the file so we don't process it again
        QFile::remove(filePath);

        // Delegate parsing and callback to derived class
        processFileData(filePath, fileData);
    };
    
    // Initial delay of 500ms, then it will enter the retry loop if needed
    QTimer::singleShot(500, tryProcess);
}

} // namespace plugins
