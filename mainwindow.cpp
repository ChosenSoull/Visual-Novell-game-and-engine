#include "mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QAction>
#include <QMenu>
#include <QDockWidget>
#include <QTextEdit>
#include <QListWidget>
#include <QTreeWidget>
#include <QTableWidget>
#include <QFileSystemModel>
#include <QSettings>
#include <QProcess>
#include <QDesktopServices>

MainWindow::MainWindow(const ProjectInfo& project, QWidget* parent) : QMainWindow(parent), currentProject(project) {
    engine = new VisualNovelEngine(toProjectConfig());
    mdiArea = new QMdiArea(this);
    setCentralWidget(mdiArea);

    setupMenuBar();
    setupDockWidgets();
    statusBar = new QStatusBar(this);
    setStatusBar(statusBar);
    statusBar->showMessage("Ready");

    apiLabel = new QLabel("Current API: " + QString::fromStdString(currentProject.renderApi), this);
    statusBar->addWidget(apiLabel);

    setWindowTitle(QString("Phantom Game Engine - %1").arg(project.name.c_str()));
    resize(1200, 800);
}

MainWindow::~MainWindow() {
    delete engine;
}

void MainWindow::setupMenuBar() {
    QMenuBar* menuBar = new QMenuBar(this);

    // File Menu
    QMenu* fileMenu = menuBar->addMenu("File");
    fileMenu->addAction("New Project", this, &MainWindow::newProject);
    fileMenu->addAction("Open Project", this, &MainWindow::openProject);
    fileMenu->addAction("Save", this, &MainWindow::saveProject);
    fileMenu->addAction("Save As...", this, &MainWindow::saveAsProject);
    QMenu* importMenu = fileMenu->addMenu("Import");
    importMenu->addAction("Import Model");
    importMenu->addAction("Import Texture");
    QMenu* exportMenu = fileMenu->addMenu("Export");
    exportMenu->addAction("Export Model");
    fileMenu->addAction("Project Settings", this, &MainWindow::projectSettings);
    fileMenu->addAction("Engine Settings", this, &MainWindow::engineSettings);
    fileMenu->addAction("Exit", this, &MainWindow::exit);

    // Edit Menu
    QMenu* editMenu = menuBar->addMenu("Edit");
    editMenu->addAction("Undo", this, &MainWindow::undo);
    editMenu->addAction("Redo", this, &MainWindow::redo);
    editMenu->addAction("Cut", this, &MainWindow::cut);
    editMenu->addAction("Copy", this, &MainWindow::copy);
    editMenu->addAction("Paste", this, &MainWindow::paste);
    editMenu->addAction("Delete", this, &MainWindow::deleteSelection);
    editMenu->addAction("Select All", this, &MainWindow::selectAll);
    editMenu->addAction("Deselect All", this, &MainWindow::deselectAll);
    editMenu->addAction("Duplicate", this, &MainWindow::duplicate);
    editMenu->addAction("Group", this, &MainWindow::group);
    editMenu->addAction("Ungroup", this, &MainWindow::ungroup);

    // View Menu
    QMenu* viewMenu = menuBar->addMenu("View");
    QMenu* layoutMenu = viewMenu->addMenu("Layouts");
    layoutMenu->addAction("Default");
    layoutMenu->addAction("Modeling");
    layoutMenu->addAction("Level Design");
    QMenu* panelMenu = viewMenu->addMenu("Show/Hide Panels");
    panelMenu->addAction("Scene", this, [this]() { togglePanel("Scene"); });
    panelMenu->addAction("Hierarchy", this, [this]() { togglePanel("Hierarchy"); });
    viewMenu->addAction("Full Screen");

    // Modules Menu
    QMenu* modulesMenu = menuBar->addMenu("Modules");
    modulesMenu->addAction("Manage Modules", this, &MainWindow::manageModules);
    modulesMenu->addAction("Add Module", this, &MainWindow::addModule);
    modulesMenu->addAction("List Modules", this, &MainWindow::listModules);
    modulesMenu->addAction("Module Settings", this, &MainWindow::moduleSettings);

    // Tools Menu
    QMenu* toolsMenu = menuBar->addMenu("Tools");
    toolsMenu->addAction("Landscape Editor", this, &MainWindow::openLandscapeEditor);
    toolsMenu->addAction("Material Editor", this, &MainWindow::openMaterialEditor);
    toolsMenu->addAction("Animation Editor", this, &MainWindow::openAnimationEditor);
    toolsMenu->addAction("Particle Editor", this, &MainWindow::openParticleEditor);
    toolsMenu->addAction("Debugger", this, &MainWindow::openDebugger);
    toolsMenu->addAction("Profiler", this, &MainWindow::openProfiler);
    toolsMenu->addAction("Console", this, &MainWindow::openConsole);
    toolsMenu->addAction("Code Editor", this, &MainWindow::openCodeEditor);

    // Window Menu
    QMenu* windowMenu = menuBar->addMenu("Window");
    windowMenu->addAction("New Window", this, &MainWindow::newWindow);
    windowMenu->addAction("Reset Window Layout", this, &MainWindow::resetLayout);

    // Help Menu
    QMenu* helpMenu = menuBar->addMenu("Help");
    helpMenu->addAction("Help", this, &MainWindow::showHelp);
    helpMenu->addAction("Tutorials", this, &MainWindow::showTutorials);
    helpMenu->addAction("About Phantom Engine", this, &MainWindow::showAbout);
    helpMenu->addAction("Online Community", this, &MainWindow::showCommunity);

    setMenuBar(menuBar);
}

