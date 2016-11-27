/**********************************************************
* Holo_ToolSet
* http://github.com/HasseltVR/Holo_ToolSet
* http://www.uhasselt.be/edm
*
* Distributed under LGPL v2.1 Licence
* http ://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
**********************************************************/
#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <QMainWindow>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
#include <QList>

#include "HPVCreator.hpp"

class QCloseEvent;
class QComboBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QSpinBox;
class QProgressBar;

using namespace HPV;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent=0);

private slots:
    void convertOrCancel();
    void updateUi();
    void checkProgress();
    void checkIfDone();
    void directoryInChanged();
    void directoryOutChanged();
    void inFrameChanged(int in);
    void outFrameChanged(int out);
    void fpsChanged(int fps);
    void compressionModeChanged(int mode);

protected:
    void closeEvent(QCloseEvent *event);
    void dragEnterEvent(QDragEnterEvent *event);
    void dragMoveEvent(QDragMoveEvent *event);
    void dragLeaveEvent(QDragLeaveEvent *event);
    void dropEvent(QDropEvent *event);

private:
    void createWidgets();
    void createLayout();
    void createConnections();
    void updateInputFolder(const QString& folder);
    void updateOutputFolder(const QString& folder);

    QLabel *directoryInLabel;
    QLineEdit *directoryInEdit;
    QPushButton *directoryInButton;
    QLabel *directoryOutLabel;
    QLineEdit *directoryOutEdit;
    QPushButton *directoryOutButton;
    QLabel *inFrameLabel;
    QSpinBox *inFrameSpinBox;
    QLabel *outFrameLabel;
    QSpinBox *outFrameSpinBox;
    QLabel *fpsLabel;
    QSpinBox *fpsSpinBox;
    QLabel *modeLabel;
    QComboBox *modeComboBox;
    QPushButton *convertOrCancelButton;
    QPushButton *quitButton;
    QProgressBar * progressBar;
    QPlainTextEdit *logEdit;

    HPVCreatorParams hpv_params;
    HPVCreator hpv_creator;
    std::vector<std::string> file_names;

    QString m_cur_filename;

    volatile bool stopped;

    uint64_t current_total;
    uint64_t planned_total;

    ThreadSafe_Queue<HPVCompressionProgress> progress_sink;
};

#endif // MAINWINDOW_HPP
