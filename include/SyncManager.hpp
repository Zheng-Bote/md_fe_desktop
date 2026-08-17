#pragma once
#include "DatabaseManager.hpp"
#include "ConfigLoader.hpp"
#include <memory>
#include <QObject>

class SyncManager : public QObject {
    Q_OBJECT
public:
    SyncManager(std::shared_ptr<DatabaseManager> dbMgr, const DesktopConfig& config, QObject* parent = nullptr);
    void performSync(const std::string& jwtToken);

signals:
    void syncFinished(bool success);

private:
    std::shared_ptr<DatabaseManager> dbManager;
    DesktopConfig m_config;
};
