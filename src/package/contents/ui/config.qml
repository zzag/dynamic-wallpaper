/*
 * SPDX-FileCopyrightText: 2020 Vlad Zahorodnii <vlad.zahorodnii@kde.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

import QtQuick 2.5
import QtQuick.Dialogs 1.3
import QtQuick.Layouts 1.0
import QtQuick.Controls 1.0 as QtControls
import QtQuick.Controls 2.3 as QtControls2
import QtPositioning 5.12

import org.kde.kcm 1.1 as KCM
import org.kde.kirigami 2.10 as Kirigami
import com.github.zzag.plasma.wallpapers.dynamic 1.0

ColumnLayout {
    id: root

    property int cfg_FillMode
    property string cfg_Image
    property int cfg_UpdateInterval
    property alias cfg_AutoDetectLocation: autoDetectLocationCheckBox.checked
    property alias cfg_ManualLatitude: latitudeSpinBox.value
    property alias cfg_ManualLongitude: longitudeSpinBox.value

    function saveConfig() {
        wallpapersModel.purge();
    }

    Kirigami.FormLayout {
        twinFormLayouts: parentLayout

        QtControls2.ComboBox {
            id: positioningComboBox
            Kirigami.FormData.label: i18n("Fill Mode:")
            textRole: "text"
            model: [
                {
                    "text": i18n("Scaled and Cropped"),
                    "value": Image.PreserveAspectCrop
                },
                {
                    "text": i18n("Scaled"),
                    "value": Image.Stretch
                },
                {
                    "text": i18n("Scaled, Keep Proportions"),
                    "value": Image.PreserveAspectFit
                },
                {
                    "text": i18n("Centered"),
                    "value": Image.Pad
                },
                {
                    "text": i18n("Tiled"),
                    "value": Image.Tile
                }
            ]

            onActivated: cfg_FillMode = model[currentIndex]["value"]

            Component.onCompleted: {
                for (var i = 0; i < model.length; i++) {
                    if (model[i]["value"] === wallpaper.configuration.FillMode) {
                        positioningComboBox.currentIndex = i;
                        break;
                    }
                }
            }
        }

        QtControls2.CheckBox {
            id: autoDetectLocationCheckBox
            text: i18n("Automatically detect location")
        }

        QtControls.SpinBox {
            Kirigami.FormData.label: i18n("Latitude:")
            enabled: !autoDetectLocationCheckBox.checked
            visible: autoDetectLocationCheckBox.checked
            decimals: 2
            minimumValue: -90
            maximumValue: 90
            value: automaticLocationProvider.position.coordinate.latitude
        }

        QtControls.SpinBox {
            Kirigami.FormData.label: i18n("Longitude:")
            enabled: !autoDetectLocationCheckBox.checked
            visible: autoDetectLocationCheckBox.checked
            decimals: 2
            minimumValue: -180
            maximumValue: 180
            value: automaticLocationProvider.position.coordinate.longitude
        }

        QtControls.SpinBox {
            id: latitudeSpinBox
            Kirigami.FormData.label: i18n("Latitude:")
            decimals: 2
            minimumValue: -90
            maximumValue: 90
            visible: !autoDetectLocationCheckBox.checked
        }

        QtControls.SpinBox {
            id: longitudeSpinBox
            Kirigami.FormData.label: i18n("Longitude:")
            decimals: 2
            minimumValue: -180
            maximumValue: 180
            visible: !autoDetectLocationCheckBox.checked
        }

        QtControls.SpinBox {
            id: updateIntervalSpinBox
            Kirigami.FormData.label: i18n("Update Every:")
            maximumValue: 360
            minimumValue: 1
            suffix: i18np(" minute", " minutes", value)
            onValueChanged: cfg_UpdateInterval = value * 60000
            Component.onCompleted: value = wallpaper.configuration.UpdateInterval / 60000
        }
    }

    KCM.GridView {
        id: wallpapersGrid
        Layout.fillWidth: true
        Layout.fillHeight: true

        function resetCurrentIndex() {
            view.currentIndex = wallpapersModel.find(cfg_Image);
        }

        view.model: wallpapersModel
        view.delegate: KCM.GridDelegate {
            hoverEnabled: true
            opacity: model.zombie ? 0.5 : 1
            text: model.name
            toolTip: {
                if (model.author && model.license)
                    return i18nc("<image> by <author> (<license>)", "By %1 (%2)", model.author, model.license);
                if (model.license)
                    return i18nc("<image> (<license>)", "%1 (%2)", model.name, model.license);
                return model.name;
            }
            actions: [
                Kirigami.Action {
                    icon.name: "document-open-folder"
                    tooltip: i18n("Open Containing Folder")
                    onTriggered: Qt.openUrlExternally(model.folder)
                },
                Kirigami.Action {
                    icon.name: "edit-undo"
                    tooltip: i18n("Restore Wallpaper")
                    visible: model.zombie
                    onTriggered: wallpapersModel.unscheduleRemove(wallpapersModel.modelIndex(index))
                },
                Kirigami.Action {
                    icon.name: "edit-delete"
                    tooltip: i18n("Remove Wallpaper")
                    visible: !model.zombie && model.removable
                    onTriggered: wallpapersModel.scheduleRemove(wallpapersModel.modelIndex(index))
                }
            ]
            thumbnail: Image {
                anchors.fill: parent
                fillMode: cfg_FillMode
                source: model.preview
            }
            onClicked: {
                cfg_Image = model.image;
                wallpapersGrid.forceActiveFocus();
            }
        }

        Connections {
            target: wallpapersModel
            onRowsInserted: Qt.callLater(wallpapersGrid.resetCurrentIndex)
            onRowsRemoved: Qt.callLater(wallpapersGrid.resetCurrentIndex)
        }
        Connections {
            target: root
            onCfg_ImageChanged: Qt.callLater(wallpapersGrid.resetCurrentIndex)
        }
    }

    Loader {
        id: wallpaperDialogLoader
        active: false
        sourceComponent: FileDialog {
            title: i18n("Open Wallpaper")
            folder: shortcuts.home
            nameFilters: [i18n("HEIF Image Files (*.heic *.heif)")]
            onAccepted: {
                wallpapersModel.add(fileUrl);
                wallpaperDialogLoader.active = false;
            }
            onRejected: {
                wallpaperDialogLoader.active = false;
            }
            Component.onCompleted: open()
        }
    }

    RowLayout {
        Layout.alignment: Qt.AlignRight

        QtControls2.Button {
            icon.name: "list-add"
            text: i18n("Add Wallpaper...")
            onClicked: wallpaperDialogLoader.active = true
        }
    }

    PositionSource {
        id: automaticLocationProvider
        active: autoDetectLocationCheckBox.checked
    }

    DynamicWallpaperModel {
        id: wallpapersModel
    }

    Component.onCompleted: wallpapersModel.reload()
}