void MainWindow::setupDockWidgets() {
    // Scene View
    sceneDock = new QDockWidget("Scene", this);
    sceneDock->setWidget(new QTextEdit("Scene Viewport"));
    addDockWidget(Qt::LeftDockWidgetArea, sceneDock);

    // Hierarchy
    hierarchyDock = new QDockWidget("Hierarchy", this);
    hierarchyDock->setWidget(new QTreeWidget());
    addDockWidget(Qt::LeftDockWidgetArea, hierarchyDock);

    // Inspector
    inspectorDock = new QDockWidget("Inspector", this);
    inspectorDock->setWidget(new QTableWidget());
    addDockWidget(Qt::RightDockWidgetArea, inspectorDock);

    // Asset Browser
    assetDock = new QDockWidget("Assets", this);
    assetDock->setWidget(new QListWidget());
    addDockWidget(Qt::LeftDockWidgetArea, assetDock);

    // Console
    consoleDock = new QDockWidget("Console", this);
    consoleDock->setWidget(new QTextEdit());
    addDockWidget(Qt::BottomDockWidgetArea, consoleDock);

    // Modules
    modulesDock = new QDockWidget("Modules", this);
    modulesDock->setWidget(new QListWidget());
    addDockWidget(Qt::RightDockWidgetArea, modulesDock);

    // Toolbar
    toolbarDock = new QDockWidget("Toolbar", this);
    toolbarDock->setWidget(new QToolBar());
    addDockWidget(Qt::TopDockWidgetArea, toolbarDock);

    foreach (QDockWidget* dock, findChildren<QDockWidget*>()) {
        dock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetClosable);
    }
}

ProjectConfig MainWindow::toProjectConfig() const {
    ProjectConfig config;
    config.path = currentProject.path;
    config.renderApi = currentProject.renderApi;
    config.libraries.insert(config.libraries.end(), currentProject.standardModules.begin(), currentProject.standardModules.end());
    config.libraries.insert(config.libraries.end(), currentProject.projectModules.begin(), currentProject.projectModules.end());
    return config;
}

// Slot implementations
void MainWindow::newProject() {
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
        currentProject = info;
        delete engine;
        engine = new VisualNovelEngine(toProjectConfig());
        setWindowTitle(QString("Phantom Game Engine - %1").arg(info.name.c_str()));
        apiLabel->setText("Current API: " + QString::fromStdString(info.renderApi));
    }
}

void MainWindow::openProject() {
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
            currentProject = info;
            delete engine;
            engine = new VisualNovelEngine(toProjectConfig());
            setWindowTitle(QString("Phantom Game Engine - %1").arg(info.name.c_str()));
            apiLabel->setText("Current API: " + QString::fromStdString(info.renderApi));
        } else {
            QMessageBox::warning(this, "Error", "No config.cfg found!");
        }
    }
}

void MainWindow::saveProject() {
    std::ofstream file(currentProject.path + "/config.cfg");
    if (file.is_open()) {
        file << "name=" << currentProject.name << "\n";
        file << "path=" << currentProject.path << "\n";
        file << "description=" << currentProject.description << "\n";
        file << "renderApi=" << currentProject.renderApi << "\n";
        file << "type=" << currentProject.type << "\n";
        file << "standartmodules=" << (currentProject.standardModules.empty() ? "" : std::accumulate(currentProject.standardModules.begin() + 1, currentProject.standardModules.end(), currentProject.standardModules[0], [](const std::string& a, const std::string& b) { return a + "," + b; })) << "\n";
        file << "projectmodules=" << (currentProject.projectModules.empty() ? "" : std::accumulate(currentProject.projectModules.begin() + 1, currentProject.projectModules.end(), currentProject.projectModules[0], [](const std::string& a, const std::string& b) { return a + "," + b; })) << "\n";
        file.close();
        statusBar->showMessage("Project Saved");
    }
}

void MainWindow::saveAsProject() {
    QString dir = QFileDialog::getExistingDirectory(this, "Save Project As");
    if (!dir.isEmpty()) {
        currentProject.path = dir.toStdString();
        saveProject();
        setWindowTitle(QString("Phantom Game Engine - %1").arg(currentProject.name.c_str()));
    }
}

