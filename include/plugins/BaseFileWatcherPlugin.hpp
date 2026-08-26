#pragma once

#include "IDevicePlugin.hpp"
#include <QObject>
#include <QFileSystemWatcher>
#include <QString>
#include <QStringList>
#include <QDir>
#include <QTimer>
#include <QFile>
#include <spdlog/spdlog.h>

namespace plugins {

class BaseFileWatcherPlugin : public QObject, public IDevicePlugin {
    Q_OBJECT
public:
    explicit BaseFileWatcherPlugin(const QStringList& nameFilters, QObject* parent = nullptr);
    virtual ~BaseFileWatcherPlugin() override;

    bool init(const DeviceConfig& config, 
              std::function<void(const std::string&, const std::map<std::string, double>&, const std::string&, const std::string&)> callback) override;
    
    void readMeasurement() override;
    void shutdown() override;

protected:
    DeviceConfig m_config;
    std::function<void(const std::string&, const std::map<std::string, double>&, const std::string&, const std::string&)> m_callback;
    QStringList m_nameFilters;

    // Derived classes must implement this to parse the successfully opened file data.
    // They are responsible for calling m_callback when done.
    virtual void processFileData(const QString& filePath, const QByteArray& fileData) = 0;

private slots:
    void onDirectoryChanged(const QString &path);
    void onFileChanged(const QString &path);

private:
    void handleFile(const QString& filePath);

    QFileSystemWatcher* m_watcher = nullptr;
};

} // namespace plugins
