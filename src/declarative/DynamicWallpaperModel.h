/*
 * SPDX-FileCopyrightText: 2020 Vlad Zahorodnii <vladzzag@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

// Own
#include "SunPath.h"
#include "SunPosition.h"

// Qt
#include <QDateTime>
#include <QGeoCoordinate>
#include <QUrl>
#include <QVector>

// std
#include <memory>

class DynamicWallpaperPackage;

class DynamicWallpaperModel
{
public:
    explicit DynamicWallpaperModel(std::shared_ptr<DynamicWallpaperPackage> wallpaper);
    virtual ~DynamicWallpaperModel();

    /**
     * Returns whether the model is no longer actual and must be destroyed.
     *
     * Default implementation returns @c false.
     */
    virtual bool isExpired() const;

    /**
     * Returns the path to image that has to be displayed in the bottom layer.
     */
    QUrl bottomLayer() const;

    /**
     * Returns the path to image that has to be displayed in the top layer.
     */
    QUrl topLayer() const;

    /**
     * Returns the blend factor between the bottom and the top layer.
     *
     * A value of 0.0 is equivalent to displaying only the bottom layer.
     *
     * A value of 1.0 is equivalent to displaying only the top layer.
     */
    qreal blendFactor() const;

    /**
     * Updates the internal state of the dynamic wallpaper model.
     */
    virtual void update() = 0;

    /**
     * Returns the dynamic wallpaper package associated with this model.
     */
    DynamicWallpaperPackage *wallpaper() const;

protected:
    struct Knot
    {
        bool operator<(const Knot &other) const { return time < other.time; }
        bool operator<=(const Knot &other) const { return time <= other.time; }

        qreal time;
        QUrl url;
    };

    QVector<Knot> m_knots;
    qreal m_time = 0;

private:
    Knot currentBottomKnot() const;
    Knot currentTopKnot() const;

    std::shared_ptr<DynamicWallpaperPackage> m_wallpaper;
};

class SolarDynamicWallpaperModel final : public DynamicWallpaperModel
{
public:
    bool isExpired() const override;
    void update() override;

    static SolarDynamicWallpaperModel *create(std::shared_ptr<DynamicWallpaperPackage> wallpaper,
                                              const QGeoCoordinate &location);

private:
    SolarDynamicWallpaperModel(std::shared_ptr<DynamicWallpaperPackage> wallpaper,
                               const QDateTime &dateTime,
                               const QGeoCoordinate &location,
                               const SunPath &path,
                               const SunPosition &midnight);

    SunPath m_sunPath;
    SunPosition m_midnight;
    QDateTime m_dateTime;
    QGeoCoordinate m_location;
};

class TimedDynamicWallpaperModel final : public DynamicWallpaperModel
{
public:
    void update() override;

    static TimedDynamicWallpaperModel *create(std::shared_ptr<DynamicWallpaperPackage> wallpaper);

private:
    TimedDynamicWallpaperModel(std::shared_ptr<DynamicWallpaperPackage> wallpaper);
};