void MainWindow::importResource() { QFileDialog::getOpenFileName(this, "Import Resource"); }
void MainWindow::exportResource() { QFileDialog::getSaveFileName(this, "Export Resource"); }
void MainWindow::projectSettings() { QMessageBox::information(this, "Settings", "Project Settings"); }
void MainWindow::engineSettings() { QMessageBox::information(this, "Settings", "Engine Settings"); }
void MainWindow::exit() { close(); }
void MainWindow::undo() { statusBar->showMessage("Undo"); }
void MainWindow::redo() { statusBar->showMessage("Redo"); }
void MainWindow::cut() { statusBar->showMessage("Cut"); }
void MainWindow::copy() { statusBar->showMessage("Copy"); }
void MainWindow::paste() { statusBar->showMessage("Paste"); }
void MainWindow::deleteSelection() { statusBar->showMessage("Delete"); }
void MainWindow::selectAll() { statusBar->showMessage("Select All"); }
void MainWindow::deselectAll() { statusBar->showMessage("Deselect All"); }
void MainWindow::duplicate() { statusBar->showMessage("Duplicate"); }
void MainWindow::group() { statusBar->showMessage("Group"); }
void MainWindow::ungroup() { statusBar->showMessage("Ungroup"); }
void MainWindow::showLayoutMenu() { /* Implement */ }
void MainWindow::togglePanel(const QString& panelName) {
    QDockWidget* dock = findChild<QDockWidget*>(panelName);
    if (dock) dock->setVisible(!dock->isVisible());
}
void MainWindow::manageModules() { QMessageBox::information(this, "Modules", "Manage Modules"); }
void MainWindow::addModule() { QFileDialog::getOpenFileName(this, "Add Module"); }
void MainWindow::listModules() { /* Implement */ }
void MainWindow::moduleSettings() { QMessageBox::information(this, "Settings", "Module Settings"); }
void MainWindow::openLandscapeEditor() { /* Implement */ }
void MainWindow::openMaterialEditor() { /* Implement */ }
void MainWindow::openAnimationEditor() { /* Implement */ }
void MainWindow::openParticleEditor() { /* Implement */ }
void MainWindow::openDebugger() { /* Implement */ }
void MainWindow::openProfiler() { /* Implement */ }
void MainWindow::openConsole() { consoleDock->show(); }
void MainWindow::newWindow() { MainWindow* newWin = new MainWindow(currentProject); newWin->show(); }
void MainWindow::resetLayout() { /* Implement */ }
void MainWindow::showHelp() { QMessageBox::information(this, "Help", "Help Content"); }
void MainWindow::showTutorials() { QMessageBox::information(this, "Tutorials", "Tutorial Links"); }
void MainWindow::showAbout() { QMessageBox::about(this, "About", "Phantom Game Engine v1.0"); }
void MainWindow::showCommunity() { QDesktopServices::openUrl(QUrl("https://community.example.com")); }
void MainWindow::switchToOpenGL() {
    currentProject.renderApi = "opengl";
    apiLabel->setText("Current API: OpenGL");
    delete engine;
    engine = new VisualNovelEngine(toProjectConfig());
}
void MainWindow::switchToVulkan() {
    currentProject.renderApi = "vulkan";
    apiLabel->setText("Current API: Vulkan");
    delete engine;
    engine = new VisualNovelEngine(toProjectConfig());
}
void MainWindow::buildProject() { QMessageBox::information(this, "Build", "Building project... (Placeholder)"); }
void MainWindow::openCodeEditor() {
    QSettings settings("MyCompany", "PhantomGameEngine");
    QString editor = settings.value("editor", "code").toString();
    QString mainCpp = QString::fromStdString(currentProject.path + "/main.cpp");
    if (!QFile::exists(mainCpp)) {
        QFile file(mainCpp);
        if (file.open(QIODevice::WriteOnly)) {
            std::string code = "#include \"VisualNovelEngine.h\"\n\nint main() {\n    ProjectConfig config = loadProjectConfig(\"" + currentProject.path + "\");\n    VisualNovelEngine engine(config);\n    engine.start();\n    return 0;\n}";
            file.write(QByteArray::fromStdString(code));
            file.close();
        }
    }
    QStringList args = editor.toLower() == "vim" || editor.toLower() == "nano" ? QStringList{mainCpp} : QStringList{"-r", mainCpp};
    QProcess::startDetached(editor.toLower(), args);
}
void MainWindow::manageLibraries() {
    NewProjectDialog dialog(this);
    dialog.setWindowTitle("Manage Libraries");
    if (dialog.exec() == QDialog::Accepted) {
        ProjectInfo newInfo = dialog.getProjectInfo();
        currentProject.standardModules = newInfo.standardModules;
        currentProject.projectModules = newInfo.projectModules;
        saveProject();
        modulesDock->widget()->deleteLater();
        QListWidget* moduleList = new QListWidget(this);
        for (const auto& lib : currentProject.standardModules) moduleList->addItem(QString("Standard: %1").arg(lib.c_str()));
        for (const auto& lib : currentProject.projectModules) moduleList->addItem(QString("Project: %1").arg(lib.c_str()));
        modulesDock->setWidget(moduleList);
        delete engine;
        engine = new VisualNovelEngine(toProjectConfig());
    }
}

void MainWindow::saveRecentProjects(const ProjectInfo& project) {
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