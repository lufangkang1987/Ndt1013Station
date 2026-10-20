#include "Station.h"
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QStandardPaths>
#include <QMutexLocker>
#include <QJsonParseError>
#include <QtEndian>
#include <cmath>
#include <windows.h>

std::atomic<Station*> Station::active{nullptr};
QImage FrameProvider::requestImage(const QString &, QSize *size, const QSize &) {
    QMutexLocker lock(&mutex);
    if(size) *size=frame.size();
    return frame;
}
void FrameProvider::setFrame(const QImage &image) { QMutexLocker lock(&mutex); frame=image; }
Station::Station(FrameProvider *provider,QObject *parent):QObject(parent),m_provider(provider) {
    m_timeout.setSingleShot(true);
    connect(&m_timeout,&QTimer::timeout,this,[this]{
        m_status=QStringLiteral("命令超时 · 请断开后重连");
        log(QStringLiteral("命令超时，保留忙状态以防止重入；可断开设备重建会话"));
    });
}
Station::~Station() { release(); }
QString Station::imageUrl() const { return m_frames ? QStringLiteral("image://detector/frame/%1").arg(m_frames):QString(); }
void Station::log(const QString &text) {
    m_logs=QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss "))+text+QLatin1Char('\n')+m_logs;
    m_logs=m_logs.left(18000); emit changed();
}
void Station::connectDevice(const QString &workDir) {
    if(m_connected || busy()) return;
    if(m_simulation) { m_connected=true; m_status=QStringLiteral("模拟设备就绪"); log(QStringLiteral("模拟模式：未连接硬件，生成的图像不用于检测")); return; }
    const QString path=QDir::cleanPath(workDir);
    if(!QFileInfo::exists(path+QStringLiteral("/config.ini"))) { log(QStringLiteral("工作目录缺少 config.ini")); return; }
    const QString sdk=QCoreApplication::applicationDirPath()+QStringLiteral("/sdk");
    SetDllDirectoryW(reinterpret_cast<LPCWSTR>(QDir::toNativeSeparators(sdk).utf16()));
    m_library.setFileName(sdk+QStringLiteral("/FpdSys.dll"));
    if(!m_library.load()) { log(QStringLiteral("SDK 加载失败（检查 VC++ 2013 x64）：")+m_library.errorString()); return; }
    m_create=reinterpret_cast<FnCreate>(m_library.resolve("Create"));
    m_destroy=reinterpret_cast<FnDestroy>(m_library.resolve("Destroy"));
    m_get=reinterpret_cast<FnGetAttr>(m_library.resolve("GetAttr"));
    m_invoke=reinterpret_cast<FnInvoke>(m_library.resolve("Invoke"));
    if(!m_create || !m_destroy || !m_get || !m_invoke) { log(QStringLiteral("SDK 导出函数不完整")); release(); return; }
    active.store(this);
    const QByteArray local=QDir::toNativeSeparators(path).toLocal8Bit();
    const int ret=m_create(local.constData(),&Station::callback,&m_id);
    if(ret!=Err_OK) { m_id=0; log(QStringLiteral("Create 失败：%1").arg(ret)); release(); return; }
    invoke(Cmd_Connect);
}
void Station::invoke(int command) {
    if(!m_id || !m_invoke || busy()) return;
    m_pending=command;
    m_status=QStringLiteral("正在执行命令 %1").arg(command);
    const int ret=m_invoke(m_id,command,nullptr,0);
    log(QStringLiteral("Invoke %1，返回 %2").arg(command).arg(ret));
    if(ret==Err_OK) event(Evt_TaskResult_Succeed,command,0,QString());
    else if(ret==Err_TaskPending) m_timeout.start(30000);
    else event(Evt_TaskResult_Failed,command,ret,QString());
}
void Station::release() {
    m_timeout.stop();
    // Destroy must quiesce SDK callbacks before object destruction/unloading.
    if(m_id && m_destroy) m_destroy(m_id);
    active.store(nullptr);
    m_id=0; m_pending=0; m_connected=false;
    m_create=nullptr; m_destroy=nullptr; m_get=nullptr; m_invoke=nullptr;
    if(m_library.isLoaded()) m_library.unload();
}
void Station::disconnectDevice() { release(); m_status=QStringLiteral("已断开"); log(QStringLiteral("设备会话已释放")); }
void Station::capture() {
    if(!m_connected || busy()) return;
    if(!m_simulation) { invoke(Cmd_ClearAcq); return; }
    QImage image(1024,768,QImage::Format_Grayscale16);
    for(int y=0;y<image.height();++y) {
        auto *row=reinterpret_cast<quint16*>(image.scanLine(y));
        for(int x=0;x<image.width();++x) {
            const double d=std::hypot(x-512.,y-384.);
            row[x]=quint16(qBound(0.,18000.+x*25.+4000.*std::sin(d/24.)+((std::abs(x-510)<18)?9000.:0.),65535.));
        }
    }
    acceptFrame(image); log(QStringLiteral("已生成模拟灰度图像"));
}
void Station::readStatus() {
    if(m_simulation) { log(QStringLiteral("模拟设备：1024 × 768 / 16 bit")); return; }
    if(!m_id || !m_get || busy()) return;
    for(int attr:{Attr_State,Attr_Width,Attr_Height}) {
        IRayVariant value{};
        const int ret=m_get(m_id,attr,&value);
        log(QStringLiteral("属性 %1：返回 %2，值 %3").arg(attr).arg(ret).arg(ret==0?QString::number(value.val.nVal):QStringLiteral("未知")));
    }
}
void Station::callback(int id,int evt,int,const char *message,int p1,int p2,int,void *data) {
    auto *self=active.load(); if(!self) return;
    if(evt==Evt_Image && data) {
        if(self->m_frameQueued.exchange(true)) return; // bounded preview mailbox
        const auto *img=static_cast<IRayImage*>(data);
        const qint64 bytes=qint64(img->nWidth)*img->nHeight*img->nBytesPerPixel;
        if(img->pData && img->nWidth>0 && img->nHeight>0 && img->nWidth<=20000 && img->nHeight<=20000 && bytes<=256*1024*1024 && (img->nBytesPerPixel==1 || img->nBytesPerPixel==2)) {
            QImage copy(img->nWidth,img->nHeight,img->nBytesPerPixel==2?QImage::Format_Grayscale16:QImage::Format_Grayscale8);
            const auto *src=reinterpret_cast<const uchar*>(img->pData);
            for(int y=0;y<img->nHeight;++y) memcpy(copy.scanLine(y),src+qsizetype(y)*img->nWidth*img->nBytesPerPixel,size_t(img->nWidth)*img->nBytesPerPixel);
            QMetaObject::invokeMethod(self,[self,id,copy]{ if(id==self->m_id) self->acceptFrame(copy); self->m_frameQueued=false; },Qt::QueuedConnection);
        } else self->m_frameQueued=false;
        return;
    }
    const QString text=QString::fromLocal8Bit(message?message:"");
    QMetaObject::invokeMethod(self,[self,id,evt,p1,p2,text]{if(id==self->m_id) self->event(evt,p1,p2,text);},Qt::QueuedConnection);
}
void Station::event(int evt,int command,int error,const QString &message) {
    if(evt==Evt_TaskResult_Succeed || evt==Evt_TaskResult_Failed || evt==Evt_TaskResult_Canceled) {
        if(command!=m_pending) return;
        m_timeout.stop(); m_pending=0;
        if(evt==Evt_TaskResult_Succeed) {
            if(command==Cmd_Connect) m_connected=true;
            m_status=m_connected?QStringLiteral("设备就绪"):QStringLiteral("未连接");
        } else m_status=QStringLiteral("命令失败/取消：%1，错误 %2").arg(command).arg(error);
    } else if(evt==Evt_CommFailure) { m_connected=false; m_status=QStringLiteral("通信异常 · 请断开后重连"); }
    else if(evt==Evt_Exp_Enable) m_status=QStringLiteral("设备允许曝光 · 等待图像");
    log(QStringLiteral("事件 %1，命令/参数 %2，错误/参数 %3 %4").arg(evt).arg(command).arg(error).arg(message));
}
void Station::acceptFrame(const QImage &image) {
    m_original=image; m_imageSimulation=m_simulation; m_imageImported=false;
    m_imageSource=m_simulation?QStringLiteral("Generated simulation"):QStringLiteral("SDK output; correction state not verified");
    ++m_frames;
    m_provider->setFrame(image.convertToFormat(QImage::Format_Grayscale8));
    m_status=QStringLiteral("图像已接收：%1 × %2").arg(image.width()).arg(image.height()); emit changed();
}
void Station::saveFrame() {
    if(m_original.isNull()) { log(QStringLiteral("没有可保存的图像")); return; }
    const QString dir=QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)+QStringLiteral("/Ndt1013Station");
    if(!QDir().mkpath(dir)) { log(QStringLiteral("无法创建保存目录")); return; }
    const QString base=dir+QLatin1Char('/')+QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmmss_zzz"));
    QSaveFile raw(base+QStringLiteral(".raw"));
    if(!raw.open(QIODevice::WriteOnly)) { log(raw.errorString()); return; }
    const int bpp=m_original.format()==QImage::Format_Grayscale16?2:1;
    for(int y=0;y<m_original.height();++y) {
        const qint64 count=qint64(m_original.width())*bpp;
        if(raw.write(reinterpret_cast<const char*>(m_original.constScanLine(y)),count)!=count) { raw.cancelWriting(); log(QStringLiteral("图像写入失败")); return; }
    }
    if(!raw.commit()) { log(QStringLiteral("图像提交失败")); return; }
    QJsonObject info{{QStringLiteral("width"),m_original.width()},{QStringLiteral("height"),m_original.height()},
        {QStringLiteral("bytesPerPixel"),bpp},{QStringLiteral("byteOrder"),QStringLiteral("little-endian")},
        {QStringLiteral("simulation"),m_imageSimulation},{QStringLiteral("frame"),m_frames},
        {QStringLiteral("source"),m_imageSource}};
    QSaveFile metadata(base+QStringLiteral(".json"));
    const auto json=QJsonDocument(info).toJson();
    if(!metadata.open(QIODevice::WriteOnly) || metadata.write(json)!=json.size() || !metadata.commit()) { log(QStringLiteral("元数据保存失败；RAW 未完整归档：")+base); return; }
    log(QStringLiteral("已保存 RAW 与 JSON：")+base);
}

