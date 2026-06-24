#include "ui/QtMainWindow.h"

#ifdef J2534_ENABLE_QT

#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>

namespace ui {

QtMainWindow::QtMainWindow(QWidget* parent) : QMainWindow(parent) {
    auto* root = new QWidget(this);
    auto* layout = new QVBoxLayout(root);
    auto* buttonRow = new QHBoxLayout();

    connectButton_ = new QPushButton("Connect", root);
    disconnectButton_ = new QPushButton("Disconnect", root);
    logView_ = new QPlainTextEdit(root);
    logView_->setReadOnly(true);
    logView_->appendPlainText("J2534 diagnostic UI ready. Bind DiagnosticController here.");

    buttonRow->addWidget(new QLabel("Device: Mock / Vendor DLL", root));
    buttonRow->addWidget(connectButton_);
    buttonRow->addWidget(disconnectButton_);

    layout->addLayout(buttonRow);
    layout->addWidget(logView_);
    setCentralWidget(root);
    setWindowTitle("J2534 Diagnostic Tool");
}

}  // namespace ui

#endif
