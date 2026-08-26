#pragma once

#include "BaseFileWatcherPlugin.hpp"

namespace plugins {

class MaicoGdtPlugin : public BaseFileWatcherPlugin {
    Q_OBJECT
public:
    explicit MaicoGdtPlugin(QObject* parent = nullptr);
    virtual ~MaicoGdtPlugin() override = default;

    normalization::DeviceInfo getInfo() override;
    PluginMetadata getMetadata() const override;

protected:
    void processFileData(const QString& filePath, const QByteArray& fileData) override;
};

} // namespace plugins
