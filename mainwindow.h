#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMdiArea>
#include <QMenuBar>
#include <QDockWidget>
#include <QTextEdit>
#include <QListWidget>
#include <QTreeWidget>
#include <QTableWidget>
#include "VisualNovelEngine.h"

struct ProjectInfo {
    std::string name;
    std::string path;
    std::string description;
    std::string renderApi; // "opengl" или "vulkan"
    std::string type;      // "2d" или "3d"
    std::vector<std::string> standardModules;
    std::vector<std::string> projectModules;
};

struct ProjectConfig {
    std::string path;
    std::string renderApi;
    std::vector<std::string> libraries;
};

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(const ProjectInfo& project, QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void newProject();
    void openProject();
    void saveProject();
    void saveAsProject();
    void importResource();
    void exportResource();
    void projectSettings();
    void engineSettings();
    void exit();
    void undo();
    void redo();
    void cut();
    void copy();
    void paste();
    void deleteSelection();
    void selectAll();
    void deselectAll();
    void duplicate();
    void group();
    void ungroup();
    void showLayoutMenu();
    void togglePanel(const QString& panelName);
    void manageModules();
    void addModule();
    void listModules();
    void moduleSettings();
    void openLandscapeEditor();
    void openMaterialEditor();
    void openAnimationEditor();
    void openParticleEditor();
    void openDebugger();
    void openProfiler();
    void openConsole();
    void newWindow();
    void resetLayout();
    void showHelp();
    void showTutorials();
    void showAbout();
    void showCommunity();
    void switchToOpenGL();
    void switchToVulkan();
    void buildProject();
    void openCodeEditor();
    void manageLibraries();

private:
    void setupMenuBar();
    void setupDockWidgets();
    ProjectConfig toProjectConfig() const;
    VisualNovelEngine* engine;
    ProjectInfo currentProject;
    QMdiArea* mdiArea;
    QDockWidget* sceneDock;
    QDockWidget* hierarchyDock;
    QDockWidget* inspectorDock;
    QDockWidget* assetDock;
    QDockWidget* consoleDock;
    QDockWidget* modulesDock;
    QDockWidget* toolbarDock;
    QStatusBar* statusBar;
    QLabel* apiLabel;
};

#endif // MAINWINDOW_H