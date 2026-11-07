#include "theme.h"

#include <QApplication>
#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QPixmap>
#include <QSettings>
#include <QStyle>
#include <QStyleFactory>
#include <QTemporaryDir>

#include <functional>
#include <map>

namespace {

const char* const kThemeKey = "tema";

ThemeColors lightColors() {
    ThemeColors c;
    c.dark = false;
    c.window = QColor("#eef1f6");
    c.surface = QColor("#ffffff");
    c.alternate = QColor("#f6f8fb");
    c.header = QColor("#f3f5f9");
    c.border = QColor("#d6dce5");
    c.text = QColor("#1d2530");
    c.muted = QColor("#667085");
    c.accent = QColor("#2563eb");
    c.accentSoft = QColor("#e3ebfd");
    c.accentText = QColor("#ffffff");
    c.dangerBg = QColor("#fdecea");
    c.dangerFg = QColor("#b42318");
    c.successBg = QColor("#dcfce7");
    c.successFg = QColor("#15803d");
    return c;
}

ThemeColors darkColors() {
    ThemeColors c;
    c.dark = true;
    c.window = QColor("#1b1e24");
    c.surface = QColor("#252a32");
    c.alternate = QColor("#2a3039");
    c.header = QColor("#2c323c");
    c.border = QColor("#3a414d");
    c.text = QColor("#e7eaf0");
    c.muted = QColor("#9aa4b2");
    c.accent = QColor("#5b8def");
    c.accentSoft = QColor("#2d3c58");
    c.accentText = QColor("#ffffff");
    c.dangerBg = QColor("#4a2427");
    c.dangerFg = QColor("#ffb4ab");
    c.successBg = QColor("#1e3a2a");
    c.successFg = QColor("#86efac");
    return c;
}

ThemeColors& current() {
    static ThemeColors colors = lightColors();
    return colors;
}

// Frecce per menu a tendina e caselle numeriche: il foglio di stile può usare solo immagini,
// quindi vengono disegnate nei colori del tema e salvate in una cartella temporanea.
QString writeArrow(const QString& name, bool up, const QColor& color) {
    static QTemporaryDir dir;
    QPixmap pixmap(24, 24);
    pixmap.fill(Qt::transparent);
    QPainter p(&pixmap);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(color, 2.6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    QPainterPath chevron;
    chevron.moveTo(5, up ? 15 : 9);
    chevron.lineTo(12, up ? 8 : 16);
    chevron.lineTo(19, up ? 15 : 9);
    p.drawPath(chevron);
    p.end();
    const QString path = dir.path() + QLatin1Char('/') + name + QStringLiteral(".png");
    pixmap.save(path, "PNG");
    return path;
}

QPalette makePalette(const ThemeColors& c) {
    QPalette p;
    p.setColor(QPalette::Window, c.window);
    p.setColor(QPalette::WindowText, c.text);
    p.setColor(QPalette::Base, c.surface);
    p.setColor(QPalette::AlternateBase, c.alternate);
    p.setColor(QPalette::Text, c.text);
    p.setColor(QPalette::Button, c.surface);
    p.setColor(QPalette::ButtonText, c.text);
    p.setColor(QPalette::ToolTipBase, c.surface);
    p.setColor(QPalette::ToolTipText, c.text);
    p.setColor(QPalette::Highlight, c.accent);
    p.setColor(QPalette::HighlightedText, c.accentText);
    p.setColor(QPalette::Link, c.accent);
    p.setColor(QPalette::PlaceholderText, c.muted);
    p.setColor(QPalette::Mid, c.border);
    p.setColor(QPalette::Dark, c.border.darker(130));
    p.setColor(QPalette::Light, c.surface.lighter(110));
    for (QPalette::ColorRole role : {QPalette::WindowText, QPalette::Text, QPalette::ButtonText}) {
        p.setColor(QPalette::Disabled, role, c.muted);
    }
    return p;
}

QString makeStyleSheet(const ThemeColors& c) {
    QString sheet = QStringLiteral(R"(
QMainWindow, QDialog { background: @window; }
QToolBar { background: @surface; border: none; border-bottom: 1px solid @border; padding: 4px 8px; spacing: 2px; }
QToolBar::separator { background: @border; width: 1px; margin: 8px 6px; }
QToolButton { color: @text; padding: 4px 8px; border-radius: 8px; border: 1px solid transparent; }
QToolButton:hover { background: @accentSoft; }
QToolButton:pressed, QToolButton:checked { background: @accentSoft; border-color: @accent; }
QToolButton:disabled { color: @muted; }
QMenuBar { background: @surface; color: @text; padding: 2px; }
QMenuBar::item { padding: 4px 10px; border-radius: 4px; }
QMenuBar::item:selected { background: @accentSoft; }
QMenu { background: @surface; color: @text; border: 1px solid @border; padding: 4px; }
QMenu::item { padding: 6px 24px 6px 28px; border-radius: 4px; }
QMenu::item:selected { background: @accent; color: @accentText; }
QMenu::item:disabled { color: @muted; }
QMenu::separator { height: 1px; background: @border; margin: 4px 8px; }
QFrame#card { background: @surface; border: 1px solid @border; border-radius: 10px; }
QFrame#card QLabel { background: transparent; }
QLabel { color: @text; }
QLabel#muted { color: @muted; }
QLabel#kpiTitle { color: @muted; font-weight: 600; }
QLabel#kpiValue { font-size: 20pt; font-weight: 700; }
QLabel#kpiDanger { font-size: 20pt; font-weight: 700; color: @dangerFg; }
QLabel#detailTitle { font-size: 15pt; font-weight: 700; }
QLineEdit, QComboBox, QSpinBox, QDoubleSpinBox {
    background: @surface; color: @text; border: 1px solid @border; border-radius: 6px;
    padding: 5px 8px; selection-background-color: @accent; selection-color: @accentText; }
QLineEdit:focus, QComboBox:focus, QSpinBox:focus, QDoubleSpinBox:focus { border: 1px solid @accent; }
QLineEdit:disabled, QComboBox:disabled { color: @muted; background: @alternate; }
QComboBox { padding-right: 26px; }
QComboBox::drop-down { subcontrol-origin: padding; subcontrol-position: center right; width: 24px; border: none; }
QComboBox::down-arrow { image: url("@arrowDown"); width: 11px; height: 11px; }
QComboBox::down-arrow:disabled { image: url("@arrowDisabled"); }
QSpinBox, QDoubleSpinBox { padding-right: 24px; }
QSpinBox::up-button, QDoubleSpinBox::up-button, QSpinBox::down-button, QDoubleSpinBox::down-button {
    subcontrol-origin: border; width: 22px; border: none; border-left: 1px solid @border; background: transparent; }
QSpinBox::up-button, QDoubleSpinBox::up-button { subcontrol-position: top right; border-top-right-radius: 6px; }
QSpinBox::down-button, QDoubleSpinBox::down-button { subcontrol-position: bottom right; border-bottom-right-radius: 6px; }
QSpinBox::up-button:hover, QDoubleSpinBox::up-button:hover,
QSpinBox::down-button:hover, QDoubleSpinBox::down-button:hover { background: @accentSoft; }
QSpinBox::up-arrow, QDoubleSpinBox::up-arrow { image: url("@arrowUp"); width: 9px; height: 9px; }
QSpinBox::down-arrow, QDoubleSpinBox::down-arrow { image: url("@arrowDown"); width: 9px; height: 9px; }
QComboBox QAbstractItemView { background: @surface; color: @text; border: 1px solid @border;
    selection-background-color: @accent; selection-color: @accentText; outline: none; }
QTableView, QTableWidget {
    background: @surface; alternate-background-color: @alternate; color: @text;
    border: 1px solid @border; border-radius: 10px; gridline-color: @border;
    selection-background-color: @accent; selection-color: @accentText; outline: none; }
QHeaderView { background: @header; border: none; border-top-left-radius: 10px; border-top-right-radius: 10px; }
QHeaderView::section { background: @header; color: @muted; font-weight: 600; border: none;
    border-bottom: 1px solid @border; padding: 7px 6px; }
QTableCornerButton::section { background: @header; border: none; }
QPushButton { background: @surface; color: @text; border: 1px solid @border; border-radius: 6px; padding: 6px 14px; }
QPushButton:hover { border-color: @accent; background: @accentSoft; }
QPushButton:default, QPushButton#primary { background: @accent; color: @accentText; border-color: @accent; }
QPushButton:default:hover, QPushButton#primary:hover { background: @accentHover; }
QPushButton:disabled { color: @muted; background: @alternate; border-color: @border; }
QCheckBox { color: @text; spacing: 6px; }
QStatusBar { background: @surface; color: @muted; border-top: 1px solid @border; }
QStatusBar QLabel { color: @muted; padding: 2px 6px; }
QScrollArea { background: transparent; border: none; }
QSplitter::handle { background: transparent; }
QTabWidget::pane { border: 1px solid @border; border-radius: 8px; background: @surface; top: -1px; }
QTabBar::tab { background: @alternate; color: @muted; padding: 7px 16px; border: 1px solid @border;
    border-bottom: none; border-top-left-radius: 6px; border-top-right-radius: 6px; margin-right: 2px; }
QTabBar::tab:selected { background: @surface; color: @text; font-weight: 600; }
QGroupBox { background: @surface; border: 1px solid @border; border-radius: 8px; margin-top: 16px;
    padding-top: 10px; color: @text; font-weight: 600; }
QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 4px; }
QToolTip { background: @surface; color: @text; border: 1px solid @border; padding: 4px; }
QScrollBar:vertical { background: transparent; width: 12px; margin: 2px; }
QScrollBar:horizontal { background: transparent; height: 12px; margin: 2px; }
QScrollBar::handle { background: @border; border-radius: 4px; min-height: 30px; min-width: 30px; }
QScrollBar::handle:hover { background: @muted; }
QScrollBar::add-line, QScrollBar::sub-line { width: 0; height: 0; }
QScrollBar::add-page, QScrollBar::sub-page { background: transparent; }
)");

    // Si sostituiscono prima i nomi più lunghi, perché "@accent" è l'inizio di "@accentSoft".
    const QString suffix = c.dark ? QStringLiteral("-scuro") : QStringLiteral("-chiaro");
    const std::map<QString, QString, std::greater<QString>> tokens = {
        {"@window", c.window.name()},
        {"@surface", c.surface.name()},
        {"@alternate", c.alternate.name()},
        {"@header", c.header.name()},
        {"@border", c.border.name()},
        {"@text", c.text.name()},
        {"@muted", c.muted.name()},
        {"@accentSoft", c.accentSoft.name()},
        {"@accentText", c.accentText.name()},
        {"@accentHover", c.accent.darker(c.dark ? 90 : 112).name()},
        {"@accent", c.accent.name()},
        {"@dangerFg", c.dangerFg.name()},
        {"@arrowDown", writeArrow("giu" + suffix, false, c.muted)},
        {"@arrowUp", writeArrow("su" + suffix, true, c.muted)},
        {"@arrowDisabled", writeArrow("giu-disattivo" + suffix, false, c.border)},
    };
    for (const auto& [token, value] : tokens) sheet.replace(token, value);
    return sheet;
}

}  // namespace

