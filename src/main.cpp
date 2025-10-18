/*
===============================================================================================================
- Expected Functionality
- This plugin is expected to export a MOF/MAF file
- For accurate results, export the file with no deduplication first to see the real vertex count. and compare it
    to the dedplucate version
===============================================================================================================
*/

#include <maya/MFnPlugin.h>                                                             // Needed to register the plugin [ InitializePlugin and UnitializePlugin ]
#include <maya/MPxCommand.h>                                                            // Used for creating custom commands
#include <maya/MQtUtil.h>                                                               // Utilities for the QT and Maya API like mainWindow() which returns the main Maya window 
#include <maya/MGlobal.h>                                                               // Utilities for the QT and Maya API like mainWindow() which returns the main Maya window 
#include <maya/MSelectionList.h>

#include <QtWidgets/QDialog>                                                            // ] = = = = = = = = = = = = = = = = = = = = = = = [ 
#include <QtWidgets/QFileDialog>                                                        // |                                               |
#include <QtWidgets/QPushButton>                                                        // | Base QT includes                              | 
#include <QtWidgets/qlabel.h>                                                           // |                                               |    
#include <QtWidgets/qcombobox.h>                                                        // |                                               |
#include <QtWidgets/QVBoxLayout>                                                        // |                                               | 
#include <QtWidgets/QWidget>                                                            // ] = = = = = = = = = = = = = = = = = = = = = = = [ 
#include <QtWidgets/qcheckbox.h>
#include <QtWidgets/qtabwidget.h>
#include <QtWidgets/qtabbar.h>

#include <vector>
#include <string>

#include "MOF_Generator.h"
#include "MAF_Generator.h"


static std::vector<std::string> Commands()
{
    static std::vector<std::string> commands =
    {
        "OPEN"
    };

    return commands;
}

struct CustomWindow : public QDialog
{
    CustomWindow(QWidget* parent = nullptr) : QDialog(parent)
    {
        setWindowTitle("Midnight File Exporter");
        QIcon* icon = new QIcon("C:/ScriptsMAYA/cpp/MOF_Plugin/MOF_Exporter/resources/icon6.png");
        setWindowIcon(*icon);
        setFixedSize(360, 180); // slightly larger for tabs

        // --- Main layout ---
        QVBoxLayout* mainLayout = new QVBoxLayout(this);

        // --- Tab widget ---
        QTabWidget* tabWidget = new QTabWidget(this);
        mainLayout->addWidget(tabWidget);

        // ========================
        // STATIC TAB
        // ========================
        QWidget* staticTab = new QWidget();
        tabWidget->addTab(staticTab, "Mesh .mof");

        QVBoxLayout* staticLayout = new QVBoxLayout(staticTab);
        QHBoxLayout* hLayout      = new QHBoxLayout();
        QLabel* dropLabel         = new QLabel("File Format:", this);
        QFont labelFont           = dropLabel->font();

        labelFont.setBold(true);
        dropLabel->setFont(labelFont);

        QComboBox* dropdown = new QComboBox(this);
        dropdown->addItem("Binary");
        dropdown->addItem("Ascii");

        hLayout->addWidget(dropLabel);
        hLayout->addWidget(dropdown);
        staticLayout->addLayout(hLayout);

        QCheckBox* checkBox = new QCheckBox("Deduplicate Vertices");     
        staticLayout->addWidget(checkBox, 0, Qt::AlignLeft);

        QPushButton* button = new QPushButton("Export Selected", this);
        button->setToolTip("Select the model you want to export - This exporter detects if the model has any influences attach to it and generates \nthe MOF accordingly [From a 44 bytes vertex stride for static models up to 76 bytes for animated ones]");
        staticLayout->addWidget(button);

        // --- Connect export button ---
        connect(button, &QPushButton::clicked, this,
            [=, this]()
            {
                MSelectionList sel;
                MGlobal::getActiveSelectionList(sel);
                if (sel.isEmpty())
                {
                    MGlobal::displayWarning("No objects selected.");
                    return;
                }

                QString choice = dropdown->currentText();
                QString filter = (choice == "Binary") ? "Binary Files (*.mof)" : "ASCII Files (*.mof)";
                QString filePath = QFileDialog::getSaveFileName(this, "Export Midnight Object File", "", filter);

                if (!filePath.isEmpty())
                {
                    std::string path   = filePath.toUtf8().constData();
                    std::string format = choice.toUtf8().constData();
                    MOF_Generator::ExportMesh(path, format, checkBox->isChecked());                    
                }
            }
        );

        // ========================
        // ANIMATED TAB (empty)
        // ========================
        QWidget* animatedTab = new QWidget();
        tabWidget->addTab(animatedTab, "Animation .maf");
        QVBoxLayout* animVertLayout    = new QVBoxLayout(animatedTab);
        QHBoxLayout* animDropHorLayout = new QHBoxLayout(animatedTab);
        animVertLayout->addLayout(animDropHorLayout);

        QLabel* animDropLabel = new QLabel("File Format:", this);
        QFont animLabelFont = dropLabel->font();
        animLabelFont.setBold(true);
        animDropLabel->setFont(labelFont);        

        QComboBox* animDropdown = new QComboBox(this);
        animDropdown->addItem("Binary");
        animDropdown->addItem("Ascii");

        animDropHorLayout->addWidget(animDropLabel);
        animDropHorLayout->addWidget(animDropdown);

        QCheckBox* animCheckBox = new QCheckBox("Deduplicate Keyframes");
        animVertLayout->addWidget(animCheckBox, 0, Qt::AlignLeft);

        QPushButton* exportMafButton = new QPushButton("Export Selected", this);
        exportMafButton->setToolTip("MAF file exporter. This file retrieves Joints hierarchy, joint Id's and keyframes data");
        animVertLayout->addWidget(exportMafButton);


        connect(exportMafButton, &QPushButton::clicked, this,
            [=, this]()
            {
                MSelectionList sel;
                MGlobal::getActiveSelectionList(sel);
                if (sel.isEmpty())
                {
                    MGlobal::displayWarning("No animated meshes selected [Please select a mesh containing an animation]");
                    return;
                }

                QString choice = animDropdown->currentText();
                QString filter = (choice == "Binary") ? "Binary Files (*.maf)" : "ASCII Files (*.maf)";
                QString filePath = QFileDialog::getSaveFileName(this, "Export Midnight Object File", "", filter);

                if (!filePath.isEmpty())
                {
                    std::string path = filePath.toUtf8().constData();
                    std::string format = choice.toUtf8().constData();
                    MAF_Generator::ExportAnimation(path, format, animCheckBox->isChecked());
                }
            });

    }
};


