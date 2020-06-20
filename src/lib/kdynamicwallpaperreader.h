/*
 * SPDX-FileCopyrightText: 2020 Vlad Zahorodnii <vlad.zahorodnii@kde.org>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#pragma once

#include "kdynamicwallpaper_export.h"

#include <QIODevice>

class KDynamicWallpaperMetaData;
class KDynamicWallpaperReaderPrivate;

class KDYNAMICWALLPAPER_EXPORT KDynamicWallpaperReader
{
public:
    enum WallpaperReaderError {
        NoError,
        DeviceError,
        InvalidDataError,
        NoMetaDataError,
        UnknownError,
    };

    KDynamicWallpaperReader();
    explicit KDynamicWallpaperReader(QIODevice *device);
    explicit KDynamicWallpaperReader(const QString &fileName);
    ~KDynamicWallpaperReader();

    void setDevice(QIODevice *device);
    QIODevice *device() const;

    void setFileName(const QString &fileName);
    QString fileName() const;

    int imageCount() const;

    KDynamicWallpaperMetaData metaDataAt(int imageIndex) const;
    QImage imageAt(int imageIndex) const;

    WallpaperReaderError error() const;
    QString errorString() const;

    static bool canRead(QIODevice *device);
    static bool canRead(const QString &fileName);

private:
    QScopedPointer<KDynamicWallpaperReaderPrivate> d;
};
