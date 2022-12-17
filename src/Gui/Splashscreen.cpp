/***************************************************************************
 *   Copyright (c) 2004 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


#include "PreCompiled.h"
#ifndef _PreComp_
# include <cstdlib>
# include <QApplication>
# include <QClipboard>
# include <QDesktopWidget>
# include <QDesktopServices>
# include <QDialogButtonBox>
# include <QLocale>
# include <QMutex>
# include <QTextBrowser>
# include <QProcess>
# include <QProcessEnvironment>
# include <QSysInfo>
# include <QTextStream>
# include <QWaitCondition>
# include <QLabel>
# include <QFileInfo>
# include <QDir>
# include <Inventor/C/basic.h>
#endif

#include <boost/algorithm/string/predicate.hpp>

#include <QScreen>
# include <QMovie>

#include <LibraryVersions.h>
#include <zlib.h>
#include <boost/version.hpp>

#include "Splashscreen.h"
#include "ui_AboutApplication.h"
#include <Base/Console.h>
#include <CXX/WrapPython.h>
#include <App/Application.h>
#include <Gui/MainWindow.h>


using namespace Gui;
using namespace Gui::Dialog;

static inline int parseAlignment(const std::string &text)
{
    int align=0;
    if (boost::starts_with(text, "VCenter"))
        align = Qt::AlignVCenter;
    else if (boost::starts_with(text, "Top"))
        align = Qt::AlignTop;
    else if (boost::starts_with(text, "Bottom"))
        align = Qt::AlignBottom;

    if (boost::starts_with(text, "HCenter"))
        align += Qt::AlignHCenter;
    else if (boost::starts_with(text, "Right"))
        align += Qt::AlignRight;
    else if (boost::starts_with(text, "Left"))
        align += Qt::AlignLeft;
    return align;
}

namespace Gui {
/** Displays all messages at startup inside the splash screen.
 * \author Werner Mayer
 */
class SplashObserver : public Base::ILogger
{
public:
    SplashObserver(QSplashScreen* splasher=0)
      : splash(splasher), alignment(Qt::AlignBottom|Qt::AlignLeft), textColor(Qt::black)
    {
        Base::Console().AttachObserver(this);

        // allow to customize text position and color
        const std::map<std::string,std::string>& cfg = App::GetApplication().Config();
        std::map<std::string,std::string>::const_iterator al = cfg.find("SplashAlignment");

        if (al != cfg.end()) {
            if (int align = parseAlignment(al->second))
                alignment = align;
        }

        // choose text color
        std::map<std::string,std::string>::const_iterator tc = cfg.find("SplashTextColor");
        if (tc != cfg.end()) {
            QColor col; col.setNamedColor(QString::fromUtf8(tc->second.c_str()));
            if (col.isValid())
                textColor = col;
        }
    }
    virtual ~SplashObserver()
    {
        Base::Console().DetachObserver(this);
    }
    const char* Name() override
    {
        return "SplashObserver";
    }
    void SendLog(const std::string& msg, Base::LogStyle level) override
    {
#ifdef FC_DEBUG
        Log(msg.c_str());
        Q_UNUSED(level)
#else
        if (level == Base::LogStyle::Log) {
            Log(msg.c_str());
        }
#endif
    }
    void Log (const char * s)
    {
        QString msg(QString::fromUtf8(s));
        QRegExp rx;
        // ignore 'Init:' and 'Mod:' prefixes
        rx.setPattern(QStringLiteral("^\\s*(Init:|Mod:)\\s*"));
        int pos = rx.indexIn(msg);
        if (pos != -1) {
            msg = msg.mid(rx.matchedLength());
        }
        else {
            // ignore activation of commands
            rx.setPattern(QStringLiteral("^\\s*(\\+App::|Create|CmdC:|CmdG:|Act:)\\s*"));
            pos = rx.indexIn(msg);
            if (pos == 0)
                return;
        }

        splash->showMessage(msg.replace(QStringLiteral("\n"), QString()), alignment, textColor);
        QMutex mutex;
        QMutexLocker ml(&mutex);
        QWaitCondition().wait(&mutex, 50);
    }

private:
    QSplashScreen* splash;
    int alignment;
    QColor textColor;
};
} // namespace Gui