bool Station::openRaw(const QUrl &fileUrl) {
    auto fail=[this](const QString &reason){ log(QStringLiteral("打开 RAW 失败：")+reason); return false; };
    if(m_connected || busy()) return fail(QStringLiteral("请先断开设备，再打开离线图像"));
    if(!fileUrl.isLocalFile()) return fail(QStringLiteral("请选择本地 RAW 文件"));
    const QFileInfo file(fileUrl.toLocalFile());
    if(file.suffix().compare(QStringLiteral("raw"),Qt::CaseInsensitive)!=0) return fail(QStringLiteral("文件扩展名必须是 .raw"));
    const QString jsonPath=file.dir().filePath(file.completeBaseName()+QStringLiteral(".json"));
    QFile metadata(jsonPath);
    if(!metadata.open(QIODevice::ReadOnly)) return fail(QStringLiteral("无法读取同名 JSON：")+jsonPath);
    if(metadata.size()>1024*1024) return fail(QStringLiteral("JSON 文件过大"));
    QJsonParseError parseError;
    const auto doc=QJsonDocument::fromJson(metadata.readAll(),&parseError);
    if(parseError.error!=QJsonParseError::NoError || !doc.isObject()) return fail(QStringLiteral("JSON 格式错误"));
    const auto obj=doc.object();
    // Reject fractional, missing, negative or oversized dimensions before allocation.
    auto integer=[&obj](const QString &key,int limit){
        const auto value=obj.value(key);
        const double n=value.toDouble(-1);
        return value.isDouble() && n>=1 && n<=limit && n==std::floor(n)?int(n):-1;
    };
    const int width=integer(QStringLiteral("width"),20000);
    const int height=integer(QStringLiteral("height"),20000);
    const int bpp=integer(QStringLiteral("bytesPerPixel"),2);
    const QString order=obj.value(QStringLiteral("byteOrder")).toString();
    if(width<1 || height<1 || bpp<1) return fail(QStringLiteral("JSON 宽高或每像素字节数无效，仅支持 8/16 位无符号灰度"));
    if(order!=QStringLiteral("little-endian") && order!=QStringLiteral("big-endian")) return fail(QStringLiteral("JSON byteOrder 必须为 little-endian 或 big-endian"));
    if(!obj.value(QStringLiteral("simulation")).isBool()) return fail(QStringLiteral("JSON 缺少 simulation 布尔标识，无法确认图像来源"));
    const qint64 expected=qint64(width)*height*bpp;
    if(expected>256*1024*1024) return fail(QStringLiteral("图像超过 256 MiB 大小限制"));
    QFile raw(file.absoluteFilePath());
    if(!raw.open(QIODevice::ReadOnly)) return fail(raw.errorString());
    if(raw.size()!=expected) return fail(QStringLiteral("RAW 文件大小与 JSON 不符：预期 %1，实际 %2 字节").arg(expected).arg(raw.size()));
    QImage image(width,height,bpp==2?QImage::Format_Grayscale16:QImage::Format_Grayscale8);
    if(image.isNull()) return fail(QStringLiteral("无法分配图像内存"));
    const qint64 rowBytes=qint64(width)*bpp;
    for(int y=0;y<height;++y) {
        auto *row=image.scanLine(y);
        if(raw.read(reinterpret_cast<char*>(row),rowBytes)!=rowBytes) return fail(QStringLiteral("RAW 读取不完整"));
        if(bpp==2) {
            auto *pixels=reinterpret_cast<quint16*>(row);
            for(int x=0;x<width;++x) pixels[x]=order==QStringLiteral("little-endian")?qFromLittleEndian<quint16>(row+x*2):qFromBigEndian<quint16>(row+x*2);
        }
    }
    // Only replace the current frame after the entire pair has passed validation.
    acceptFrame(image);
    m_imageSimulation=obj.value(QStringLiteral("simulation")).toBool();
    m_imageImported=true;
    m_imageSource=obj.value(QStringLiteral("source")).toString(QStringLiteral("Imported RAW; provenance unverified"));
    m_status=QStringLiteral("已打开 RAW：%1 × %2 / %3 位").arg(width).arg(height).arg(bpp*8);
    log(QStringLiteral("已打开：%1；%2").arg(file.absoluteFilePath(),m_imageSimulation?QStringLiteral("模拟图像"):QStringLiteral("文件标记为非模拟，未经设备验证")));
    return true;
}
