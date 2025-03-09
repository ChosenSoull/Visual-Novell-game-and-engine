#include <QApplication>
#include "mainwindow.h"
#include "startupdialog.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    StartupDialog startup;
    if (startup.exec() == QDialog::Accepted) {
        ProjectInfo project = startup.getSelectedProject();
        if (project.name.empty()) return 0;
        MainWindow window(project);
        window.show();
        return app.exec();
    }
    return 0;
}