// ------------------------------------------------------------------------------

namespace {

class GifLabel: public QLabel
{
public:
    GifLabel(QWidget *parent)
        :QLabel(parent)
    {
        parent->installEventFilter(this);
    }

    bool eventFilter(QObject *o, QEvent *e)
    {
        if (e->type() == QEvent::Resize) {
            auto parentSize = static_cast<QResizeEvent*>(e)->size();
            if (auto label = qobject_cast<QLabel*>(o)) {
                QSize imageSize;
                if (auto pixmap = label->pixmap())
                    imageSize = pixmap->size();
                else if (auto movie = label->movie())
                    imageSize = movie->frameRect().size();
                if (!label->hasScaledContents()
                        && imageSize.height()
                        && imageSize.width()
                        && parentSize.height())
                {
                    double ar = double(imageSize.width()) / imageSize.height();
                    double arParent = double(parentSize.width()) / parentSize.height();
                    if (arParent > ar) {
                        imageSize.setWidth(imageSize.width() * parentSize.height() / imageSize.height());
                        imageSize.setHeight(parentSize.height());
                    } else {
                        imageSize.setHeight(imageSize.height() * parentSize.width() / imageSize.width());
                        imageSize.setWidth(parentSize.width());
                    }

                    QPoint pos;
                    auto align = label->alignment();
                    if (align & Qt::AlignRight)
                        pos.setX(parentSize.width() - imageSize.width());
                    else if (align & Qt::AlignHCenter)
                        pos.setX((parentSize.width() - imageSize.width())/2);
                    if (align & Qt::AlignBottom)
                        pos.setY(parentSize.height() - imageSize.height());
                    else if (align & Qt::AlignVCenter)
                        pos.setY((parentSize.height() - imageSize.height())/2);
                    move(pos);
                    parentSize = imageSize;
                }
            }
            resize(parentSize);
        }
        return QLabel::eventFilter(o, e);
    }
};

QLabel *loadSplashGif(QWidget *parent, const char *key, const char *alignmentKey)
{
    const auto& cfg = App::GetApplication().Config();
    auto itGif = cfg.find(key);
    if (itGif == cfg.end())
        return nullptr;
    QString path = QString::fromUtf8(itGif->second.c_str());
    if (QDir(path).isRelative()) {
        QString home = QString::fromUtf8(App::GetApplication().getHomePath());
        path = QFileInfo(QDir(home), path).absoluteFilePath();
    }
    auto movie =  new QMovie(parent);
    movie->setFileName(path);
    if (!movie->isValid()) {
        delete movie;
        return nullptr;
    }
    auto gifLabel = new GifLabel(parent);
    gifLabel->setMovie(movie);
    movie->start();
    auto itAlignment = cfg.find(alignmentKey);
    int align = 0;
    if (itAlignment != cfg.end())
        align = parseAlignment(itAlignment->second);
    if (align)
        gifLabel->setAlignment(Qt::AlignmentFlag(align));
    else
        gifLabel->setAlignment(Qt::AlignTop | Qt::AlignRight);
    gifLabel->setAttribute(Qt::WA_TranslucentBackground, true);
    return gifLabel;
}
} // anonymous namespace

/**
 * Constructs a splash screen that will display the pixmap.
 */
SplashScreen::SplashScreen(  const QPixmap & pixmap , Qt::WindowFlags f )
    : QSplashScreen(pixmap, f)
{
    // write the messages to splasher
    messages = new SplashObserver(this);
    loadSplashGif(this, "SplashGif", "SplashGifAlignment");
}

/** Destruction. */
SplashScreen::~SplashScreen()
{
    delete messages;
}

