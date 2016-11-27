#include "aqp.hpp"
#include "alt_key.hpp"
#include "mainwindow.hpp"
#include <QApplication>
#include <QCloseEvent>
#include <QComboBox>
#include <QCompleter>
#include <QFileDialog>
#include <QSpinBox>
#include <QDirIterator>
#include <QDirModel>
#include <QGridLayout>
#include <QImageReader>
#include <QImageWriter>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QProgressBar>
#include <QStyleFactory>
#include <QTimer>

namespace {

const int PollTimeout = 100;

#ifdef USE_CUSTOM_DIR_MODEL
// Taken from Qt's completer example
class DirModel : public QDirModel
{
public:
    explicit DirModel(QObject *parent=0) : QDirModel(parent) {}

    QVariant data(const QModelIndex &index, int role=Qt::DisplayRole) const
    {
        if (role == Qt::DisplayRole && index.column() == 0) {
            QString path = QDir::toNativeSeparators(filePath(index));
            if (path.endsWith(QDir::separator()))
                path.chop(1);
            return path;
        }
        return QDirModel::data(index, role);
    }
};
#endif

} // anonymous namespace

MainWindow::MainWindow(QWidget *parent): QMainWindow(parent), stopped(true)
{
    //QApplication::setStyle(QStyleFactory::create("Fusion"));

    HPV::hpv_log_init();

    createWidgets();
    createLayout();
    createConnections();

    AQP::accelerateWidget(this);
    updateUi();
    directoryInEdit->setFocus();
    setWindowTitle(QApplication::applicationName());

    setAcceptDrops(true);

    hpv_params.fps = 25;
    hpv_params.in_frame = 0;
    hpv_params.out_frame = 0;
    hpv_params.in_path = "";
    hpv_params.out_path = "";
    hpv_params.file_names = nullptr;
    hpv_params.type = HPV::HPVCompressionType::HPV_TYPE_DXT1_NO_ALPHA;

    stopped = true;
}

void MainWindow::createWidgets()
{
    directoryInLabel = new QLabel(tr("Input path:"));
    directoryOutLabel = new QLabel(tr("Output path:"));

    QCompleter *directoryCompleter = new QCompleter(this);
#ifndef Q_WS_X11
    directoryCompleter->setCaseSensitivity(Qt::CaseInsensitive);
#endif
#ifdef USE_CUSTOM_DIR_MODEL
    directoryCompleter->setModel(new DirModel(directoryCompleter));
#else
    directoryCompleter->setModel(new QDirModel(directoryCompleter));
#endif
    directoryInEdit = new QLineEdit();
    directoryInEdit->setCompleter(directoryCompleter);
    directoryInLabel->setBuddy(directoryInEdit);

    directoryOutEdit = new QLineEdit();
    directoryOutEdit->setCompleter(directoryCompleter);
    directoryOutLabel->setBuddy(directoryOutEdit);

    directoryInButton = new QPushButton();
    directoryInButton->setText(tr("Choose input folder"));

    directoryOutButton = new QPushButton();
    directoryOutButton->setText(tr("Choose output folder"));

    inFrameLabel = new QLabel(tr("Start frame:"));
    inFrameSpinBox = new QSpinBox;
    inFrameSpinBox->setRange(0, INT_MAX);

    outFrameLabel = new QLabel(tr("End frame"));
    outFrameSpinBox = new QSpinBox;
    outFrameSpinBox->setRange(0, INT_MAX);

    fpsLabel = new QLabel(tr("FPS:"));
    fpsSpinBox = new QSpinBox;
    fpsSpinBox->setRange(1, 100);
    fpsSpinBox->setValue(25);

    modeLabel = new QLabel(tr("Compression mode:"));
    modeComboBox = new QComboBox();

    for (uint8_t type = 0; type < (uint8_t)HPVCompressionType::HPV_NUM_TYPES; ++type)
    {
        modeComboBox->addItem(QString::fromStdString(HPVCompressionTypeStrings[type]));
    }

    progressBar = new QProgressBar;
    progressBar->setOrientation(Qt::Horizontal);
    progressBar->setRange(0,100);

    logEdit = new QPlainTextEdit;
    logEdit->setReadOnly(true);
    logEdit->setPlainText(tr("Choose a path, source type and target file type, and click Convert."));

    convertOrCancelButton = new QPushButton(tr("&Convert"));
    quitButton = new QPushButton(tr("Quit"));
}

