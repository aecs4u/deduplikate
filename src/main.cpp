#include <QApplication>
#include <QCommandLineParser>
#include <KAboutData>
#include <KLocalizedString>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    KLocalizedString::setApplicationDomain("deduplikate");

    KAboutData aboutData(
        QStringLiteral("deduplikate"),
        i18n("Deduplikate"),
        QStringLiteral("1.0.0"),
        i18n("Find and remove duplicate files using czkawka backend"),
        KAboutLicense::GPL_V3,
        i18n("Copyright (C) 2026"),
        QString(),
        QStringLiteral("https://github.com/yourusername/deduplikate")
    );

    aboutData.addAuthor(
        i18n("Your Name"),
        i18n("Developer"),
        QStringLiteral("your.email@example.com")
    );

    aboutData.setOrganizationDomain("aecs4u.it");

    KAboutData::setApplicationData(aboutData);
    app.setWindowIcon(QIcon::fromTheme(QStringLiteral("edit-find")));

    QCommandLineParser parser;
    aboutData.setupCommandLine(&parser);
    parser.process(app);
    aboutData.processCommandLine(&parser);

    MainWindow *window = new MainWindow();
    window->show();

    return app.exec();
}