/**
 * Draws the contents of the splash screen using painter \a painter. The default
 * implementation draws the message passed by message().
 */
void SplashScreen::drawContents ( QPainter * painter )
{
    QSplashScreen::drawContents(painter);
}

// ------------------------------------------------------------------------------

AboutDialogFactory* AboutDialogFactory::factory = 0;

AboutDialogFactory::~AboutDialogFactory()
{
}

QDialog *AboutDialogFactory::create(QWidget *parent) const
{
#ifdef _USE_3DCONNEXION_SDK
    return new AboutDialog(true, parent);
#else
    return new AboutDialog(false, parent);
#endif
}

const AboutDialogFactory *AboutDialogFactory::defaultFactory()
{
    static const AboutDialogFactory this_factory;
    if (factory)
        return factory;
    return &this_factory;
}

void AboutDialogFactory::setDefaultFactory(AboutDialogFactory *f)
{
    if (factory != f)
        delete factory;
    factory = f;
}

// ------------------------------------------------------------------------------

/* TRANSLATOR Gui::Dialog::AboutDialog */

/**
 *  Constructs an AboutDialog which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'WStyle_Customize|WStyle_NoBorder|WType_Modal'
 *
 *  The dialog will be modal.
 */
AboutDialog::AboutDialog(bool showLic, QWidget* parent)
  : QDialog(parent), ui(new Ui_AboutApplication)
{
    Q_UNUSED(showLic);

    setModal(true);
    ui->setupUi(this);
    QRect rect = QApplication::desktop()->availableGeometry(getMainWindow());

    loadSplashGif(ui->labelSplashPicture, "SplashGif", "SplashGifAlignment");

    // See if we have a custom About screen image set
    QPixmap image = getMainWindow()->aboutImage();

    // Fallback to the splashscreen image
    if (image.isNull()) {
        image = getMainWindow()->splashImage();
    }

    // Make sure the image is not too big
    int denom = 2;
    if (image.height() > rect.height()/denom || image.width() > rect.width()/denom) {
        float scale = static_cast<float>(image.width()) / static_cast<float>(image.height());
        int width = std::min(image.width(), rect.width()/denom);
        int height = std::min(image.height(), rect.height()/denom);
        height = std::min(height, static_cast<int>(width / scale));
        width = static_cast<int>(scale * height);

        image = image.scaled(width, height,
                    Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }
    ui->labelSplashPicture->setPixmap(image);
    ui->tabWidget->setCurrentIndex(0); // always start on the About tab

    setupLabels();
    showCredits();
    showLicenseInformation();
    showLibraryInformation();
    showCollectionInformation();
    showOrHideImage(rect);
}

/**
 *  Destroys the object and frees any allocated resources
 */
AboutDialog::~AboutDialog()
{
    // no need to delete child widgets, Qt does it all for us
    delete ui;
}

void AboutDialog::showOrHideImage(const QRect& rect)
{
    adjustSize();
    if (height() > rect.height()) {
        ui->labelSplashPicture->hide();
    }
}

void AboutDialog::setupLabels()
{
    //fonts are rendered smaller on Mac so point size can't be the same for all platforms
    int fontSize = 8;
#ifdef Q_OS_MAC
    fontSize = 11;
#endif
    //avoid overriding user set style sheet
    if (qApp->styleSheet().isEmpty()) {
        setStyleSheet(QStringLiteral("Gui--Dialog--AboutDialog QLabel {font-size: %1pt;}").arg(fontSize));
    }

    QString exeName = qApp->applicationName();
    std::map<std::string, std::string>& config = App::Application::Config();
    std::map<std::string,std::string>::iterator it;
    QString banner  = QString::fromUtf8(config["CopyrightInfo"].c_str());
    banner = banner.left( banner.indexOf(QLatin1Char('\n')) );
    QString major  = QString::fromUtf8(config["BuildVersionMajor"].c_str());
    QString minor  = QString::fromUtf8(config["BuildVersionMinor"].c_str());
    QString build  = QString::fromUtf8(config["BuildRevision"].c_str());
    QString disda  = QString::fromUtf8(config["BuildRevisionDate"].c_str());
    QString mturl  = QString::fromUtf8(config["MaintainerUrl"].c_str());

    // we use replace() to keep label formatting, so a label with text "<b>Unknown</b>"
    // gets replaced to "<b>FreeCAD</b>", for example

    QString author = ui->labelAuthor->text();
    author.replace(QStringLiteral("Unknown Application"), exeName);
    author.replace(QStringLiteral("(c) Unknown Author"), banner);
    ui->labelAuthor->setText(author);
    ui->labelAuthor->setUrl(mturl);

    if (qApp->styleSheet().isEmpty()) {
        ui->labelAuthor->setStyleSheet(QStringLiteral("Gui--UrlLabel {color: #0000FF;text-decoration: underline;font-weight: 600;font-family: MS Shell Dlg 2;}"));
    }

    QString version = ui->labelBuildVersion->text();
    version.replace(QStringLiteral("Unknown"), QStringLiteral("%1.%2").arg(major, minor));
    ui->labelBuildVersion->setText(version);

    QString revision = ui->labelBuildRevision->text();
    revision.replace(QStringLiteral("Unknown"), build);
    ui->labelBuildRevision->setText(revision);

    QString date = ui->labelBuildDate->text();
    date.replace(QStringLiteral("Unknown"), disda);
    ui->labelBuildDate->setText(date);

    QString os = ui->labelBuildOS->text();
    os.replace(QStringLiteral("Unknown"), QSysInfo::prettyProductName());
    ui->labelBuildOS->setText(os);

    QString platform = ui->labelBuildPlatform->text();
    platform.replace(QStringLiteral("Unknown"),
        QStringLiteral("%1-bit").arg(QSysInfo::WordSize));
    ui->labelBuildPlatform->setText(platform);

    // branch name
    it = config.find("BuildRevisionBranch");
    if (it != config.end()) {
        QString branch = ui->labelBuildBranch->text();
        branch.replace(QStringLiteral("Unknown"), QString::fromUtf8(it->second.c_str()));
        ui->labelBuildBranch->setText(branch);
    }
    else {
        ui->labelBranch->hide();
        ui->labelBuildBranch->hide();
    }

    // hash id
    it = config.find("BuildRevisionHash");
    if (it != config.end()) {
        QString hash = ui->labelBuildHash->text();
        hash.replace(QStringLiteral("Unknown"), QString::fromUtf8(it->second.c_str()).left(7)); // Use the 7-char abbreviated hash
        ui->labelBuildHash->setText(hash);
        if (auto url_itr = config.find("BuildRepositoryURL"); url_itr != config.end()) {
            auto url = QString::fromStdString(url_itr->second);

            if (int space = url.indexOf(QChar::fromLatin1(' ')); space != -1)
                url = url.left(space); // Strip off the branch information to get just the repo

            if (url == QString::fromUtf8("Unknown"))
                url = QString::fromUtf8("https://github.com/FreeCAD/FreeCAD"); // Just take a guess

            // This may only create valid URLs for Github, but some other hosts use the same format so give it a shot...
            auto https = url.replace(QString::fromUtf8("git://"), QString::fromUtf8("https://"));
            https.replace(QString::fromUtf8(".git"), QString::fromUtf8(""));
            ui->labelBuildHash->setUrl(https + QString::fromUtf8("/commit/") + QString::fromStdString(it->second));
        }
    }
    else {
        ui->labelHash->hide();
        ui->labelBuildHash->hide();
    }
}

class AboutDialog::LibraryInfo {
public:
    QString name;
    QString version;
    QString href;
    QString url;
};

void AboutDialog::showCredits()
{
    QString creditsFileURL = QStringLiteral("%1/CONTRIBUTORS")
        .arg(QString::fromUtf8(App::Application::getHelpDir().c_str()));
    QFile creditsFile(creditsFileURL);

    if (!creditsFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }

    QWidget* tab_credits = new QWidget();
    tab_credits->setObjectName(QStringLiteral("tab_credits"));
    ui->tabWidget->addTab(tab_credits, tr("Credits"));
    QVBoxLayout* hlayout = new QVBoxLayout(tab_credits);
    QTextBrowser* textField = new QTextBrowser(tab_credits);
    textField->setOpenExternalLinks(false);
    textField->setOpenLinks(false);
    hlayout->addWidget(textField);

    QString creditsHTML = QStringLiteral("<html><body><h1>");
    //: Header for the Credits tab of the About screen
    creditsHTML += tr("Credits");
    creditsHTML += QStringLiteral("</h1><p>");
    creditsHTML += tr("FreeCAD would not be possible without the contributions of");
    creditsHTML += QStringLiteral(":</p><h2>"); 
    //: Header for the list of individual people in the Credits list.
    creditsHTML += tr("Individuals");
    creditsHTML += QStringLiteral("</h2><ul>");

    QTextStream stream(&creditsFile);
    stream.setCodec("UTF-8");
    QString line;
    while (stream.readLineInto(&line)) {
        if (!line.isEmpty()) {
            if (line == QStringLiteral("Firms")) {
                creditsHTML += QStringLiteral("</ul><h2>");
                //: Header for the list of companies/organizations in the Credits list.
                creditsHTML += tr("Organizations");
                creditsHTML += QStringLiteral("</h2><ul>");
            }
            else {
                creditsHTML += QStringLiteral("<li>") + line + QStringLiteral("</li>");
            }
        }
    }
    creditsHTML += QStringLiteral("</ul></body></html>");
    textField->setHtml(creditsHTML);
}

void AboutDialog::showLicenseInformation()
{
    QString licenseFileURL = QStringLiteral("%1/LICENSE.html")
        .arg(QString::fromUtf8(App::Application::getHelpDir().c_str()));
    QFile licenseFile(licenseFileURL);

    if (licenseFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString licenseHTML = QString::fromUtf8(licenseFile.readAll());
        const auto placeholder = QString::fromUtf8("<!--PLACEHOLDER_FOR_ADDITIONAL_LICENSE_INFORMATION-->");
        licenseHTML.replace(placeholder, getAdditionalLicenseInformation());

        ui->tabWidget->removeTab(1); // Hide the license placeholder widget

        QWidget* tab_license = new QWidget();
        tab_license->setObjectName(QStringLiteral("tab_license"));
        ui->tabWidget->addTab(tab_license, tr("License"));
        QVBoxLayout* hlayout = new QVBoxLayout(tab_license);
        QTextBrowser* textField = new QTextBrowser(tab_license);
        textField->setOpenExternalLinks(true);
        textField->setOpenLinks(true);
        hlayout->addWidget(textField);

        textField->setHtml(licenseHTML);
    }
    else {
        QString info(QStringLiteral("SUCH DAMAGES.<hr/>"));
        info += getAdditionalLicenseInformation();
        QString lictext = ui->textBrowserLicense->toHtml();
        lictext.replace(QStringLiteral("SUCH DAMAGES.<hr/>"), info);
        ui->textBrowserLicense->setHtml(lictext);
    }
}

QString AboutDialog::getAdditionalLicenseInformation() const
{
    // Any additional piece of text to be added after the main license text goes below.
    // Please set title in <h2> tags, license text in <p> tags
    // and add an <hr/> tag at the end to nicely separate license blocks
    QString info;
#ifdef _USE_3DCONNEXION_SDK
    info += QString::fromUtf8(
        "<h2>3D Mouse Support</h2>"
        "<p>Development tools and related technology provided under license from 3Dconnexion.<br/>"
        "Copyright &#169; 1992&ndash;2012 3Dconnexion. All rights reserved.</p>"
        "<hr/>"
    );
#endif
    return info;
}

void AboutDialog::showLibraryInformation()
{
    QWidget *tab_library = new QWidget();
    tab_library->setObjectName(QStringLiteral("tab_library"));
    ui->tabWidget->addTab(tab_library, tr("Libraries"));
    QVBoxLayout* hlayout = new QVBoxLayout(tab_library);
    QTextBrowser* textField = new QTextBrowser(tab_library);
    textField->setOpenExternalLinks(false);
    textField->setOpenLinks(false);
    hlayout->addWidget(textField);

    QList<LibraryInfo> libInfo;
    LibraryInfo li;
    QString baseurl = QStringLiteral("file:///%1/ThirdPartyLibraries.html")
            .arg(QString::fromUtf8(App::Application::getHelpDir().c_str()));

    // Boost
    li.name = QStringLiteral("Boost");
    li.href = baseurl + QStringLiteral("#_TocBoost");
    li.url = QStringLiteral("http://www.boost.org");
    li.version = QStringLiteral(BOOST_LIB_VERSION);
    libInfo << li;

    // Coin3D
    li.name = QStringLiteral("Coin3D");
    li.href = baseurl + QStringLiteral("#_TocCoin3D");
    li.url = QStringLiteral("https://coin3d.github.io");
    li.version = QStringLiteral(COIN_VERSION);
    libInfo << li;

    // Eigen3
    li.name = QStringLiteral("Eigen3");
    li.href = baseurl + QStringLiteral("#_TocEigen3");
    li.url = QStringLiteral("http://eigen.tuxfamily.org");
    li.version = QString::fromUtf8(FC_EIGEN3_VERSION);
    libInfo << li;

    // FreeType
    li.name = QStringLiteral("FreeType");
    li.href = baseurl + QStringLiteral("#_TocFreeType");
    li.url = QStringLiteral("http://freetype.org");
    li.version = QString::fromUtf8(FC_FREETYPE_VERSION);
    libInfo << li;

    // KDL
    li.name = QStringLiteral("KDL");
    li.href = baseurl + QStringLiteral("#_TocKDL");
    li.url = QStringLiteral("http://www.orocos.org/kdl");
    li.version.clear();
    libInfo << li;

    // libarea
    li.name = QStringLiteral("libarea");
    li.href = baseurl + QStringLiteral("#_TocLibArea");
    li.url = QStringLiteral("https://github.com/danielfalck/libarea");
    li.version.clear();
    libInfo << li;

    // OCCT
#if defined(HAVE_OCC_VERSION)
    li.name = QStringLiteral("Open CASCADE Technology");
    li.href = baseurl + QStringLiteral("#_TocOCCT");
    li.url = QStringLiteral("http://www.opencascade.com");
    li.version = QStringLiteral(OCC_VERSION_STRING_EXT);
    libInfo << li;
#endif

    // pcl
    li.name = QStringLiteral("Point Cloud Library");
    li.href = baseurl + QStringLiteral("#_TocPcl");
    li.url = QStringLiteral("http://www.pointclouds.org");
    li.version = QString::fromUtf8(FC_PCL_VERSION);
    libInfo << li;

    // PyCXX
    li.name = QStringLiteral("PyCXX");
    li.href = baseurl + QStringLiteral("#_TocPyCXX");
    li.url = QStringLiteral("http://cxx.sourceforge.net");
    li.version = QString::fromUtf8(FC_PYCXX_VERSION);
    libInfo << li;

    // Python
    li.name = QStringLiteral("Python");
    li.href = baseurl + QStringLiteral("#_TocPython");
    li.url = QStringLiteral("http://www.python.org");
    li.version = QStringLiteral(PY_VERSION);
    libInfo << li;

    // PySide
    li.name = QStringLiteral("PySide");
    li.href = baseurl + QStringLiteral("#_TocPySide");
    li.url = QStringLiteral("http://www.pyside.org");
    li.version = QString::fromUtf8(FC_PYSIDE_VERSION);
    libInfo << li;

    // Qt
    li.name = QStringLiteral("Qt");
    li.href = baseurl + QStringLiteral("#_TocQt");
    li.url = QStringLiteral("http://www.qt.io");
    li.version = QStringLiteral(QT_VERSION_STR);
    libInfo << li;

    // Salome SMESH
    li.name = QStringLiteral("Salome SMESH");
    li.href = baseurl + QStringLiteral("#_TocSalomeSMESH");
    li.url = QStringLiteral("http://salome-platform.org");
    li.version.clear();
    libInfo << li;

    // Shiboken
    li.name = QStringLiteral("Shiboken");
    li.href = baseurl + QStringLiteral("#_TocPySide");
    li.url = QStringLiteral("http://www.pyside.org");
    li.version = QString::fromUtf8(FC_SHIBOKEN_VERSION);
    libInfo << li;

    // vtk
    li.name = QStringLiteral("vtk");
    li.href = baseurl + QStringLiteral("#_TocVtk");
    li.url = QStringLiteral("https://www.vtk.org");
    li.version = QString::fromUtf8(FC_VTK_VERSION);
    libInfo << li;

    // Xerces-C
    li.name = QStringLiteral("Xerces-C");
    li.href = baseurl + QStringLiteral("#_TocXercesC");
    li.url = QStringLiteral("https://xerces.apache.org/xerces-c");
    li.version = QString::fromUtf8(FC_XERCESC_VERSION);
    libInfo << li;

    // Zipios++
    li.name = QStringLiteral("Zipios++");
    li.href = baseurl + QStringLiteral("#_TocZipios");
    li.url = QStringLiteral("http://zipios.sourceforge.net");
    li.version.clear();
    libInfo << li;

    // zlib
    li.name = QStringLiteral("zlib");
    li.href = baseurl + QStringLiteral("#_TocZlib");
    li.url = QStringLiteral("http://zlib.net");
    li.version = QString::fromUtf8(ZLIB_VERSION);
    libInfo << li;


    QString msg = tr("This software uses open source components whose copyright and other "
                     "proprietary rights belong to their respective owners:");
    QString html;
    QTextStream out(&html);
    out << "<html><head/><body style=\" font-family:'MS Shell Dlg 2'; font-size:8.25pt; font-weight:400; font-style:normal;\">"
        << "<p>" << msg << "<br/></p>\n<ul>\n";
    for (QList<LibraryInfo>::iterator it = libInfo.begin(); it != libInfo.end(); ++it) {
        out << "<li><p>" << it->name << " " << it->version << "</p>"
               "<p><a href=\"" << it->href << "\">" << it->url
            << "</a><br/></p></li>\n";
    }
    out << "</ul>\n</body>\n</html>";
    textField->setHtml(html);

    connect(textField, SIGNAL(anchorClicked(QUrl)), this, SLOT(linkActivated(QUrl)));
}

void AboutDialog::showCollectionInformation()
{
    QString doc = QString::fromUtf8(App::Application::getHelpDir().c_str());
    QString path = doc + QStringLiteral("Collection.html");
    if (!QFile::exists(path))
        return;

    QWidget *tab_collection = new QWidget();
    tab_collection->setObjectName(QStringLiteral("tab_collection"));
    ui->tabWidget->addTab(tab_collection, tr("Collection"));
    QVBoxLayout* hlayout = new QVBoxLayout(tab_collection);
    QTextBrowser* textField = new QTextBrowser(tab_collection);
    textField->setOpenExternalLinks(true);
    hlayout->addWidget(textField);
    textField->setSource(path);
}

void AboutDialog::linkActivated(const QUrl& link)
{
    LicenseView* licenseView = new LicenseView();
    licenseView->setAttribute(Qt::WA_DeleteOnClose);
    licenseView->show();
    QString title = tr("License");
    QString fragment = link.fragment();
    if (fragment.startsWith(QStringLiteral("_Toc"))) {
        QString prefix = fragment.mid(4);
        title = QStringLiteral("%1 %2").arg(prefix, title);
    }
    licenseView->setWindowTitle(title);
    getMainWindow()->addWindow(licenseView);
    licenseView->setSource(link);
}

void AboutDialog::on_copyButton_clicked()
{
    QString data;
    QTextStream str(&data);
    std::map<std::string, std::string>& config = App::Application::Config();
    std::map<std::string,std::string>::iterator it;
    QString exe = QString::fromUtf8(App::GetApplication().getExecutableName());

    QString major  = QString::fromUtf8(config["BuildVersionMajor"].c_str());
    QString minor  = QString::fromUtf8(config["BuildVersionMinor"].c_str());
    QString build  = QString::fromUtf8(config["BuildRevision"].c_str());

    QString deskEnv = QProcessEnvironment::systemEnvironment().value(QStringLiteral("XDG_CURRENT_DESKTOP"),QStringLiteral(""));
    QString deskSess = QProcessEnvironment::systemEnvironment().value(QStringLiteral("DESKTOP_SESSION"),QStringLiteral(""));
    QString deskInfo = QStringLiteral("");

    if (!(deskEnv == QStringLiteral("") && deskSess == QStringLiteral("")))
    {
        if (deskEnv == QStringLiteral("") || deskSess == QStringLiteral(""))
        {
            deskInfo = QStringLiteral(" (") + deskEnv + deskSess + QStringLiteral(")");

        }
        else
        {
            deskInfo = QStringLiteral(" (") + deskEnv + QStringLiteral("/") + deskSess + QStringLiteral(")");
        }
    }

    str << "OS: " << QSysInfo::prettyProductName() << deskInfo << '\n';
    str << "Word size of " << exe << ": " << QSysInfo::WordSize << "-bit\n";
    str << "Version: " << major << "." << minor << "." << build;
    char *appimage = getenv("APPIMAGE");
    if (appimage)
        str << " AppImage";
    str << '\n';

#if defined(_DEBUG) || defined(DEBUG)
    str << "Build type: Debug\n";
#elif defined(NDEBUG)
    str << "Build type: Release\n";
#elif defined(CMAKE_BUILD_TYPE)
    str << "Build type: " << CMAKE_BUILD_TYPE << '\n';
#else
    str << "Build type: Unknown\n";
#endif
    it = config.find("BuildRevisionBranch");
    if (it != config.end())
        str << "Branch: " << QString::fromUtf8(it->second.c_str()) << '\n';
    it = config.find("BuildRevisionHash");
    if (it != config.end())
        str << "Hash: " << it->second.c_str() << '\n';
    // report also the version numbers of the most important libraries in FreeCAD
    str << "Python version: " << PY_VERSION << '\n';
    str << "Qt version: " << QT_VERSION_STR << '\n';
    str << "Coin version: " << COIN_VERSION << '\n';
#if defined(HAVE_OCC_VERSION)
    str << "OCC version: "
        << OCC_VERSION_MAJOR << "."
        << OCC_VERSION_MINOR << "."
        << OCC_VERSION_MAINTENANCE
#ifdef OCC_VERSION_DEVELOPMENT
        << "." OCC_VERSION_DEVELOPMENT
#endif
        << '\n';
#endif
    QLocale loc;
    str << "Locale: " << loc.languageToString(loc.language()) << "/"
        << loc.countryToString(loc.country())
        << " (" << loc.name() << ")\n";

    QClipboard* cb = QApplication::clipboard();
    cb->setText(data);
}

// ----------------------------------------------------------------------------

/* TRANSLATOR Gui::LicenseView */

LicenseView::LicenseView(QWidget* parent)
    : MDIView(0,parent,Qt::WindowFlags())
{
    browser = new QTextBrowser(this);
    browser->setOpenExternalLinks(true);
    browser->setOpenLinks(true);
    setCentralWidget(browser);
}

LicenseView::~LicenseView()
{
}

void LicenseView::setSource(const QUrl& url)
{
    browser->setSource(url);
}

#include "moc_Splashscreen.cpp"
