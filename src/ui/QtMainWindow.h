#pragma once

#ifdef J2534_ENABLE_QT

#include <QMainWindow>
#include <QPlainTextEdit>
#include <QPushButton>

namespace ui {

class QtMainWindow final : public QMainWindow {
    Q_OBJECT
public:
    explicit QtMainWindow(QWidget* parent = nullptr);

private:
    QPushButton* connectButton_{};
    QPushButton* disconnectButton_{};
    QPlainTextEdit* logView_{};
};

}  // namespace ui

#endif
