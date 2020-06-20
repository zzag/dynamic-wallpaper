/*
 * SPDX-FileCopyrightText: 2020 Vlad Zahorodnii <vlad.zahorodnii@kde.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "config-dynamicwallpaper.h"

#include "dynamicwallpaperhandler.h"
#include "dynamicwallpaperdescription.h"
#include "dynamicwallpaperengine_solar.h"
#include "dynamicwallpaperengine_timed.h"

#include <KConfigGroup>
#include <KLocalizedString>
#include <KPackage/PackageLoader>
#include <KSharedConfig>

DynamicWallpaperHandler::DynamicWallpaperHandler(QObject *parent)
    : QObject(parent)
    , m_updateTimer(new QTimer(this))
{
    m_updateTimer->setInterval(0);
    m_updateTimer->setSingleShot(true);
    connect(m_updateTimer, &QTimer::timeout, this, &DynamicWallpaperHandler::update);
}

DynamicWallpaperHandler::~DynamicWallpaperHandler()
{
    delete m_engine;
}

void DynamicWallpaperHandler::setLocation(const QGeoCoordinate &coordinate)
{
    if (m_location == coordinate)
        return;
    m_location = coordinate;
    reloadEngine();
    scheduleUpdate();
    emit locationChanged();
}

QGeoCoordinate DynamicWallpaperHandler::location() const
{
    return m_location;
}

static QUrl locateWallpaper(const QString &name)
{
    const QString packagePath = QStandardPaths::locate(QStandardPaths::GenericDataLocation,
                                                       QStringLiteral("wallpapers/") + name,
                                                       QStandardPaths::LocateDirectory);

    KPackage::PackageLoader *packageLoader = KPackage::PackageLoader::self();
    KPackage::Package package = packageLoader->loadPackage(QStringLiteral("Wallpaper/Dynamic"));
    package.setPath(packagePath);
    if (package.isValid())
        return package.fileUrl(QByteArrayLiteral("dynamic"));

    const QString filePath = QStandardPaths::locate(QStandardPaths::GenericDataLocation,
                                                    QStringLiteral("wallpapers/") + name,
                                                    QStandardPaths::LocateFile);

    return QUrl::fromLocalFile(filePath);
}

static QUrl defaultLookAndFeelWallpaper()
{
    KConfigGroup kdeConfigGroup(KSharedConfig::openConfig(QStringLiteral("kdeglobals")), "KDE");
    const QString lookAndFeelPackageName = kdeConfigGroup.readEntry("LookAndFeelPackage");

    KPackage::PackageLoader *packageLoader = KPackage::PackageLoader::self();
    KPackage::Package lookAndFeelPackage =
            packageLoader->loadPackage(QStringLiteral("Plasma/LookAndFeel"));
    if (!lookAndFeelPackageName.isEmpty())
        lookAndFeelPackage.setPath(lookAndFeelPackageName);

    KSharedConfigPtr lookAndFeelConfig =
            KSharedConfig::openConfig(lookAndFeelPackage.filePath("defaults"));
    KConfigGroup wallpaperConfigGroup = KConfigGroup(lookAndFeelConfig, "Dynamic Wallpaper");

    const QString wallpaperName = wallpaperConfigGroup.readEntry("Image");
    if (wallpaperName.isEmpty())
        return QUrl();

    return locateWallpaper(wallpaperName);
}

static QUrl defaultFallbackWallpaper()
{
    return locateWallpaper(QStringLiteral(FALLBACK_WALLPAPER));
}

static QUrl defaultWallpaper()
{
    QUrl fileUrl = defaultLookAndFeelWallpaper();
    if (fileUrl.isValid())
        return fileUrl;
    return defaultFallbackWallpaper();
}

void DynamicWallpaperHandler::setSource(const QUrl &url)
{
    const QUrl source = url.isValid() ? url : defaultWallpaper();
    if (m_source == source)
        return;
    m_source = source;
    reloadDescription();
    reloadEngine();
    scheduleUpdate();
    emit sourceChanged();
}

QUrl DynamicWallpaperHandler::source() const
{
    return m_source;
}

void DynamicWallpaperHandler::setTopLayer(const QUrl &url)
{
    if (m_topLayer == url)
        return;
    m_topLayer = url;
    emit topLayerChanged();
}

QUrl DynamicWallpaperHandler::topLayer() const
{
    return m_topLayer;
}

void DynamicWallpaperHandler::setBottomLayer(const QUrl &url)
{
    if (m_bottomLayer == url)
        return;
    m_bottomLayer = url;
    emit bottomLayerChanged();
}

QUrl DynamicWallpaperHandler::bottomLayer() const
{
    return m_bottomLayer;
}

void DynamicWallpaperHandler::setBlendFactor(qreal blendFactor)
{
    if (m_blendFactor == blendFactor)
        return;
    m_blendFactor = blendFactor;
    emit blendFactorChanged();
}

qreal DynamicWallpaperHandler::blendFactor() const
{
    return m_blendFactor;
}

void DynamicWallpaperHandler::setStatus(Status status)
{
    if (m_status == status)
        return;
    m_status = status;
    emit statusChanged();
}

DynamicWallpaperHandler::Status DynamicWallpaperHandler::status() const
{
    return m_status;
}

void DynamicWallpaperHandler::setErrorString(const QString &text)
{
    if (m_errorString == text)
        return;
    m_errorString = text;
    emit errorStringChanged();
}

QString DynamicWallpaperHandler::errorString() const
{
    return m_errorString;
}

void DynamicWallpaperHandler::scheduleUpdate()
{
    m_updateTimer->start();
}

void DynamicWallpaperHandler::update()
{
    if (m_status != Ready)
        return;
    if (!m_engine || m_engine->isExpired())
        reloadEngine();
    m_engine->update();
    setTopLayer(m_engine->topLayer());
    setBottomLayer(m_engine->bottomLayer());
    setBlendFactor(m_engine->blendFactor());
}

void DynamicWallpaperHandler::reloadDescription()
{
    const QString fileName = m_source.toLocalFile();

    m_description = DynamicWallpaperDescription::fromFile(fileName);

    if (m_description.isValid()) {
        setStatus(Ready);
    } else {
        setErrorString(i18n("%1 is not a dynamic wallpaper", fileName));
        setStatus(Error);
    }
}

void DynamicWallpaperHandler::reloadEngine()
{
    delete m_engine;
    m_engine = nullptr;

    if (!m_description.isValid())
        return;

    if (m_description.supportedEngines() & DynamicWallpaperDescription::SolarEngine)
        m_engine = SolarDynamicWallpaperEngine::create(m_location);
    if (!m_engine)
        m_engine = TimedDynamicWallpaperEngine::create();

    m_engine->setDescription(m_description);
}