void MainWindow::createLayout()
{
    QGridLayout *layout = new QGridLayout;
    layout->addWidget(directoryInLabel, 0, 0);
    layout->addWidget(directoryInEdit, 0, 1, 1, 4);
    layout->addWidget(directoryInButton, 0, 5);
    layout->addWidget(directoryOutLabel, 1, 0);
    layout->addWidget(directoryOutEdit, 1, 1, 1, 4);
    layout->addWidget(directoryOutButton,1 , 5);
    layout->addWidget(inFrameLabel, 2, 0);
    layout->addWidget(inFrameSpinBox, 2, 1);
    layout->addWidget(outFrameLabel, 2, 2);
    layout->addWidget(outFrameSpinBox, 2, 3);
    layout->addWidget(fpsLabel, 2, 4);
    layout->addWidget(fpsSpinBox, 2, 5);
    layout->addWidget(modeLabel, 3, 0);
    layout->addWidget(modeComboBox, 3, 1);
    layout->addWidget(convertOrCancelButton, 3, 2, 1, 2);
    layout->addWidget(quitButton, 3, 4, 1, 2);
    layout->addWidget(progressBar, 4, 0, 1, 6);
    layout->addWidget(logEdit, 5, 0, 1, 6);

    QWidget *widget = new QWidget;
    widget->setLayout(layout);
    setCentralWidget(widget);
}

void MainWindow::createConnections()
{
    connect(directoryInEdit, SIGNAL(textChanged(const QString&)), this, SLOT(updateUi()));
    connect(directoryInButton, SIGNAL(clicked(bool)), this, SLOT(directoryInChanged()));
    connect(directoryOutButton, SIGNAL(clicked(bool)), this, SLOT(directoryOutChanged()));
    connect(inFrameSpinBox, SIGNAL(valueChanged(int)), this, SLOT(inFrameChanged(int)));
    connect(outFrameSpinBox, SIGNAL(valueChanged(int)), this, SLOT(outFrameChanged(int)));
    connect(fpsSpinBox, SIGNAL(valueChanged(int)), this, SLOT(fpsChanged(int)));
    connect(modeComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(compressionModeChanged(int)));
    connect(convertOrCancelButton, SIGNAL(clicked()), this, SLOT(convertOrCancel()));
    connect(quitButton, SIGNAL(clicked()), this, SLOT(close()));
}

void MainWindow::updateUi()
{
    if (stopped)
    {
        convertOrCancelButton->setText(tr("&Convert"));
        convertOrCancelButton->setEnabled(!directoryInEdit->text().isEmpty());
    }
    else
    {
        convertOrCancelButton->setText(tr("&Cancel"));
        convertOrCancelButton->setEnabled(true);
    }
}

void MainWindow::updateInputFolder(const QString& folder)
{
    logEdit->clear();
    file_names.clear();

    QDir work_dir(folder);
    QString work_dir_path = folder + "/";

    QStringList filters;
    QStringList allowedHPVExtensions;

    for (std::size_t i=0; i<HPV::supported_filetypes.size(); ++i)
    {
        filters << HPV::supported_filetypes.at(i).c_str();
    }

    QStringList list = work_dir.entryList(filters, QDir::Files|QDir::Readable, QDir::Name);

    if (list.size() > 0)
    {
        foreach( QString str, list)
        {
            QString file = work_dir_path + str;
            file_names.push_back(file.toStdString());
        }
        hpv_params.file_names = &file_names;

        hpv_params.in_path = work_dir.absolutePath().toStdString();
        m_cur_filename = work_dir.dirName();

        if (directoryOutEdit->text().isEmpty())
        {
            work_dir.cdUp();
            QString out_path = work_dir.absolutePath() + "/" + m_cur_filename + ".hpv";
            hpv_params.out_path = out_path.toStdString();
            directoryOutEdit->setText(out_path);
        }

        directoryInEdit->setText(folder);
        outFrameSpinBox->setValue(static_cast<int>(file_names.size()-1));
        outFrameSpinBox->setMaximum(static_cast<int>(file_names.size()-1));
        logEdit->appendPlainText("Opened folder '" + folder + "'");
        logEdit->appendPlainText("Added " + QString::number(file_names.size()) + " files to compression list");
    }
    else
    {
        logEdit->appendPlainText("Directory '" + folder + "' doesn't contain any valid image files");
    }
}

