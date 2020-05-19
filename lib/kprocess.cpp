/*
 * This file is a part of QTerminal - http://gitorious.org/qterminal
 *
 * This file was un-linked from KDE and modified
 * by Maxim Bourmistrov <maxim@unixconn.com>
 *
 */

/*
    This file is part of the KDE libraries

    Copyright (C) 2007 Oswald Buddenhagen <ossi@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "kprocess.h"

#include <qfile.h>

#ifdef Q_OS_WIN
# include <windows.h>
#else
# include <unistd.h>
# include <errno.h>
#endif

#ifndef Q_OS_WIN
# define STD_OUTPUT_HANDLE 1
# define STD_ERROR_HANDLE 2
#endif

#ifdef _WIN32_WCE
#include <stdio.h>
#endif

void KTermProcessPrivate::writeAll(const QByteArray &buf, int fd)
{
#ifdef Q_OS_WIN
#ifndef _WIN32_WCE
    HANDLE h = GetStdHandle(fd);
    if (h) {
        DWORD wr;
        WriteFile(h, buf.data(), buf.size(), &wr, 0);
    }
#else
    fwrite(buf.data(), 1, buf.size(), (FILE*)fd);
#endif
#else
    int off = 0;
    do {
        int ret = ::write(fd, buf.data() + off, buf.size() - off);
        if (ret < 0) {
            if (errno != EINTR)
                return;
        } else {
            off += ret;
        }
    } while (off < buf.size());
#endif
}

void KTermProcessPrivate::forwardStd(KTermProcess::ProcessChannel good, int fd)
{
    Q_Q(KTermProcess);

    QProcess::ProcessChannel oc = q->readChannel();
    q->setReadChannel(good);
    writeAll(q->readAll(), fd);
    q->setReadChannel(oc);
}

void KTermProcessPrivate::_k_forwardStdout()
{
#ifndef _WIN32_WCE
    forwardStd(KTermProcess::StandardOutput, STD_OUTPUT_HANDLE);
#else
    forwardStd(KTermProcess::StandardOutput, (int)stdout);
#endif
}

void KTermProcessPrivate::_k_forwardStderr()
{
#ifndef _WIN32_WCE
    forwardStd(KTermProcess::StandardError, STD_ERROR_HANDLE);
#else
    forwardStd(KTermProcess::StandardError, (int)stderr);
#endif
}

/////////////////////////////
// public member functions //
/////////////////////////////

KTermProcess::KTermProcess(QObject *parent) :
    QProcess(parent),
    d_ptr(new KTermProcessPrivate)
{
    d_ptr->q_ptr = this;
    setOutputChannelMode(ForwardedChannels);
}

KTermProcess::KTermProcess(KTermProcessPrivate *d, QObject *parent) :
    QProcess(parent),
    d_ptr(d)
{
    d_ptr->q_ptr = this;
    setOutputChannelMode(ForwardedChannels);
}

KTermProcess::~KTermProcess()
{
    delete d_ptr;
}

void KTermProcess::setOutputChannelMode(OutputChannelMode mode)
{
    Q_D(KTermProcess);

    d->outputChannelMode = mode;
    disconnect(this, SIGNAL(readyReadStandardOutput()));
    disconnect(this, SIGNAL(readyReadStandardError()));
    switch (mode) {
    case OnlyStdoutChannel:
        connect(this, SIGNAL(readyReadStandardError()), SLOT(_k_forwardStderr()));
        break;
    case OnlyStderrChannel:
        connect(this, SIGNAL(readyReadStandardOutput()), SLOT(_k_forwardStdout()));
        break;
    default:
        QProcess::setProcessChannelMode((ProcessChannelMode)mode);
        return;
    }
    QProcess::setProcessChannelMode(QProcess::SeparateChannels);
}

KTermProcess::OutputChannelMode KTermProcess::outputChannelMode() const
{
    Q_D(const KTermProcess);

    return d->outputChannelMode;
}

void KTermProcess::setNextOpenMode(QIODevice::OpenMode mode)
{
    Q_D(KTermProcess);

    d->openMode = mode;
}

#define DUMMYENV "_KPROCESS_DUMMY_="

void KTermProcess::clearEnvironment()
{
    setEnvironment(QStringList() << QString::fromLatin1(DUMMYENV));
}

void KTermProcess::setEnv(const QString &name, const QString &value, bool overwrite)
{
    QStringList env = environment();
    if (env.isEmpty()) {
        env = systemEnvironment();
        env.removeAll(QString::fromLatin1(DUMMYENV));
    }
    QString fname(name);
    fname.append(QLatin1Char('='));
    for (QStringList::Iterator it = env.begin(); it != env.end(); ++it)
        if ((*it).startsWith(fname)) {
            if (overwrite) {
                *it = fname.append(value);
                setEnvironment(env);
            }
            return;
        }
    env.append(fname.append(value));
    setEnvironment(env);
}

void KTermProcess::unsetEnv(const QString &name)
{
    QStringList env = environment();
    if (env.isEmpty()) {
        env = systemEnvironment();
        env.removeAll(QString::fromLatin1(DUMMYENV));
    }
    QString fname(name);
    fname.append(QLatin1Char('='));
    for (QStringList::Iterator it = env.begin(); it != env.end(); ++it)
        if ((*it).startsWith(fname)) {
            env.erase(it);
            if (env.isEmpty())
                env.append(QString::fromLatin1(DUMMYENV));
            setEnvironment(env);
            return;
        }
}

void KTermProcess::setProgram(const QString &exe, const QStringList &args)
{
    Q_D(KTermProcess);

    d->prog = exe;
    d->args = args;
#ifdef Q_OS_WIN
    setNativeArguments(QString());
#endif
}

void KTermProcess::setProgram(const QStringList &argv)
{
    Q_D(KTermProcess);

    Q_ASSERT( !argv.isEmpty() );
    d->args = argv;
    d->prog = d->args.takeFirst();
#ifdef Q_OS_WIN
    setNativeArguments(QString());
#endif
}

KTermProcess &KTermProcess::operator<<(const QString &arg)
{
    Q_D(KTermProcess);

    if (d->prog.isEmpty())
        d->prog = arg;
    else
        d->args << arg;
    return *this;
}

KTermProcess &KTermProcess::operator<<(const QStringList &args)
{
    Q_D(KTermProcess);

    if (d->prog.isEmpty())
        setProgram(args);
    else
        d->args << args;
    return *this;
}

void KTermProcess::clearProgram()
{
    Q_D(KTermProcess);

    d->prog.clear();
    d->args.clear();
#ifdef Q_OS_WIN
    setNativeArguments(QString());
#endif
}

#if 0
void KTermProcess::setShellCommand(const QString &cmd)
{
    Q_D(KTermProcess);

    KShell::Errors err;
    d->args = KShell::splitArgs(
            cmd, KShell::AbortOnMeta | KShell::TildeExpand, &err);
    if (err == KShell::NoError && !d->args.isEmpty()) {
        d->prog = KStandardDirs::findExe(d->args[0]);
        if (!d->prog.isEmpty()) {
            d->args.removeFirst();
#ifdef Q_OS_WIN
            setNativeArguments(QString());
#endif
            return;
        }
    }

    d->args.clear();

#ifdef Q_OS_UNIX
// #ifdef NON_FREE // ... as they ship non-POSIX /bin/sh
# if !defined(__linux__) && !defined(__FreeBSD__) && !defined(__NetBSD__) && !defined(__OpenBSD__) && !defined(__DragonFly__) && !defined(__GNU__)
    // If /bin/sh is a symlink, we can be pretty sure that it points to a
    // POSIX shell - the original bourne shell is about the only non-POSIX
    // shell still in use and it is always installed natively as /bin/sh.
    d->prog = QFile::symLinkTarget(QString::fromLatin1("/bin/sh"));
    if (d->prog.isEmpty()) {
        // Try some known POSIX shells.
        d->prog = KStandardDirs::findExe(QString::fromLatin1("ksh"));
        if (d->prog.isEmpty()) {
            d->prog = KStandardDirs::findExe(QString::fromLatin1("ash"));
            if (d->prog.isEmpty()) {
                d->prog = KStandardDirs::findExe(QString::fromLatin1("bash"));
                if (d->prog.isEmpty()) {
                    d->prog = KStandardDirs::findExe(QString::fromLatin1("zsh"));
                    if (d->prog.isEmpty())
                        // We're pretty much screwed, to be honest ...
                        d->prog = QString::fromLatin1("/bin/sh");
                }
            }
        }
    }
# else
    d->prog = QString::fromLatin1("/bin/sh");
# endif

    d->args << QString::fromLatin1("-c") << cmd;
#else // Q_OS_UNIX
    // KMacroExpander::expandMacrosShellQuote(), KShell::quoteArg() and
    // KShell::joinArgs() may generate these for security reasons.
    setEnv(PERCENT_VARIABLE, QLatin1String("%"));

#ifndef _WIN32_WCE
    WCHAR sysdir[MAX_PATH + 1];
    UINT size = GetSystemDirectoryW(sysdir, MAX_PATH + 1);
    d->prog = QString::fromUtf16((const ushort *) sysdir, size);
    d->prog += QLatin1String("\\cmd.exe");
    setNativeArguments(QLatin1String("/V:OFF /S /C \"") + cmd + QLatin1Char('"'));
#else
    d->prog = QLatin1String("\\windows\\cmd.exe");
    setNativeArguments(QLatin1String("/S /C \"") + cmd + QLatin1Char('"'));
#endif
#endif
}
#endif
QStringList KTermProcess::program() const
{
    Q_D(const KTermProcess);

    QStringList argv = d->args;
    argv.prepend(d->prog);
    return argv;
}

void KTermProcess::start()
{
    Q_D(KTermProcess);

    QProcess::start(d->prog, d->args, d->openMode);
}

int KTermProcess::execute(int msecs)
{
    start();
    if (!waitForFinished(msecs)) {
        kill();
        waitForFinished(-1);
        return -2;
    }
    return (exitStatus() == QProcess::NormalExit) ? exitCode() : -1;
}

// static
int KTermProcess::execute(const QString &exe, const QStringList &args, int msecs)
{
    KTermProcess p;
    p.setProgram(exe, args);
    return p.execute(msecs);
}

// static
int KTermProcess::execute(const QStringList &argv, int msecs)
{
    KTermProcess p;
    p.setProgram(argv);
    return p.execute(msecs);
}

int KTermProcess::startDetached()
{
    Q_D(KTermProcess);

    qint64 pid;
    if (!QProcess::startDetached(d->prog, d->args, workingDirectory(), &pid))
        return 0;
    return (int) pid;
}

// static
int KTermProcess::startDetached(const QString &exe, const QStringList &args)
{
    qint64 pid;
    if (!QProcess::startDetached(exe, args, QString(), &pid))
        return 0;
    return (int) pid;
}

// static
int KTermProcess::startDetached(const QStringList &argv)
{
    QStringList args = argv;
    QString prog = args.takeFirst();
    return startDetached(prog, args);
}

int KTermProcess::pid() const
{
#ifdef Q_OS_UNIX
    return (int) QProcess::pid();
#else
    return QProcess::pid() ? QProcess::pid()->dwProcessId : 0;
#endif
}

