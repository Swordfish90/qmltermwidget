#include <QtTest>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>

#include "TerminalDisplay.h"

class TestTerminalDisplay final : public Konsole::TerminalDisplay
{
public:
    using Konsole::TerminalDisplay::dragEnterEvent;
    using Konsole::TerminalDisplay::dropEvent;
};

class DragDropTest final : public QObject
{
    Q_OBJECT

private slots:
    void acceptsDrops();
    void insertsLocalFilePath();
    void insertsQuotedUrls();
};

void DragDropTest::acceptsDrops()
{
    TestTerminalDisplay display;

    QVERIFY(display.flags() & QQuickItem::ItemAcceptsDrops);
}

void DragDropTest::insertsLocalFilePath()
{
    TestTerminalDisplay display;
    QByteArray inserted;

    QObject::connect(&display, &Konsole::TerminalDisplay::sendStringToEmu,
                     [&inserted](const char *text) { inserted = text; });

    QMimeData mimeData;
    mimeData.setUrls({QUrl::fromLocalFile(QStringLiteral("/tmp/image with spaces.png"))});
    QDropEvent event(QPointF(0, 0), Qt::CopyAction, &mimeData,
                     Qt::LeftButton, Qt::NoModifier);
    event.ignore();

    display.dropEvent(&event);

    QCOMPARE(inserted, QByteArray("'/tmp/image with spaces.png' "));
    QVERIFY(event.isAccepted());
    QCOMPARE(event.dropAction(), Qt::CopyAction);
}

void DragDropTest::insertsQuotedUrls()
{
    TestTerminalDisplay display;
    QByteArray inserted;
    QObject::connect(&display, &Konsole::TerminalDisplay::sendStringToEmu,
                     [&inserted](const char *text) { inserted = text; });

    display.insertDroppedUrls({QUrl::fromLocalFile(QStringLiteral("/tmp/Jaco's image.png")),
                               QUrl::fromLocalFile(QStringLiteral("/tmp/second image.png"))});
    QCOMPARE(inserted, QByteArray("'/tmp/Jaco'\\''s image.png' '/tmp/second image.png' "));
}

QTEST_MAIN(DragDropTest)
#include "tst_dragdrop.moc"
