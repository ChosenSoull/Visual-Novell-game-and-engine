#include "startupdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>
#include <QSettings>
#include <fstream>
#include <sstream>

StartupDialog::StartupDialog(QWidget* parent) : QDialog(parent) {
    QHBoxLayout* layout = new QHBoxLayout(this);
    recentProjectsList = new QListWidget(this);
    loadRecentProjects();
    layout->addWidget(recentProjectsList);

    QVBoxLayout* buttonLayout = new QVBoxLayout();
    QPushButton* newProjectButton = new QPushButton("Create Project", this);
    QPushButton* openProjectButton = new QPushButton("Open Project", this);
    buttonLayout->addWidget(newProjectButton);
    buttonLayout->addWidget(openProjectButton);
    buttonLayout->addStretch();
    layout->addLayout(buttonLayout);

    connect(newProjectButton, &QPushButton::clicked, this, &StartupDialog::newProject);
    connect(openProjectButton, &QPushButton::clicked, this, &StartupDialog::openProject);
    connect(recentProjectsList, &QListWidget::itemDoubleClicked, this, &StartupDialog::openRecentProject);

    setWindowTitle("Phantom Game Engine");
    resize(600, 400);
}

ProjectInfo StartupDialog::getSelectedProject() const {
    int row = recentProjectsList->currentRow();
    if (row >= 0 && static_cast<size_t>(row) < recentProjects.size()) {
        return recentProjects[row];
    }
    return ProjectInfo();
}

void StartupDialog::loadRecentProjects() {
    QSettings settings("MyCompany", "PhantomGameEngine");
    QStringList projects = settings.value("recentProjects").toStringList();
    recentProjects.clear();
    recentProjectsList->clear();
    for (const QString& project : projects) {
        std::ifstream file(project.toStdString() + "/config.cfg");
        if (file.is_open()) {
            ProjectInfo info;
            std::string line;
            while (std::getline(file, line)) {
                if (line.find("name=") == 0) info.name = line.substr(5);
                else if (line.find("path=") == 0) info.path = line.substr(5);
                else if (line.find("description=") == 0) info.description = line.substr(12);
                else if (line.find("renderApi=") == 0) info.renderApi = line.substr(10);
                else if (line.find("type=") == 0) info.type = line.substr(5);
                else if (line.find("standartmodules=") == 0) {
                    std::string mods = line.substr(15);
                    std::istringstream iss(mods);
                    std::string mod;
                    while (std::getline(iss, mod, ',')) info.standardModules.push_back(mod);
                } else if (line.find("projectmodules=") == 0) {
                    std::string mods = line.substr(14);
                    std::istringstream iss(mods);
                    std::string mod;
                    while (std::getline(iss, mod, ',')) info.projectModules.push_back(mod);
                }
            }
            recentProjects.push_back(info);
            recentProjectsList->addItem(QString("%1\n%2\n%3").arg(info.name.c_str()).arg(info.description.c_str()).arg(info.path.c_str()));
            file.close();
        }
    }
}

void StartupDialog::saveRecentProjects(const ProjectInfo& project) {
    QSettings settings("MyCompany", "PhantomGameEngine");
    QStringList projects = settings.value("recentProjects").toStringList();
    QString projectPath = QString::fromStdString(project.path);
    if (!projects.contains(projectPath)) {
        projects.prepend(projectPath);
        if (projects.size() > 5) projects.removeLast();
        settings.setValue("recentProjects", projects);
    }

    std::ofstream file(project.path + "/config.cfg");
    if (file.is_open()) {
        file << "name=" << project.name << "\n";
        file << "path=" << project.path << "\n";
        file << "description=" << project.description << "\n";
        file << "renderApi=" << project.renderApi << "\n";
        file << "type=" << project.type << "\n";
        file << "standartmodules=" << (project.standardModules.empty() ? "" : std::accumulate(project.standardModules.begin() + 1, project.standardModules.end(), project.standardModules[0], [](const std::string& a, const std::string& b) { return a + "," + b; })) << "\n";
        file << "projectmodules=" << (project.projectModules.empty() ? "" : std::accumulate(project.projectModules.begin() + 1, project.projectModules.end(), project.projectModules[0], [](const std::string& a, const std::string& b) { return a + "," + b; })) << "\n";
        file.close();
    }
}

void StartupDialog::newProject() {
    NewProjectDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        ProjectInfo info = dialog.getProjectInfo();
        if (info.name.empty() || info.path.empty()) {
            QMessageBox::warning(this, "Error", "Name and path are required!");
            return;
        }
        QDir dir(QString::fromStdString(info.path));
        if (!dir.exists()) dir.mkpath(".");
        saveRecentProjects(info);
        accept();
    }
}

void StartupDialog::openProject() {
    QString dir = QFileDialog::getExistingDirectory(this, "Open Project Directory");
    if (!dir.isEmpty()) {
        std::ifstream file(dir.toStdString() + "/config.cfg");
        if (file.is_open()) {
            ProjectInfo info;
            std::string line;
            while (std::getline(file, line)) {
                if (line.find("name=") == 0) info.name = line.substr(5);
                else if (line.find("path=") == 0) info.path = line.substr(5);
                else if (line.find("description=") == 0) info.description = line.substr(12);
                else if (line.find("renderApi=") == 0) info.renderApi = line.substr(10);
                else if (line.find("type=") == 0) info.type = line.substr(5);
                else if (line.find("standartmodules=") == 0) {
                    std::string mods = line.substr(15);
                    std::istringstream iss(mods);
                    std::string mod;
                    while (std::getline(iss, mod, ',')) info.standardModules.push_back(mod);
                } else if (line.find("projectmodules=") == 0) {
                    std::string mods = line.substr(14);
                    std::istringstream iss(mods);
                    std::string mod;
                    while (std::getline(iss, mod, ',')) info.projectModules.push_back(mod);
                }
            }
            file.close();
            saveRecentProjects(info);
            accept();
        } else {
            QMessageBox::warning(this, "Error", "No config.cfg found!");
        }
    }
}

void StartupDialog::openRecentProject() {
    if (recentProjectsList->currentRow() >= 0) accept();
}