void applyTheme(ThemeMode mode) {
    // Palette del desktop, letta una sola volta prima di applicare il tema.
    static const bool systemIsDark = QApplication::palette().color(QPalette::Window).lightness() < 128;

    bool dark = mode == ThemeMode::Dark;
    if (mode == ThemeMode::System) dark = systemIsDark;
    current() = dark ? darkColors() : lightColors();

    // Lo stile "Fusion" ha lo stesso aspetto su Windows, macOS e Linux.
    if (QStyle* fusion = QStyleFactory::create(QStringLiteral("Fusion"))) QApplication::setStyle(fusion);
    QApplication::setPalette(makePalette(current()));
    qApp->setStyleSheet(makeStyleSheet(current()));
}

const ThemeColors& theme() {
    return current();
}

ThemeMode loadThemeMode() {
    const int value = QSettings().value(kThemeKey, static_cast<int>(ThemeMode::Light)).toInt();
    if (value == static_cast<int>(ThemeMode::Dark)) return ThemeMode::Dark;
    if (value == static_cast<int>(ThemeMode::System)) return ThemeMode::System;
    return ThemeMode::Light;
}

void saveThemeMode(ThemeMode mode) {
    QSettings().setValue(kThemeKey, static_cast<int>(mode));
}

QIcon makeIcon(IconKind kind, const QColor& color) {
    // Disegno su una griglia di 32×32 punti, a doppia risoluzione per gli schermi ad alta densità.
    QPixmap pixmap(64, 64);
    pixmap.fill(Qt::transparent);
    QPainter p(&pixmap);
    p.setRenderHint(QPainter::Antialiasing);
    p.scale(2.0, 2.0);
    QPen pen(color, 2.2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);

    auto tray = [&p] {
        QPainterPath path;
        path.moveTo(6, 19);
        path.lineTo(6, 26);
        path.lineTo(26, 26);
        path.lineTo(26, 19);
        p.drawPath(path);
    };

    switch (kind) {
        case IconKind::Add:
            p.drawEllipse(QPointF(16, 16), 11, 11);
            p.drawLine(QPointF(16, 10), QPointF(16, 22));
            p.drawLine(QPointF(10, 16), QPointF(22, 16));
            break;
        case IconKind::Edit: {
            p.save();
            p.translate(16, 16);
            p.rotate(45);
            p.drawRoundedRect(QRectF(-3.5, -13, 7, 19), 1.5, 1.5);
            QPainterPath tip;
            tip.moveTo(-3.5, 6);
            tip.lineTo(0, 12);
            tip.lineTo(3.5, 6);
            p.drawPath(tip);
            p.drawLine(QPointF(-3.5, -8.5), QPointF(3.5, -8.5));
            p.restore();
            break;
        }
        case IconKind::Duplicate:
            p.drawRoundedRect(QRectF(5, 10, 14, 17), 2.5, 2.5);
            p.setBrush(Qt::NoBrush);
            p.drawRoundedRect(QRectF(13, 5, 14, 17), 2.5, 2.5);
            break;
        case IconKind::Delete: {
            p.drawLine(QPointF(6, 9), QPointF(26, 9));
            p.drawRoundedRect(QRectF(12.5, 5, 7, 4), 1, 1);
            QPainterPath body;
            body.moveTo(8.5, 9);
            body.lineTo(10, 27);
            body.lineTo(22, 27);
            body.lineTo(23.5, 9);
            p.drawPath(body);
            p.drawLine(QPointF(14, 14), QPointF(14, 22));
            p.drawLine(QPointF(18, 14), QPointF(18, 22));
            break;
        }
        case IconKind::Withdraw:
            tray();
            p.drawLine(QPointF(16, 21), QPointF(16, 5));
            p.drawLine(QPointF(11, 10), QPointF(16, 5));
            p.drawLine(QPointF(21, 10), QPointF(16, 5));
            break;
        case IconKind::Restock:
            tray();
            p.drawLine(QPointF(16, 4), QPointF(16, 20));
            p.drawLine(QPointF(11, 15), QPointF(16, 20));
            p.drawLine(QPointF(21, 15), QPointF(16, 20));
            break;
        case IconKind::Reorder: {
            QPainterPath triangle;
            triangle.moveTo(16, 4.5);
            triangle.lineTo(28.5, 26.5);
            triangle.lineTo(3.5, 26.5);
            triangle.closeSubpath();
            p.drawPath(triangle);
            p.drawLine(QPointF(16, 12), QPointF(16, 19));
            p.drawPoint(QPointF(16, 23));
            break;
        }
        case IconKind::Statistics:
            p.drawLine(QPointF(5, 27), QPointF(27, 27));
            p.drawRoundedRect(QRectF(7, 16, 4.5, 8), 1, 1);
            p.drawRoundedRect(QRectF(13.75, 7, 4.5, 17), 1, 1);
            p.drawRoundedRect(QRectF(20.5, 12, 4.5, 12), 1, 1);
            break;
        case IconKind::ColorCode: {
            p.drawLine(QPointF(2, 16), QPointF(30, 16));
            p.setBrush(QColor(228, 205, 160));
            p.setPen(QPen(color, 1.6));
            p.drawRoundedRect(QRectF(7, 9.5, 18, 13), 4, 4);
            p.setPen(Qt::NoPen);
            const QColor bands[] = {QColor(250, 215, 30), QColor(140, 60, 200), QColor(215, 30, 30)};
            for (int i = 0; i < 3; ++i) {
                p.setBrush(bands[i]);
                p.drawRect(QRectF(10.5 + i * 3.6, 10.3, 2.2, 11.4));
            }
            break;
        }
        case IconKind::Search:
            p.drawEllipse(QPointF(13.5, 13.5), 7.5, 7.5);
            p.drawLine(QPointF(19, 19), QPointF(27, 27));
            break;
    }
    p.end();
    pixmap.setDevicePixelRatio(2.0);
    return QIcon(pixmap);
}

