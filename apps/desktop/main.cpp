#include "medforge/core/Version.h"
#include "medforge/ui/FoundationWindow.h"
#include "medforge/ui/GalleryWindow.h"
#include "medforge/ui/SampleViewport.h"
#include "medforge/ui/ScreenshotCapture.h"
#include "medforge/ui/Style.h"
#include "medforge/ui/WorkspaceReference.h"
#include "medforge/ui/WorkspaceWindow.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QEventLoop>
#include <QStandardPaths>
#include <QTextStream>
#include <QTimer>

int main(int argc, char* argv[]) {
#ifdef Q_OS_WIN
    if (qEnvironmentVariableIsEmpty("QT_QPA_FONTDIR")) {
        qputenv("QT_QPA_FONTDIR", "C:/Windows/Fonts");
    }
#endif
    medforge::ui::prepareSampleViewportSurface();
    QApplication application(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("MedForge"));
    QCoreApplication::setApplicationVersion(QString::fromUtf8(medforge::core::kVersionString));
    QCoreApplication::setOrganizationName(QStringLiteral("MedForge"));
    medforge::ui::loadBundledFonts();
    medforge::ui::applyMedForgeStyle(application);

    QCommandLineParser parser;
    parser.setApplicationDescription(
        QStringLiteral("MedForge research workstation foundation. Not for clinical use."));
    parser.addHelpOption();
    parser.addVersionOption();
    const QCommandLineOption galleryOption(QStringLiteral("gallery"),
                                           QStringLiteral("Open the developer component gallery."));
    const QCommandLineOption referenceOption(
        QStringLiteral("reference"), QStringLiteral("Open the M1 workspace layout reference."));
    const QCommandLineOption captureOption(
        QStringLiteral("capture"),
        QStringLiteral("Write gallery and reference screenshots into <directory> and exit."),
        QStringLiteral("directory"));
    const QCommandLineOption sampleOption(QStringLiteral("sample"),
                                          QStringLiteral("Show the sample cube and sphere."));
    const QCommandLineOption sampleShotOption(
        QStringLiteral("sample-shot"),
        QStringLiteral("Show the sample cube and sphere, write a PNG, and exit."),
        QStringLiteral("png"));
    parser.addOption(galleryOption);
    parser.addOption(referenceOption);
    parser.addOption(captureOption);
    parser.addOption(sampleOption);
    parser.addOption(sampleShotOption);
    parser.process(application);

    if (parser.isSet(captureOption)) {
        QString error;
        if (!medforge::ui::captureUiEvidence(parser.value(captureOption), &error)) {
            QTextStream stream(stderr);
            stream << error << "\n";
            return 1;
        }
        return 0;
    }
    if (parser.isSet(galleryOption)) {
        GalleryWindow gallery;
        gallery.show();
        return application.exec();
    }
    if (parser.isSet(referenceOption)) {
        WorkspaceReference reference;
        reference.show();
        return application.exec();
    }

    const QString layoutDirectory =
        QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    const QString layoutFile = layoutDirectory + QStringLiteral("/workspace-layout.ini");
    WorkspaceWindow window(layoutFile);
    window.show();
    if (parser.isSet(sampleOption) || parser.isSet(sampleShotOption)) {
        window.showSample();
    }
    if (parser.isSet(sampleShotOption)) {
        QEventLoop loop;
        QTimer::singleShot(400, &loop, &QEventLoop::quit);
        loop.exec();
        QString error;
        if (!window.sampleViewport()->saveSampleImage(parser.value(sampleShotOption), &error)) {
            QTextStream stream(stderr);
            stream << error << "\n";
            return 1;
        }
        return 0;
    }
    return application.exec();
}
