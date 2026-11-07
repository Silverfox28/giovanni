// Conversioni tra le stringhe del "motore" (std::string UTF-8) e quelle di Qt.
#pragma once

#include <QString>

#include <filesystem>
#include <string>

inline QString toQ(const std::string& s) {
    return QString::fromStdString(s);
}

inline std::string toStd(const QString& s) {
    return s.toStdString();
}

// Percorso di file che funziona anche con lettere accentate nel nome (in particolare su Windows).
inline std::filesystem::path toPath(const QString& s) {
#ifdef _WIN32
    return std::filesystem::path(s.toStdWString());
#else
    return std::filesystem::path(s.toStdString());
#endif
}