QColor categoryColor(Category category) {
    switch (category) {
        case Category::Resistor: return QColor("#c58b4c");
        case Category::Capacitor: return QColor("#2f80ed");
        case Category::Inductor: return QColor("#8e5cd9");
        case Category::Diode: return QColor("#7d8a9c");
        case Category::Led: return QColor("#e5484d");
        case Category::Transistor: return QColor("#2fa36b");
        case Category::IntegratedCircuit: return QColor("#f08c2e");
        case Category::Connector: return QColor("#19b5c9");
        case Category::Electromechanical: return QColor("#d6519b");
        case Category::Sensor: return QColor("#a3b81c");
        case Category::Module: return QColor("#5b6ee1");
        case Category::Other: return QColor("#9aa0a6");
    }
    return QColor("#9aa0a6");
}

QIcon categoryIcon(Category category) {
    QPixmap pixmap(32, 32);
    pixmap.fill(Qt::transparent);
    QPainter p(&pixmap);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);
    p.setBrush(categoryColor(category));
    p.drawEllipse(QRectF(6, 6, 20, 20));
    p.end();
    pixmap.setDevicePixelRatio(2.0);
    return QIcon(pixmap);
}

QString pillStyle(const QColor& background, const QColor& foreground) {
    return QStringLiteral("background: %1; color: %2; border-radius: 9px; padding: 2px 9px; font-weight: 600;")
        .arg(background.name(), foreground.name());
}
