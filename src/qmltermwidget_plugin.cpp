#include "qmltermwidget_plugin.h"

#include "ColorScheme.h"
#include "TerminalDisplay.h"
#include "ksession.h"

#include <qqml.h>
#include <QQmlEngine>
#include <QDir>

using namespace Konsole;

static QObject *colorschememanager_provider(QQmlEngine *engine, QJSEngine *scriptEngine)
{
    Q_UNUSED(engine)
    Q_UNUSED(scriptEngine)

    return ColorSchemeManager::instance();
}

void QmltermwidgetPlugin::registerTypes(const char *uri)
{
    // @uri org.qterminal.qmlterminal
    qmlRegisterType<TerminalDisplay>(uri, 1, 0, "QMLTermWidget");
    qmlRegisterType<KSession>(uri, 1, 0, "QMLTermSession");
    qmlRegisterUncreatableType<const Konsole::ColorScheme>(uri, 1, 0, "ColorScheme", QStringLiteral("Not instantiatable"));
    qmlRegisterSingletonType<ColorSchemeManager>(uri, 1, 0, "ColorSchemeManager", colorschememanager_provider);
}

void QmltermwidgetPlugin::initializeEngine(QQmlEngine *engine, const char *uri)
{
    QQmlExtensionPlugin::initializeEngine(engine, uri);

    QStringList pwds = engine->importPathList();

    if (!pwds.empty()){

        QString cs, kbl;

        foreach (QString pwd, pwds) {
            cs  = pwd + "/QMLTermWidget/color-schemes";
            kbl = pwd + "/QMLTermWidget/kb-layouts";
            if (QDir(cs).exists()) break;
        }

        setenv("KB_LAYOUT_DIR",kbl.toUtf8().constData(),1);
        setenv("COLORSCHEMES_DIR",cs.toUtf8().constData(),1);
    }
}
