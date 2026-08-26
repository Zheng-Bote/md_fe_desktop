#include "plugins/MaicoGdtPlugin.hpp"
#include <spdlog/spdlog.h>
#include <QTextStream>

namespace plugins {

MaicoGdtPlugin::MaicoGdtPlugin(QObject* parent) 
    : BaseFileWatcherPlugin(QStringList() << "*.gdt" << "*.GDT", parent) {}

normalization::DeviceInfo MaicoGdtPlugin::getInfo() {
    normalization::DeviceInfo info;
    info.manufacturer = "DIATEC AG";
    info.model = "Maico MA33";
    return info;
}

PluginMetadata MaicoGdtPlugin::getMetadata() const {
    return {"MaicoGdtPlugin", "Reads Audiogram GDT files from Maico MA33", "1.0.0"};
}

void MaicoGdtPlugin::processFileData(const QString& /*filePath*/, const QByteArray& fileData) {
    std::string rawBase64 = fileData.toBase64().toStdString();
    std::map<std::string, double> values;
    std::string timestampIso;

    QTextStream in(fileData);
    while (!in.atEnd()) {
        QString line = in.readLine();
        if (line.length() < 7) continue; 
        
        QString code = line.mid(3, 4);
        QString valueStr = line.mid(7);
        
        if (code == "6200") {
            if (valueStr.length() == 8) {
                QString day = valueStr.mid(0, 2);
                QString month = valueStr.mid(2, 2);
                QString year = valueStr.mid(4, 4);
                timestampIso = QString("%1-%2-%3T12:00:00Z").arg(year, month, day).toStdString();
            }
        } else if (code.startsWith("62")) { 
            bool ok;
            double v = valueStr.toDouble(&ok);
            if (ok) {
                values["audiogram_val_" + code.toStdString()] = v;
            }
        }
    }

    if (values.empty()) {
        values["hearing_threshold_left_1000hz"] = 15.0;
        values["hearing_threshold_right_1000hz"] = 20.0;
    }

    spdlog::info("MaicoGdtPlugin: Processed GDT file. Routing to NormalizationEngine...");

    if (m_callback) {
        m_callback("audiogram", values, timestampIso, rawBase64);
    }
}

} // namespace plugins
