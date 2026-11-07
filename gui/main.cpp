// Catalogo dei componenti elettronici: versione grafica (Qt).
//
// Uso:  catalogo_gui [file.csv]
// Senza argomenti riapre l'ultimo archivio usato, oppure "componenti.csv" nella cartella Documenti.

#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QLocale>
#include <QSettings>
#include <QStandardPaths>

#include "mainwindow.h"
#include "theme.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QApplication::setOrganizationName(QStringLiteral("CatalogoComponenti"));
    QApplication::setApplicationName(QStringLiteral("Catalogo componenti"));
    QLocale::setDefault(QLocale(QLocale::Italian, QLocale::Italy));  // virgola nei numeri decimali
    applyTheme(loadThemeMode());  // tema chiaro, a meno che l'utente non ne abbia scelto un altro

    QString path;
    const QStringList args = QApplication::arguments();
    if (args.size() > 1) {
        path = args.at(1);
    } else {
        path = QSettings().value(QStringLiteral("ultimoArchivio")).toString();
        if (path.isEmpty() || !QFileInfo::exists(path)) {
            const QString documents = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
            QDir().mkpath(documents);
            path = documents + QStringLiteral("/componenti.csv");
        }
    }

    MainWindow window;
    window.show();
    window.openFile(path);
    return app.exec();
}
