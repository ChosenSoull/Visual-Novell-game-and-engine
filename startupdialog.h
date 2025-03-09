#ifndef STARTUPDIALOG_H
#define STARTUPDIALOG_H

#include <QDialog>
#include <QListWidget>
#include "newprojectdialog.h"

class StartupDialog : public QDialog {
    Q_OBJECT
private:
    QListWidget* recentProjectsList;
    std::vector<ProjectInfo> recentProjects;

public:
    StartupDialog(QWidget* parent = nullptr);
    ProjectInfo getSelectedProject() const;

private:
    void loadRecentProjects();
    void saveRecentProjects(const ProjectInfo& project);

private slots:
    void newProject();
    void openProject();
    void openRecentProject();
};

#endif // STARTUPDIALOG_H