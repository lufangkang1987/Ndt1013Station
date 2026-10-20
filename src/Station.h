#pragma once
#include <QObject>
#include <QImage>
#include <QLibrary>
#include <QMutex>
#include <QQuickImageProvider>
#include <QTimer>
#include <QUrl>
#include <atomic>
#include "IRayFpdSys.h"

class FrameProvider : public QQuickImageProvider {
public:
    FrameProvider() : QQuickImageProvider(Image) {}
    QImage requestImage(const QString &, QSize *size, const QSize &) override;
    void setFrame(const QImage &image);
private:
    QMutex mutex;
    QImage frame;
};

class Station : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString status READ status NOTIFY changed)
    Q_PROPERTY(QString logs READ logs NOTIFY changed)
    Q_PROPERTY(QString imageUrl READ imageUrl NOTIFY changed)
    Q_PROPERTY(bool connected READ connected NOTIFY changed)
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    Q_PROPERTY(bool simulation MEMBER m_simulation NOTIFY changed)
    Q_PROPERTY(int frames READ frames NOTIFY changed)
    Q_PROPERTY(bool imageSimulation READ imageSimulation NOTIFY changed)
    Q_PROPERTY(bool imageImported READ imageImported NOTIFY changed)
public:
    explicit Station(FrameProvider *provider, QObject *parent=nullptr);
    ~Station() override;
    QString status() const { return m_status; }
    QString logs() const { return m_logs; }
    QString imageUrl() const;
    bool connected() const { return m_connected; }
    bool busy() const { return m_pending != 0; }
    int frames() const { return m_frames; }
    bool imageSimulation() const { return m_imageSimulation; }
    bool imageImported() const { return m_imageImported; }
    Q_INVOKABLE void connectDevice(const QString &workDir);
    Q_INVOKABLE void disconnectDevice();
    Q_INVOKABLE void capture();
    Q_INVOKABLE void readStatus();
    Q_INVOKABLE void saveFrame();
    Q_INVOKABLE bool openRaw(const QUrl &fileUrl);
signals:
    void changed();
private:
    static void callback(int,int,int,const char*,int,int,int,void*);
    void event(int eventId, int command, int error, const QString &message);
    void acceptFrame(const QImage &frame);
    void log(const QString &text);
    void invoke(int command);
    void release();
    FrameProvider *m_provider;
    QLibrary m_library;
    FnCreate m_create=nullptr;
    FnDestroy m_destroy=nullptr;
    FnGetAttr m_get=nullptr;
    FnInvoke m_invoke=nullptr;
    int m_id=0, m_pending=0, m_frames=0;
    bool m_connected=false, m_simulation=true;
    bool m_imageSimulation=true;
    bool m_imageImported=false;
    QString m_imageSource;
    QString m_status=QStringLiteral("离线 · 模拟模式"), m_logs;
    QImage m_original;
    QTimer m_timeout;
    std::atomic_bool m_frameQueued{false};
    static std::atomic<Station*> active;
};