struct ShowWindowCmd : public MPxCommand                                                // Struct that inherits from MPxCommand. This struct implements the commnad Maya will use to show the 'Dialog'
{
    static void* creator() { return new ShowWindowCmd; }                                // Returns a new instance of the command. Maya's API expects a new 'creator' function to be passed when the command is registered
    
    MStatus doIt(const MArgList& args) override                                         // Overrides the doIt method from MPxCommand. This is the function Maya will call when the user executes the command 'MOF_EXP'
    {
        QWidget*      mayaMainWindow = (QWidget*)MQtUtil::mainWindow();                 // Retrieves a pointer to the main maya window and casts it to a QWidget
        CustomWindow* cWndw          = new CustomWindow(mayaMainWindow);                // Creates a new instance of CustomWindow

        cWndw->setAttribute(Qt::WA_DeleteOnClose);                                      // Sets cWndw with the attribute WA_DeleteOnClose, which means that it frees cWndw memory when the window is closed
        cWndw->show();                                                                  // It shows the window
        return MS::kSuccess;                                                            // Process completed succesfully
    }
};

MStatus initializePlugin(MObject obj)                                                   // Mandatory function that maya calls when the plugin is loaded
{   
    MFnPlugin plugin(obj, "Midnight_Polygons", "1.0", "Any");                           // It creates a helper MFnPlugin object with the obj that maya provides the function when it calls it. We also provide some metadata [vendor, plug-in version, required Maya Version]
     
    for (std::string cmnd : Commands())
    {
        MString tmpCmd{ &cmnd[0], (int)cmnd.length() };
        MStatus status = plugin.registerCommand(tmpCmd, ShowWindowCmd::creator);        // Registers the command "MOF_EXP" and the function it calls. (It also checks if the registering process was succesfull)
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }
    
    return MS::kSuccess;                                                                // Process completed succesfully
}

MStatus uninitializePlugin(MObject obj)                                                 // Function that maya calls when the plugin gets unloaded (When it's removed from the Plug-in manager)
{
    MFnPlugin plugin(obj);                                                              // Creates a MFnPlugin asociated with the new obj

    for (std::string cmnd : Commands())
    {
        MString tmpCmd{ &cmnd[0], (int)cmnd.length() };
        MStatus status = plugin.deregisterCommand(tmpCmd);                              // Deregisters the stablished commands and checks if the result was succesful
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }
    return MS::kSuccess;                                                                // Process completed succesfully
}
