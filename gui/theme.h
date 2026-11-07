// Aspetto grafico dell'applicazione: temi chiaro/scuro, colori e icone.
#pragma once

#include <QColor>
#include <QIcon>
#include <QString>

#include "component.h"

enum class ThemeMode {
    Light,   // tema chiaro (predefinito)
    Dark,    // tema scuro
    System,  // chiaro o scuro come il desktop
};

struct ThemeColors {
    bool dark = false;
    QColor window;      // sfondo della finestra
    QColor surface;     // sfondo di tabelle, riquadri e caselle
    QColor alternate;   // righe alternate della tabella
    QColor header;      // intestazioni della tabella
    QColor border;
    QColor text;
    QColor muted;       // testo secondario
    QColor accent;      // colore principale (selezione, pulsanti)
    QColor accentSoft;  // sfondo al passaggio del mouse
    QColor accentText;  // testo sopra il colore principale
    QColor dangerBg;    // componenti da riordinare
    QColor dangerFg;
    QColor successBg;
    QColor successFg;
};

// Applica il tema a tutta l'applicazione (stile, palette e foglio di stile).
void applyTheme(ThemeMode mode);

// Colori del tema attualmente in uso.
const ThemeColors& theme();

ThemeMode loadThemeMode();
void saveThemeMode(ThemeMode mode);

enum class IconKind {
    Add,
    Edit,
    Duplicate,
    Delete,
    Withdraw,
    Restock,
    Reorder,
    Statistics,
    ColorCode,
    Search,
};

// Icone disegnate dal programma (non servono file esterni).
QIcon makeIcon(IconKind kind, const QColor& color);

// Colore identificativo di ogni categoria e relativo pallino.
QColor categoryColor(Category category);
QIcon categoryIcon(Category category);

// Stile di una "etichetta a pillola" colorata.
QString pillStyle(const QColor& background, const QColor& foreground);