void MainWindow::updateOutputFolder(const QString &folder)
{
    if (folder.length() > 0)
    {
        QString out_path;
        if (m_cur_filename.length() > 0)
        {
            out_path = folder + "/" + m_cur_filename + ".hpv";
        }
        else
        {
            out_path = folder + "/output.hpv";
        }

        directoryOutEdit->setText(out_path);
        hpv_params.out_path = out_path.toStdString();
    }

}

void MainWindow::directoryInChanged()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Choose directory with files to convert"),
                                                    "/home",
                                                    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    updateInputFolder(dir);
}

void MainWindow::directoryOutChanged()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Choose directory for output HPV file"),
                                                    "/home",
                                                    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    updateOutputFolder(dir);
}

void MainWindow::inFrameChanged(int in)
{
    hpv_params.in_frame = in;
}

void MainWindow::outFrameChanged(int out)
{
    hpv_params.out_frame = out;
}

void MainWindow::fpsChanged(int fps)
{
    hpv_params.fps = fps;
}

void MainWindow::compressionModeChanged(int mode)
{
    hpv_params.type = static_cast<HPVCompressionType>(mode);
}

void MainWindow::convertOrCancel()
{
    if (stopped)
    {
        stopped = false;
        progress_sink.clear();
        logEdit->appendPlainText("Starting conversion");
        convertOrCancelButton->setText(tr("&Cancel"));
        convertOrCancelButton->setEnabled(true);
        hpv_creator.init(hpv_params, &progress_sink);
        hpv_creator.process_sequence(4);
        QTimer::singleShot(PollTimeout, this, SLOT(checkProgress()));
    }
    else
    {
        hpv_creator.cancel();

        convertOrCancelButton->setText(tr("&Convert"));
        convertOrCancelButton->setEnabled(!directoryInEdit->text().isEmpty());

        progressBar->reset();
    }
}

void MainWindow::checkProgress()
{
    if (stopped)
        return;

    bool done = false;

    while (progress_sink.size() > 0)
    {
        HPVCompressionProgress progress;
        if (progress_sink.try_pop(progress))
        {
            if (progress.state == HPV_CREATOR_STATE_ERROR)
            {
                QString error_str = QString::fromStdString(progress.done_item_name);
                logEdit->appendPlainText(error_str);

                hpv_creator.cancel();

                stopped = true;
                updateUi();
            }
            else if (progress.state == HPV_CREATOR_STATE_DONE)
            {
                QString done_str = QString::fromStdString(progress.done_item_name);
                logEdit->appendPlainText(done_str);
                progressBar->reset();
                done = true;
                stopped = true;
                updateUi();

                hpv_creator.reset();

                break;
            }
            else
            {
                uint8_t percent = static_cast<uint8_t>( (progress.done_items/(float)progress.total_items) *100);

                QString str = "Done: " + QString::fromStdString(progress.done_item_name) + " [deflated to: " + QString::number(progress.compression_ratio, 'f', 2) + "%]";
                logEdit->appendPlainText(str);
                progressBar->setValue(percent);
            }
        }
    }

    if (!done)
        QTimer::singleShot(PollTimeout, this, SLOT(checkProgress()));

}

void MainWindow::checkIfDone()
{

}

void MainWindow::closeEvent(QCloseEvent *event)
{
    stopped = true;
    event->accept();
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event)
{
    event->acceptProposedAction();
}

void MainWindow::dragMoveEvent(QDragMoveEvent* event)
{
    event->acceptProposedAction();
}

void MainWindow::dragLeaveEvent(QDragLeaveEvent* event)
{
    event->accept();
}

void MainWindow::dropEvent(QDropEvent* event)
{
    const QMimeData* mimeData = event->mimeData();

    if (mimeData->hasUrls())
    {
        QUrl path = mimeData->urls().at(0);
        QString localPath = path.toLocalFile();
        QFileInfo fileInfo(localPath);

        if(fileInfo.isDir())
        {
            updateInputFolder(localPath);
        }
    }
}
