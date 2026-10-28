import QtQuick
import QtQuick.Window

Window {
    id: window

    width: 1040
    height: 720
    minimumWidth: 760
    minimumHeight: 580
    visible: true
    title: "Everstead Launcher v" + launcherState.launcherVersion
    color: window.tc("#091012")

    property string page: "profiles"
    property int editorIndex: -1
    property bool editorIsNew: false
    property bool versionMenuOpen: false
    property var editorProfile: editorIndex >= 0 && editorIndex < launcherState.profiles.length ? launcherState.profiles[editorIndex] : ({})
    property var versionChoices: ["Latest"].concat(launcherState.installedVersions)

    property var themes: [
        { name: "forest", label: "Forest", hue: 0.0, saturation: 1.0 },
        { name: "ocean", label: "Ocean", hue: 0.18, saturation: 1.0 },
        { name: "amethyst", label: "Amethyst", hue: 0.36, saturation: 1.0 },
        { name: "ember", label: "Ember", hue: -0.3, saturation: 1.0 },
        { name: "graphite", label: "Graphite", hue: 0.0, saturation: 0.08 }
    ]
    property var activeTheme: themes.find(theme => theme.name === launcherState.theme)
    property bool minimizedForGame: false

    function tc(value) {
        return themeColor(value, activeTheme)
    }

    // We leave reds and greys alone so error text and neutral colors read the same in every theme
    function themeColor(value, theme) {
        let color = Qt.lighter(value, 1.0)
        if (color.hslHue < 0.15 || color.hslHue > 0.9) {
            return color
        }

        return Qt.hsla((color.hslHue + theme.hue + 1) % 1, color.hslSaturation * theme.saturation, color.hslLightness, color.a)
    }

    Connections {
        target: launcherState

        function onGameRunningChanged() {
            if (launcherState.gameRunning && launcherState.minimizeWhilePlaying && !launcherState.logWindowVisible) {
                window.minimizedForGame = true
                window.showMinimized()
            }
            else if (!launcherState.gameRunning && window.minimizedForGame) {
                window.minimizedForGame = false
                window.showNormal()
                window.raise()
                window.requestActivate()
            }
        }
    }

    function openEditor(index, isNewProfile) {
        editorIndex = index
        editorIsNew = isNewProfile
        versionMenuOpen = false
        page = "editor"
    }

    function closeEditor() {
        versionMenuOpen = false
        editorIndex = -1
        page = "profiles"
    }

    function cancelEditor() {
        if (editorIsNew) {
            launcherState.DeleteProfile(editorIndex)
        }

        closeEditor()
    }

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: window.tc("#11271f") }
            GradientStop { position: 0.55; color: window.tc("#0b1517") }
            GradientStop { position: 1.0; color: window.tc("#080c10") }
        }
    }

    Rectangle {
        width: 700
        height: 700
        radius: width / 2
        x: -width * 0.48
        y: window.height - height * 0.38
        color: window.tc("#258a62")
        opacity: 0.11
    }

    Text {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.leftMargin: 42
        anchors.topMargin: 34
        color: window.tc("#edf9f2")
        font.pixelSize: 25
        font.weight: Font.DemiBold
        font.letterSpacing: 4.8
        text: "EVERSTEAD"
    }

    Row {
        id: mainNavigation

        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 31
        z: 10
        spacing: 8
        visible: window.page !== "editor"

        Rectangle {
            width: 112
            height: 40
            radius: 9
            color: window.page === "profiles" ? window.tc("#315346") : profilesNavigationMouse.containsMouse ? window.tc("#203b32") : window.tc("#172a26")
            border.width: 1
            border.color: window.page === "profiles" ? window.tc("#63d796") : window.tc("#315346")
            scale: profilesNavigationMouse.pressed ? 0.96 : 1

            Behavior on color { ColorAnimation { duration: 120 } }
            Behavior on border.color { ColorAnimation { duration: 120 } }
            Behavior on scale { NumberAnimation { duration: 100; easing.type: Easing.OutCubic } }

            Text {
                anchors.centerIn: parent
                color: window.page === "profiles" ? window.tc("#eaf8f0") : window.tc("#9db7aa")
                font.pixelSize: 12
                font.weight: Font.DemiBold
                font.letterSpacing: 1.1
                text: "PROFILES"

                Behavior on color { ColorAnimation { duration: 120 } }
            }

            MouseArea {
                id: profilesNavigationMouse

                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: window.page = "profiles"
            }
        }

        Rectangle {
            width: 124
            height: 40
            radius: 9
            color: window.page === "changelog" ? window.tc("#315346") : changelogNavigationMouse.containsMouse ? window.tc("#203b32") : window.tc("#172a26")
            border.width: 1
            border.color: window.page === "changelog" ? window.tc("#63d796") : window.tc("#315346")
            scale: changelogNavigationMouse.pressed ? 0.96 : 1

            Behavior on color { ColorAnimation { duration: 120 } }
            Behavior on border.color { ColorAnimation { duration: 120 } }
            Behavior on scale { NumberAnimation { duration: 100; easing.type: Easing.OutCubic } }

            Text {
                anchors.centerIn: parent
                color: window.page === "changelog" ? window.tc("#eaf8f0") : window.tc("#9db7aa")
                font.pixelSize: 12
                font.weight: Font.DemiBold
                font.letterSpacing: 1.1
                text: "CHANGELOG"

                Behavior on color { ColorAnimation { duration: 120 } }
            }

            MouseArea {
                id: changelogNavigationMouse

                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: window.page = "changelog"
            }
        }

        Rectangle {
            width: 118
            height: 40
            radius: 9
            color: window.page === "versions" ? window.tc("#315346") : versionsNavigationMouse.containsMouse ? window.tc("#203b32") : window.tc("#172a26")
            border.width: 1
            border.color: window.page === "versions" ? window.tc("#63d796") : window.tc("#315346")
            scale: versionsNavigationMouse.pressed ? 0.96 : 1

            Behavior on color { ColorAnimation { duration: 120 } }
            Behavior on border.color { ColorAnimation { duration: 120 } }
            Behavior on scale { NumberAnimation { duration: 100; easing.type: Easing.OutCubic } }

            Text {
                anchors.centerIn: parent
                color: window.page === "versions" ? window.tc("#eaf8f0") : window.tc("#9db7aa")
                font.pixelSize: 12
                font.weight: Font.DemiBold
                font.letterSpacing: 1.1
                text: "VERSIONS"

                Behavior on color { ColorAnimation { duration: 120 } }
            }

            MouseArea {
                id: versionsNavigationMouse

                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: window.page = "versions"
            }
        }
    }

    Rectangle {
        id: settingsButton

        width: 38
        height: 34
        radius: 9
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.rightMargin: 42
        anchors.topMargin: 31
        z: 10
        visible: window.page !== "editor"
        color: window.page === "settings" ? window.tc("#315346") : settingsButtonMouse.containsMouse ? window.tc("#203b32") : window.tc("#172a26")
        border.width: 1
        border.color: window.page === "settings" ? window.tc("#63d796") : window.tc("#315346")

        Behavior on color { ColorAnimation { duration: 120 } }

        Canvas {
            id: gearCanvas

            property color gearColor: window.page === "settings" ? window.tc("#eaf8f0") : window.tc("#9db7aa")

            width: 18
            height: 18
            anchors.centerIn: parent
            onGearColorChanged: requestPaint()
            onPaint: {
                let context = getContext("2d")
                context.reset()
                context.fillStyle = gearColor
                context.translate(width / 2, height / 2)
                for (let i = 0; i < 8; i++) {
                    context.fillRect(-2, -9, 4, 5)
                    context.rotate(Math.PI / 4)
                }

                context.beginPath()
                context.arc(0, 0, 6.5, 0, Math.PI * 2)
                context.arc(0, 0, 2.6, 0, Math.PI * 2, true)
                context.fill()
            }
        }

        MouseArea {
            id: settingsButtonMouse

            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: window.page = window.page === "settings" ? "profiles" : "settings"
        }
    }

    Text {
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.rightMargin: 18
        anchors.bottomMargin: 14
        z: 10
        color: window.tc("#60766b")
        font.pixelSize: 11
        text: "v" + launcherState.launcherVersion
    }

    Item {
        id: profilesPage

        anchors.fill: parent
        visible: opacity > 0
        enabled: window.page === "profiles"
        opacity: window.page === "profiles" ? 1 : 0
        scale: window.page === "profiles" ? 1 : 0.985

        Behavior on opacity { NumberAnimation { duration: 170; easing.type: Easing.OutCubic } }
        Behavior on scale { NumberAnimation { duration: 170; easing.type: Easing.OutCubic } }

        Column {
            width: Math.min(parent.width - 64, 760)
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: 118
            spacing: 0

            Row {
                width: parent.width

                Text {
                    color: window.tc("#eff9f3")
                    font.pixelSize: 31
                    font.weight: Font.DemiBold
                    text: "Profiles"
                }

                Item { width: parent.width - newProfileButton.width - openFolderButton.width - 120; height: 1 }

                Rectangle {
                    id: openFolderButton

                    width: 142
                    height: 44
                    radius: 10
                    color: openFolderMouse.containsMouse ? window.tc("#24382c") : window.tc("#1a2a20")
                    scale: openFolderMouse.pressed ? 0.97 : openFolderMouse.containsMouse ? 1.025 : 1

                    Behavior on color { ColorAnimation { duration: 120 } }
                    Behavior on scale { NumberAnimation { duration: 110; easing.type: Easing.OutCubic } }

                    Text {
                        anchors.centerIn: parent
                        color: window.tc("#eff9f3")
                        font.pixelSize: 13
                        font.weight: Font.DemiBold
                        font.letterSpacing: 1.3
                        text: "OPEN FOLDER"
                    }

                    MouseArea {
                        id: openFolderMouse

                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: launcherState.OpenDataDirectory()
                    }
                }

                Item { width: 10; height: 1 }

                Rectangle {
                    id: newProfileButton

                    width: 142
                    height: 44
                    radius: 10
                    color: newProfileMouse.containsMouse ? window.tc("#78e5a8") : window.tc("#63d796")
                    scale: newProfileMouse.pressed ? 0.97 : newProfileMouse.containsMouse ? 1.025 : 1

                    Behavior on color { ColorAnimation { duration: 120 } }
                    Behavior on scale { NumberAnimation { duration: 110; easing.type: Easing.OutCubic } }

                    Text {
                        anchors.centerIn: parent
                        color: window.tc("#07120b")
                        font.pixelSize: 13
                        font.weight: Font.DemiBold
                        font.letterSpacing: 1.3
                        text: "+ NEW PROFILE"
                    }

                    MouseArea {
                        id: newProfileMouse

                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            launcherState.CreateProfile()
                            window.openEditor(launcherState.profiles.length - 1, true)
                        }
                    }
                }
            }

            Text {
                width: parent.width
                topPadding: 10
                color: window.tc("#94afa1")
                font.pixelSize: 15
                text: "Choose a profile to play."
            }

            Item { width: 1; height: 28 }

            Text {
                visible: launcherState.profiles.length === 0
                width: parent.width
                color: window.tc("#b1c7bc")
                font.pixelSize: 17
                horizontalAlignment: Text.AlignHCenter
                text: launcherState.loading ? "Loading available versions" : "Create a profile to get started."
            }

            Flickable {
                id: profilesFlickable

                width: parent.width
                height: Math.min(contentHeight, window.height - 250)
                contentWidth: width
                contentHeight: profileColumn.height
                clip: true
                visible: launcherState.profiles.length > 0
                boundsBehavior: Flickable.StopAtBounds

                WheelHandler {
                    target: null
                    onWheel: event => {
                        let delta = event.pixelDelta.y !== 0 ? event.pixelDelta.y : event.angleDelta.y
                        let maximumContentY = profilesFlickable.originY
                            + Math.max(0, profilesFlickable.contentHeight - profilesFlickable.height)
                        profilesFlickable.contentY = Math.max(profilesFlickable.originY,
                            Math.min(maximumContentY, profilesFlickable.contentY - delta))
                        event.accepted = true
                    }
                }

                Column {
                    id: profileColumn

                    width: parent.width
                    spacing: 12

                    Repeater {
                        model: launcherState.profiles

                        delegate: Rectangle {
                            required property var modelData
                            required property int index
                            property bool profileBusy: launcherState.loading && launcherState.activeProfile === index
                            property bool profileFailed: !launcherState.loading && launcherState.error.length > 0
                                && launcherState.activeProfile === index

                            width: profileColumn.width
                            height: profileBusy || profileFailed ? 120 : 94
                            radius: 14
                            color: window.tc("#142321")
                            border.width: 1
                            border.color: launcherState.gameRunning && launcherState.activeProfile === index || profileFailed
                                ? window.tc("#d86c6c")
                                : profileBusy ? window.tc("#4b8a6d") : window.tc("#35594a")

                            Behavior on height { NumberAnimation { duration: 160; easing.type: Easing.OutCubic } }
                            Behavior on border.color { ColorAnimation { duration: 140 } }

                            Column {
                                anchors.left: parent.left
                                anchors.leftMargin: 22
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.verticalCenterOffset: profileBusy ? -7 : 0
                                width: parent.width - profileActions.width - 66
                                spacing: 5

                                Behavior on anchors.verticalCenterOffset { NumberAnimation { duration: 160; easing.type: Easing.OutCubic } }

                                Text {
                                    color: window.tc("#f0f9f3")
                                    font.pixelSize: 20
                                    font.weight: Font.DemiBold
                                    text: modelData.name
                                }

                                Text {
                                    color: window.tc("#95afa2")
                                    font.pixelSize: 13
                                    text: modelData.version === "Latest" && launcherState.latestVersion.length > 0
                                        ? "Latest · " + launcherState.latestVersion
                                        : modelData.version
                                }

                                Text {
                                    width: parent.width
                                    visible: profileBusy || profileFailed
                                    color: profileFailed ? window.tc("#f0a5a5") : window.tc("#82cba4")
                                    font.pixelSize: 12
                                    elide: Text.ElideRight
                                    text: profileFailed ? launcherState.error : launcherState.status
                                }
                            }

                            Row {
                                id: profileActions

                                anchors.right: parent.right
                                anchors.rightMargin: 16
                                anchors.verticalCenter: parent.verticalCenter
                                spacing: 10

                                Rectangle {
                                    width: 78
                                    height: 42
                                    radius: 9
                                    color: editProfileMouse.containsMouse ? window.tc("#315346") : window.tc("#263f36")
                                    scale: editProfileMouse.pressed ? 0.96 : editProfileMouse.containsMouse ? 1.035 : 1

                                    Behavior on color { ColorAnimation { duration: 120 } }
                                    Behavior on scale { NumberAnimation { duration: 100; easing.type: Easing.OutCubic } }

                                    Text {
                                        anchors.centerIn: parent
                                        color: window.tc("#c8dcd1")
                                        font.pixelSize: 12
                                        font.weight: Font.DemiBold
                                        text: "EDIT"
                                    }

                                    MouseArea {
                                        id: editProfileMouse

                                        anchors.fill: parent
                                        enabled: !launcherState.gameRunning && !launcherState.loading
                                        hoverEnabled: true
                                        cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
                                        onClicked: window.openEditor(index, false)
                                    }
                                }

                                Rectangle {
                                    width: 116
                                    height: 42
                                    radius: 9
                                    color: launcherState.gameRunning && launcherState.activeProfile === index
                                        ? (playProfileMouse.containsMouse ? window.tc("#e47b7b") : window.tc("#d86c6c"))
                                        : profileBusy
                                            ? window.tc("#3f705a")
                                        : (playProfileMouse.containsMouse ? window.tc("#78e5a8") : window.tc("#63d796"))
                                    scale: playProfileMouse.pressed ? 0.96 : playProfileMouse.containsMouse && playProfileMouse.enabled ? 1.035 : 1

                                    Behavior on color { ColorAnimation { duration: 120 } }
                                    Behavior on scale { NumberAnimation { duration: 100; easing.type: Easing.OutCubic } }

                                    Text {
                                        anchors.centerIn: parent
                                        color: window.tc("#07120b")
                                        font.pixelSize: 12
                                        font.weight: Font.DemiBold
                                        font.letterSpacing: 1.1
                                        text: launcherState.gameRunning && launcherState.activeProfile === index
                                            ? "STOP"
                                            : profileBusy || profileFailed ? launcherState.actionText : "PLAY"
                                    }

                                    MouseArea {
                                        id: playProfileMouse

                                        anchors.fill: parent
                                        enabled: !launcherState.loading && (!launcherState.gameRunning || launcherState.activeProfile === index)
                                        hoverEnabled: true
                                        cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
                                        onClicked: launcherState.PlayProfile(index)
                                    }
                                }
                            }

                            Rectangle {
                                id: profileProgressTrack

                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.bottom: parent.bottom
                                anchors.leftMargin: 22
                                anchors.rightMargin: 22
                                anchors.bottomMargin: 12
                                height: 5
                                radius: 3
                                visible: profileBusy
                                color: window.tc("#223a32")
                                clip: true

                                Rectangle {
                                    height: parent.height
                                    width: launcherState.progress > 0
                                        ? parent.width * Math.min(1, launcherState.progress)
                                        : Math.min(72, parent.width)
                                    x: launcherState.progress > 0 ? 0 : (parent.width - width) / 2
                                    radius: 3
                                    color: window.tc("#63d796")
                                    opacity: launcherState.progress > 0 ? 1 : 0.55

                                    Behavior on width { NumberAnimation { duration: 100; easing.type: Easing.OutCubic } }
                                    SequentialAnimation on opacity {
                                        running: profileBusy && launcherState.progress === 0
                                        loops: Animation.Infinite
                                        NumberAnimation { to: 1; duration: 600; easing.type: Easing.InOutSine }
                                        NumberAnimation { to: 0.4; duration: 600; easing.type: Easing.InOutSine }
                                    }
                                }
                            }
                        }
                    }
                }
            }

        }
    }

    Item {
        id: changelogPage

        anchors.fill: parent
        visible: opacity > 0
        enabled: window.page === "changelog"
        opacity: window.page === "changelog" ? 1 : 0
        scale: window.page === "changelog" ? 1 : 0.985

        Behavior on opacity { NumberAnimation { duration: 170; easing.type: Easing.OutCubic } }
        Behavior on scale { NumberAnimation { duration: 170; easing.type: Easing.OutCubic } }

        Item {
            width: Math.min(parent.width - 64, 760)
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: 102
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 42

            Text {
                anchors.centerIn: parent
                visible: launcherState.changelogLoading
                color: window.tc("#9fb7ab")
                font.pixelSize: 15
                text: "Loading changelog..."
            }

            Column {
                width: Math.min(parent.width - 48, 360)
                anchors.centerIn: parent
                spacing: 18
                visible: !launcherState.changelogLoading && launcherState.changelogError.length > 0

                Text {
                    width: parent.width
                    color: window.tc("#f0a5a5")
                    font.pixelSize: 15
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.Wrap
                    text: launcherState.changelogError
                }

                Rectangle {
                    width: 130
                    height: 42
                    anchors.horizontalCenter: parent.horizontalCenter
                    radius: 9
                    color: retryChangelogMouse.containsMouse ? window.tc("#78e5a8") : window.tc("#63d796")
                    scale: retryChangelogMouse.pressed ? 0.96 : retryChangelogMouse.containsMouse ? 1.025 : 1

                    Behavior on color { ColorAnimation { duration: 120 } }
                    Behavior on scale { NumberAnimation { duration: 100; easing.type: Easing.OutCubic } }

                    Text {
                        anchors.centerIn: parent
                        color: window.tc("#07120b")
                        font.pixelSize: 12
                        font.weight: Font.DemiBold
                        font.letterSpacing: 1.1
                        text: "TRY AGAIN"
                    }

                    MouseArea {
                        id: retryChangelogMouse

                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: launcherState.RefreshChangelog()
                    }
                }
            }

            Flickable {
                id: changelogView

                anchors.left: parent.left
                anchors.right: changelogScrollTrack.left
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.rightMargin: 18
                contentWidth: width
                contentHeight: changelogText.height
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                visible: !launcherState.changelogLoading && launcherState.changelogError.length === 0 && launcherState.changelog.length > 0

                TextEdit {
                    id: changelogText

                    width: changelogView.width
                    height: Math.max(changelogView.height, contentHeight)
                    readOnly: true
                    selectByMouse: true
                    textFormat: TextEdit.RichText
                    wrapMode: TextEdit.Wrap
                    color: window.tc("#d9e9e0")
                    selectionColor: window.tc("#376d55")
                    selectedTextColor: window.tc("#f3fbf6")
                    font.pixelSize: 15
                    text: launcherState.changelog
                    onLinkActivated: link => Qt.openUrlExternally(link)
                }
            }

            Rectangle {
                id: changelogScrollTrack

                width: 8
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                radius: 4
                visible: changelogView.visible && changelogView.contentHeight > changelogView.height + 1
                color: window.tc("#172622")

                property real scrollRange: Math.max(0, changelogView.contentHeight - changelogView.height)
                property real scrollProgress: scrollRange > 0
                    ? Math.max(0, Math.min(1, (changelogView.contentY - changelogView.originY) / scrollRange))
                    : 0

                Rectangle {
                    id: changelogScrollThumb

                    width: parent.width
                    height: Math.min(parent.height, Math.max(36, parent.height * changelogView.height / changelogView.contentHeight))
                    y: Math.max(0, Math.min(parent.height - height, (parent.height - height) * changelogScrollTrack.scrollProgress))
                    radius: 4
                    color: changelogScrollMouse.containsMouse || changelogScrollMouse.pressed ? window.tc("#67d59a") : window.tc("#496a5c")

                    Behavior on color { ColorAnimation { duration: 100 } }
                }

                MouseArea {
                    id: changelogScrollMouse

                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor

                    property real dragOffset: 0

                    function setScrollPosition(thumbPosition) {
                        let thumbRange = height - changelogScrollThumb.height
                        if (thumbRange <= 0) {
                            return
                        }

                        let progress = Math.max(0, Math.min(1, thumbPosition / thumbRange))
                        changelogView.contentY = changelogView.originY + progress * changelogScrollTrack.scrollRange
                    }

                    onPressed: mouse => {
                        dragOffset = mouse.y >= changelogScrollThumb.y && mouse.y <= changelogScrollThumb.y + changelogScrollThumb.height
                            ? mouse.y - changelogScrollThumb.y
                            : changelogScrollThumb.height / 2
                        setScrollPosition(mouse.y - dragOffset)
                    }
                    onPositionChanged: mouse => {
                        if (pressed) {
                            setScrollPosition(mouse.y - dragOffset)
                        }
                    }
                }
            }
        }
    }

    Item {
        id: versionsPage

        anchors.fill: parent
        visible: opacity > 0
        enabled: window.page === "versions"
        opacity: window.page === "versions" ? 1 : 0
        scale: window.page === "versions" ? 1 : 0.985

        Behavior on opacity { NumberAnimation { duration: 170; easing.type: Easing.OutCubic } }
        Behavior on scale { NumberAnimation { duration: 170; easing.type: Easing.OutCubic } }

        Item {
            width: Math.min(parent.width - 64, 760)
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: 102
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 42

            Text {
                id: versionsStatus

                width: parent.width
                anchors.top: parent.top
                color: launcherState.error.length > 0 ? window.tc("#f0a5a5") : window.tc("#9fb7ab")
                font.pixelSize: 13
                elide: Text.ElideRight
                text: launcherState.error.length > 0 ? launcherState.error : launcherState.loading ? launcherState.status : ""
            }

            Text {
                anchors.centerIn: parent
                visible: launcherState.versions.length === 0
                color: window.tc("#9fb7ab")
                font.pixelSize: 15
                text: launcherState.loading ? "Loading available versions" : "No versions available"
            }

            ListView {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: versionsStatus.bottom
                anchors.topMargin: 16
                anchors.bottom: parent.bottom
                spacing: 8
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                model: launcherState.versions

                delegate: Rectangle {
                    id: versionRow

                    required property string modelData

                    property bool isInstalled: launcherState.installedVersions.indexOf(modelData) >= 0
                    property bool isBusy: launcherState.loading && launcherState.selectedVersion === modelData

                    width: ListView.view.width
                    height: 62
                    radius: 10
                    color: window.tc("#182b29")
                    border.width: 1
                    border.color: isInstalled ? window.tc("#406756") : window.tc("#2a443b")

                    Rectangle {
                        anchors.left: parent.left
                        anchors.bottom: parent.bottom
                        anchors.margins: 1
                        height: 3
                        radius: 2
                        width: (parent.width - 2) * Math.min(1, launcherState.progress)
                        visible: versionRow.isBusy && launcherState.progress > 0
                        color: window.tc("#63d796")

                        Behavior on width { NumberAnimation { duration: 100; easing.type: Easing.OutCubic } }
                    }

                    Column {
                        anchors.left: parent.left
                        anchors.leftMargin: 16
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 3

                        Text {
                            color: window.tc("#edf8f1")
                            font.pixelSize: 17
                            font.weight: Font.Medium
                            text: versionRow.modelData
                        }

                        Text {
                            color: window.tc("#8fa99c")
                            font.pixelSize: 11
                            font.letterSpacing: 1.1
                            text: (versionRow.modelData === launcherState.latestVersion ? "LATEST · " : "")
                                + (versionRow.isBusy ? "WORKING" : versionRow.isInstalled ? "INSTALLED" : "NOT INSTALLED")
                        }
                    }

                    Rectangle {
                        width: 112
                        height: 38
                        anchors.right: parent.right
                        anchors.rightMargin: 12
                        anchors.verticalCenter: parent.verticalCenter
                        radius: 9
                        opacity: versionActionMouse.enabled ? 1 : 0.45
                        color: versionRow.isInstalled
                            ? (versionActionMouse.containsMouse ? window.tc("#5a2a2a") : window.tc("#432222"))
                            : (versionActionMouse.containsMouse ? window.tc("#78e5a8") : window.tc("#63d796"))
                        scale: versionActionMouse.pressed ? 0.96 : 1

                        Behavior on color { ColorAnimation { duration: 120 } }
                        Behavior on scale { NumberAnimation { duration: 100; easing.type: Easing.OutCubic } }

                        Text {
                            anchors.centerIn: parent
                            color: versionRow.isInstalled ? window.tc("#f0c4c4") : window.tc("#07120b")
                            font.pixelSize: 12
                            font.weight: Font.DemiBold
                            font.letterSpacing: 1.1
                            text: versionRow.isBusy ? "INSTALLING" : versionRow.isInstalled ? "REMOVE" : "INSTALL"
                        }

                        MouseArea {
                            id: versionActionMouse

                            anchors.fill: parent
                            enabled: !launcherState.loading && !launcherState.gameRunning
                            hoverEnabled: true
                            cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
                            onClicked: {
                                if (versionRow.isInstalled) {
                                    launcherState.RemoveVersion(versionRow.modelData)
                                } else {
                                    launcherState.InstallVersion(versionRow.modelData)
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    Item {
        id: settingsPage

        anchors.fill: parent
        visible: opacity > 0
        enabled: window.page === "settings"
        opacity: window.page === "settings" ? 1 : 0
        scale: window.page === "settings" ? 1 : 0.985

        Behavior on opacity { NumberAnimation { duration: 170; easing.type: Easing.OutCubic } }
        Behavior on scale { NumberAnimation { duration: 170; easing.type: Easing.OutCubic } }

        Column {
            width: Math.min(parent.width - 64, 760)
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: 118
            spacing: 0

            Text {
                color: window.tc("#9ab5a6")
                font.pixelSize: 12
                font.weight: Font.DemiBold
                font.letterSpacing: 1.4
                text: "THEME"
            }

            Item { width: 1; height: 9 }

            Row {
                width: parent.width
                spacing: 10

                Repeater {
                    model: window.themes

                    Rectangle {
                        required property var modelData

                        property bool isActive: launcherState.theme === modelData.name

                        width: (parent.width - 40) / 5
                        height: 86
                        radius: 10
                        color: themeMouse.containsMouse ? window.themeColor("#203b32", modelData) : window.themeColor("#182b29", modelData)
                        border.width: isActive ? 2 : 1
                        border.color: isActive ? window.themeColor("#63d796", modelData) : window.themeColor("#406756", modelData)

                        Behavior on color { ColorAnimation { duration: 120 } }

                        Row {
                            anchors.horizontalCenter: parent.horizontalCenter
                            anchors.top: parent.top
                            anchors.topMargin: 16
                            spacing: 6

                            Repeater {
                                model: ["#63d796", "#315346", "#11271f"]

                                Rectangle {
                                    required property string modelData

                                    width: 16
                                    height: 16
                                    radius: 8
                                    color: window.themeColor(modelData, parent.parent.modelData)
                                }
                            }
                        }

                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            anchors.bottom: parent.bottom
                            anchors.bottomMargin: 16
                            color: parent.isActive ? window.themeColor("#eaf8f0", parent.modelData) : window.themeColor("#9db7aa", parent.modelData)
                            font.pixelSize: 13
                            font.weight: Font.DemiBold
                            text: parent.modelData.label
                        }

                        MouseArea {
                            id: themeMouse

                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: launcherState.theme = parent.modelData.name
                        }
                    }
                }
            }

            Item { width: 1; height: 26 }

            Text {
                color: window.tc("#9ab5a6")
                font.pixelSize: 12
                font.weight: Font.DemiBold
                font.letterSpacing: 1.4
                text: "WHILE PLAYING"
            }

            Item { width: 1; height: 9 }

            Rectangle {
                width: parent.width
                height: 58
                radius: 10
                color: window.tc("#182b29")
                border.width: 1
                border.color: window.tc("#406756")

                Text {
                    anchors.left: parent.left
                    anchors.leftMargin: 16
                    anchors.verticalCenter: parent.verticalCenter
                    color: window.tc("#dcebe3")
                    font.pixelSize: 15
                    text: "Minimize launcher while the game is running"
                }

                Rectangle {
                    width: 46
                    height: 26
                    radius: 13
                    anchors.right: parent.right
                    anchors.rightMargin: 16
                    anchors.verticalCenter: parent.verticalCenter
                    color: launcherState.minimizeWhilePlaying ? window.tc("#63d796") : window.tc("#31443e")

                    Behavior on color { ColorAnimation { duration: 150 } }

                    Rectangle {
                        width: 20
                        height: 20
                        radius: 10
                        anchors.verticalCenter: parent.verticalCenter
                        x: launcherState.minimizeWhilePlaying ? parent.width - width - 3 : 3
                        color: launcherState.minimizeWhilePlaying ? window.tc("#07120b") : window.tc("#a7bab0")

                        Behavior on x { NumberAnimation { duration: 160; easing.type: Easing.OutCubic } }
                        Behavior on color { ColorAnimation { duration: 150 } }
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: launcherState.minimizeWhilePlaying = !launcherState.minimizeWhilePlaying
                }
            }
        }
    }

    Item {
        id: editorPage

        anchors.fill: parent
        visible: opacity > 0
        enabled: window.page === "editor"
        opacity: window.page === "editor" ? 1 : 0
        scale: window.page === "editor" ? 1 : 0.985

        Behavior on opacity { NumberAnimation { duration: 170; easing.type: Easing.OutCubic } }
        Behavior on scale { NumberAnimation { duration: 170; easing.type: Easing.OutCubic } }

        Rectangle {
            width: Math.min(parent.width - 64, 700)
            height: Math.min(parent.height - 142, 560)
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: parent.verticalCenter
            radius: 18
            color: window.tc("#121f1e")
            border.width: 1
            border.color: window.tc("#35594a")

            Row {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 28

                Text {
                    color: window.tc("#eef8f2")
                    font.pixelSize: 23
                    font.weight: Font.DemiBold
                    text: window.editorIsNew ? "New profile" : "Edit profile"
                }

                Item { width: parent.width - closeEditorButton.width - 120; height: 1 }

                Rectangle {
                    id: closeEditorButton

                    width: 76
                    height: 34
                    radius: 8
                    color: closeEditorMouse.containsMouse ? window.tc("#315346") : window.tc("#263f36")
                    scale: closeEditorMouse.pressed ? 0.96 : 1

                    Behavior on color { ColorAnimation { duration: 120 } }
                    Behavior on scale { NumberAnimation { duration: 100; easing.type: Easing.OutCubic } }

                    Text {
                        anchors.centerIn: parent
                        color: window.tc("#bfd4c8")
                        font.pixelSize: 12
                        font.weight: Font.DemiBold
                        text: window.editorIsNew ? "CANCEL" : "BACK"
                    }

                    MouseArea {
                        id: closeEditorMouse

                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: window.cancelEditor()
                    }
                }
            }

            Flickable {
                id: editorFlickable

                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.bottom: editorActions.top
                anchors.leftMargin: 28
                anchors.rightMargin: 28
                anchors.topMargin: 82
                anchors.bottomMargin: 18
                contentWidth: width
                contentHeight: editorContent.height
                clip: true
                boundsBehavior: Flickable.StopAtBounds

                WheelHandler {
                    target: null
                    onWheel: event => {
                        let delta = event.pixelDelta.y !== 0 ? event.pixelDelta.y : event.angleDelta.y
                        let maximumContentY = editorFlickable.originY
                            + Math.max(0, editorFlickable.contentHeight - editorFlickable.height)
                        editorFlickable.contentY = Math.max(editorFlickable.originY,
                            Math.min(maximumContentY, editorFlickable.contentY - delta))
                        event.accepted = true
                    }
                }

                Column {
                    id: editorContent

                    width: parent.width
                    spacing: 0

                    Text {
                        color: window.tc("#9ab5a6")
                        font.pixelSize: 12
                        font.weight: Font.DemiBold
                        font.letterSpacing: 1.4
                        text: "PROFILE NAME"
                    }

                    Item { width: 1; height: 9 }

                    Rectangle {
                        width: parent.width
                        height: 52
                        radius: 10
                        color: window.tc("#182b29")
                        border.width: 1
                        border.color: window.tc("#406756")

                        TextInput {
                            id: profileNameInput

                            anchors.fill: parent
                            anchors.leftMargin: 15
                            anchors.rightMargin: 15
                            color: window.tc("#eef8f2")
                            font.pixelSize: 17
                            font.weight: Font.Medium
                            verticalAlignment: TextInput.AlignVCenter
                            selectByMouse: true
                            text: window.editorProfile.name || ""
                        }
                    }

                    Item { width: 1; height: 24 }

                    Text {
                        color: window.tc("#9ab5a6")
                        font.pixelSize: 12
                        font.weight: Font.DemiBold
                        font.letterSpacing: 1.4
                        text: "GAME VERSION"
                    }

                    Item { width: 1; height: 9 }

                    Rectangle {
                        id: editorVersionPicker

                        width: parent.width
                        height: 62
                        radius: 10
                        color: editorVersionMouse.containsMouse ? window.tc("#1d3430") : window.tc("#182b29")
                        border.width: 1
                        border.color: window.versionMenuOpen ? window.tc("#67d59a") : window.tc("#406756")

                        Behavior on color { ColorAnimation { duration: 120 } }
                        Behavior on border.color { ColorAnimation { duration: 120 } }

                        Text {
                            anchors.left: parent.left
                            anchors.leftMargin: 16
                            anchors.verticalCenter: parent.verticalCenter
                            color: window.tc("#edf8f1")
                            font.pixelSize: 17
                            font.weight: Font.Medium
                            text: window.editorProfile.version === "Latest" && launcherState.latestVersion.length > 0
                                ? "Latest · " + launcherState.latestVersion
                                : window.editorProfile.version || "Loading versions"
                        }

                        Text {
                            anchors.right: parent.right
                            anchors.rightMargin: 18
                            anchors.verticalCenter: parent.verticalCenter
                            color: window.tc("#b7d0c2")
                            font.pixelSize: 21
                            text: window.versionMenuOpen ? "⌃" : "⌄"
                        }

                        MouseArea {
                            id: editorVersionMouse

                            anchors.fill: parent
                            enabled: launcherState.versions.length > 0
                            hoverEnabled: true
                            cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
                            onClicked: window.versionMenuOpen = !window.versionMenuOpen
                        }
                    }

                    Rectangle {
                        id: editorVersionMenu

                        width: parent.width
                        height: window.versionMenuOpen ? Math.min(versionOptions.height + 12, 264) : 0
                        radius: 10
                        visible: height > 0
                        opacity: window.versionMenuOpen ? 1 : 0
                        color: window.tc("#1b302c")
                        border.width: 1
                        border.color: window.tc("#527965")
                        clip: true

                        Behavior on height { NumberAnimation { duration: 170; easing.type: Easing.OutCubic } }
                        Behavior on opacity { NumberAnimation { duration: 120 } }

                        Flickable {
                            anchors.fill: parent
                            anchors.margins: 6
                            contentWidth: width
                            contentHeight: versionOptions.height
                            clip: true

                            Column {
                                id: versionOptions

                                width: parent.width
                                Repeater {
                                    model: window.versionChoices

                                    delegate: Rectangle {
                                        required property string modelData

                                        width: versionOptions.width
                                        height: 48
                                        radius: 7
                                        color: versionOptionMouse.containsMouse ? window.tc("#315744") : "transparent"

                                        Behavior on color { ColorAnimation { duration: 100 } }

                                        Text {
                                            anchors.left: parent.left
                                            anchors.leftMargin: 13
                                            anchors.verticalCenter: parent.verticalCenter
                                            color: window.tc("#e5f1ea")
                                            font.pixelSize: 15
                                            text: modelData === "Latest" && launcherState.latestVersion.length > 0
                                                ? "Latest · " + launcherState.latestVersion
                                                : modelData
                                        }

                                        MouseArea {
                                            id: versionOptionMouse

                                            anchors.fill: parent
                                            hoverEnabled: true
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: {
                                                launcherState.SetProfileVersion(window.editorIndex, modelData)
                                                window.versionMenuOpen = false
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    Item { width: 1; height: 26 }

                    Text {
                        color: window.tc("#9ab5a6")
                        font.pixelSize: 12
                        font.weight: Font.DemiBold
                        font.letterSpacing: 1.4
                        text: "GAME LOGS"
                    }

                    Item { width: 1; height: 9 }

                    Rectangle {
                        width: parent.width
                        height: 58
                        radius: 10
                        color: window.tc("#182b29")
                        border.width: 1
                        border.color: window.tc("#406756")

                        Text {
                            anchors.left: parent.left
                            anchors.leftMargin: 16
                            anchors.verticalCenter: parent.verticalCenter
                            color: window.tc("#dcebe3")
                            font.pixelSize: 15
                            text: "Open log window when playing"
                        }

                        Rectangle {
                            id: gameLogsToggle

                            width: 46
                            height: 26
                            radius: 13
                            anchors.right: parent.right
                            anchors.rightMargin: 16
                            anchors.verticalCenter: parent.verticalCenter
                            color: window.editorProfile.showLogs ? window.tc("#63d796") : window.tc("#31443e")

                            Behavior on color { ColorAnimation { duration: 150 } }

                            Rectangle {
                                width: 20
                                height: 20
                                radius: 10
                                anchors.verticalCenter: parent.verticalCenter
                                x: window.editorProfile.showLogs ? parent.width - width - 3 : 3
                                color: window.editorProfile.showLogs ? window.tc("#07120b") : window.tc("#a7bab0")

                                Behavior on x { NumberAnimation { duration: 160; easing.type: Easing.OutCubic } }
                                Behavior on color { ColorAnimation { duration: 150 } }
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: launcherState.SetProfileShowLogs(window.editorIndex, !window.editorProfile.showLogs)
                        }
                    }

                    Item { width: 1; height: 26 }

                    Row {
                        width: parent.width

                        Text {
                            color: window.tc("#9ab5a6")
                            font.pixelSize: 12
                            font.weight: Font.DemiBold
                            font.letterSpacing: 1.4
                            text: "ENVIRONMENT VARIABLES"
                        }

                        Item { width: parent.width - addVariableButton.width - 190; height: 1 }

                        Rectangle {
                            id: addVariableButton

                            width: 34
                            height: 28
                            radius: 7
                            color: addVariableMouse.containsMouse ? window.tc("#3d6b53") : window.tc("#315744")
                            scale: addVariableMouse.pressed ? 0.93 : addVariableMouse.containsMouse ? 1.06 : 1

                            Behavior on color { ColorAnimation { duration: 100 } }
                            Behavior on scale { NumberAnimation { duration: 100; easing.type: Easing.OutCubic } }

                            Text {
                                anchors.centerIn: parent
                                color: window.tc("#d7ede1")
                                font.pixelSize: 19
                                text: "+"
                            }

                            MouseArea {
                                id: addVariableMouse

                                anchors.fill: parent
                                hoverEnabled: true
                                preventStealing: true
                                cursorShape: Qt.PointingHandCursor
                                onPressed: launcherState.AddProfileVariable(window.editorIndex)
                            }
                        }
                    }

                    Item { width: 1; height: 10 }

                    Repeater {
                        id: environmentRepeater

                        model: window.editorProfile.environment || []

                        delegate: Item {
                            id: environmentRow

                            required property var modelData
                            property string variableId: modelData.id
                            property alias variableName: variableNameInput.text
                            property alias variableValue: variableValueInput.text

                            width: editorContent.width
                            height: 60

                            Row {
                                width: parent.width
                                height: 48
                                spacing: 8

                                Rectangle {
                                    width: (parent.width - 58) * 0.42
                                    height: parent.height
                                    radius: 8
                                    color: window.tc("#182b29")
                                    border.width: 1
                                    border.color: window.tc("#406756")

                                    TextInput {
                                        id: variableNameInput

                                        z: 1
                                        anchors.fill: parent
                                        anchors.leftMargin: 12
                                        anchors.rightMargin: 12
                                        color: window.tc("#eaf5ee")
                                        font.pixelSize: 14
                                        verticalAlignment: TextInput.AlignVCenter
                                        activeFocusOnPress: true
                                        selectByMouse: true
                                        text: modelData.name
                                    }

                                    Text {
                                        enabled: false
                                        anchors.left: parent.left
                                        anchors.leftMargin: 12
                                        anchors.verticalCenter: parent.verticalCenter
                                        visible: variableNameInput.text.length === 0 && !variableNameInput.activeFocus
                                        color: window.tc("#789285")
                                        font.pixelSize: 13
                                        text: "VARIABLE NAME"
                                    }
                                }

                                Rectangle {
                                    width: (parent.width - 58) * 0.58
                                    height: parent.height
                                    radius: 8
                                    color: window.tc("#182b29")
                                    border.width: 1
                                    border.color: window.tc("#406756")

                                    TextInput {
                                        id: variableValueInput

                                        z: 1
                                        anchors.fill: parent
                                        anchors.leftMargin: 12
                                        anchors.rightMargin: 12
                                        color: window.tc("#eaf5ee")
                                        font.pixelSize: 14
                                        verticalAlignment: TextInput.AlignVCenter
                                        activeFocusOnPress: true
                                        selectByMouse: true
                                        text: modelData.value
                                    }

                                    Text {
                                        enabled: false
                                        anchors.left: parent.left
                                        anchors.leftMargin: 12
                                        anchors.verticalCenter: parent.verticalCenter
                                        visible: variableValueInput.text.length === 0 && !variableValueInput.activeFocus
                                        color: window.tc("#789285")
                                        font.pixelSize: 13
                                        text: "VALUE"
                                    }
                                }

                                Rectangle {
                                    width: 42
                                    height: parent.height
                                    radius: 8
                                    z: 2
                                    color: removeVariableMouse.containsMouse ? window.tc("#65403f") : window.tc("#513534")
                                    scale: removeVariableMouse.pressed ? 0.93 : 1

                                    Behavior on color { ColorAnimation { duration: 100 } }
                                    Behavior on scale { NumberAnimation { duration: 100; easing.type: Easing.OutCubic } }

                                    Text {
                                        anchors.centerIn: parent
                                        color: window.tc("#f1bfbc")
                                        font.pixelSize: 18
                                        text: "−"
                                    }

                                    MouseArea {
                                        id: removeVariableMouse

                                        anchors.fill: parent
                                        hoverEnabled: true
                                        preventStealing: true
                                        cursorShape: Qt.PointingHandCursor
                                        onPressed: launcherState.RemoveProfileVariable(window.editorIndex, environmentRow.variableId)
                                    }
                                }
                            }
                        }
                    }
                }
            }

            Row {
                id: editorActions

                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.margins: 28
                spacing: 10

                Rectangle {
                    width: visible ? 104 : 0
                    height: 48
                    radius: 10
                    visible: !window.editorIsNew
                    color: deleteProfileMouse.containsMouse ? window.tc("#65403f") : window.tc("#513534")
                    scale: deleteProfileMouse.pressed ? 0.96 : 1

                    Behavior on color { ColorAnimation { duration: 120 } }
                    Behavior on scale { NumberAnimation { duration: 100; easing.type: Easing.OutCubic } }

                    Text {
                        anchors.centerIn: parent
                        color: window.tc("#f1bfbc")
                        font.pixelSize: 12
                        font.weight: Font.DemiBold
                        text: "DELETE"
                    }

                    MouseArea {
                        id: deleteProfileMouse

                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            launcherState.DeleteProfile(window.editorIndex)
                            window.closeEditor()
                        }
                    }
                }

                Item { width: parent.width - saveProfileButton.width - (window.editorIsNew ? 0 : 114); height: 1 }

                Rectangle {
                    id: saveProfileButton

                    width: 144
                    height: 48
                    radius: 10
                    color: saveProfileMouse.containsMouse ? window.tc("#78e5a8") : window.tc("#63d796")
                    scale: saveProfileMouse.pressed ? 0.96 : saveProfileMouse.containsMouse ? 1.025 : 1

                    Behavior on color { ColorAnimation { duration: 120 } }
                    Behavior on scale { NumberAnimation { duration: 100; easing.type: Easing.OutCubic } }

                    Text {
                        anchors.centerIn: parent
                        color: window.tc("#07120b")
                        font.pixelSize: 13
                        font.weight: Font.DemiBold
                        font.letterSpacing: 1.3
                        text: window.editorIsNew ? "ADD PROFILE" : "SAVE PROFILE"
                    }

                    MouseArea {
                        id: saveProfileMouse

                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            let environment = []
                            for (let i = 0; i < environmentRepeater.count; i++) {
                                let row = environmentRepeater.itemAt(i)
                                environment.push({
                                    "id": row.variableId,
                                    "name": row.variableName,
                                    "value": row.variableValue
                                })
                            }

                            launcherState.SaveProfile(window.editorIndex, profileNameInput.text, environment)
                            window.editorIsNew = false
                            window.closeEditor()
                        }
                    }
                }
            }
        }
    }

    Rectangle {
        id: launcherUpdatePage

        anchors.fill: parent
        z: 100
        visible: launcherState.launcherUpdateActive
        gradient: Gradient {
            GradientStop { position: 0.0; color: window.tc("#11271f") }
            GradientStop { position: 0.58; color: window.tc("#0b1517") }
            GradientStop { position: 1.0; color: window.tc("#080c10") }
        }

        MouseArea {
            anchors.fill: parent
        }

        Rectangle {
            width: 620
            height: 620
            radius: width / 2
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: parent.verticalCenter
            color: window.tc("#258a62")
            opacity: 0.08
        }

        Text {
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.leftMargin: 42
            anchors.topMargin: 34
            color: window.tc("#edf9f2")
            font.pixelSize: 25
            font.weight: Font.DemiBold
            font.letterSpacing: 4.8
            text: "EVERSTEAD"
        }

        Column {
            width: Math.min(parent.width - 80, 520)
            anchors.centerIn: parent
            spacing: 18

            Text {
                width: parent.width
                color: launcherState.launcherUpdateError.length > 0 ? window.tc("#f1bfbc") : window.tc("#eff9f3")
                font.pixelSize: 30
                font.weight: Font.DemiBold
                horizontalAlignment: Text.AlignHCenter
                text: launcherState.launcherUpdateError.length > 0
                    ? "UPDATE FAILED"
                    : launcherState.launcherUpdateProgress > 0
                        ? "UPDATING LAUNCHER"
                        : "CHECKING FOR UPDATES"

                Behavior on color { ColorAnimation { duration: 160 } }
            }

            Text {
                width: parent.width
                color: window.tc("#9cb5a8")
                font.pixelSize: 15
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
                text: launcherState.launcherUpdateError.length > 0
                    ? launcherState.launcherUpdateError
                    : launcherState.launcherUpdateStatus
            }

            Rectangle {
                width: parent.width
                height: 8
                radius: 4
                color: window.tc("#1a2e28")
                visible: launcherState.launcherUpdateError.length === 0
                clip: true

                Rectangle {
                    height: parent.height
                    width: launcherState.launcherUpdateProgress > 0
                        ? parent.width * Math.min(1, launcherState.launcherUpdateProgress)
                        : 72
                    x: launcherState.launcherUpdateProgress > 0 ? 0 : (parent.width - width) / 2
                    radius: 4
                    color: window.tc("#63d796")
                    opacity: launcherState.launcherUpdateProgress > 0 ? 1 : 0.55

                    Behavior on width { NumberAnimation { duration: 100; easing.type: Easing.OutCubic } }
                    SequentialAnimation on opacity {
                        running: launcherState.launcherUpdateProgress === 0
                        loops: Animation.Infinite
                        NumberAnimation { to: 1; duration: 650; easing.type: Easing.InOutSine }
                        NumberAnimation { to: 0.4; duration: 650; easing.type: Easing.InOutSine }
                    }
                }
            }

            Text {
                width: parent.width
                visible: launcherState.launcherUpdateProgress > 0 && launcherState.launcherUpdateError.length === 0
                color: window.tc("#7f9b8d")
                font.pixelSize: 12
                horizontalAlignment: Text.AlignHCenter
                text: Math.round(launcherState.launcherUpdateProgress * 100) + "%"
            }

            Rectangle {
                width: 150
                height: 46
                anchors.horizontalCenter: parent.horizontalCenter
                radius: 10
                visible: launcherState.launcherUpdateError.length > 0
                color: retryLauncherUpdateMouse.containsMouse ? window.tc("#78e5a8") : window.tc("#63d796")
                scale: retryLauncherUpdateMouse.pressed ? 0.96 : retryLauncherUpdateMouse.containsMouse ? 1.025 : 1

                Behavior on color { ColorAnimation { duration: 120 } }
                Behavior on scale { NumberAnimation { duration: 100; easing.type: Easing.OutCubic } }

                Text {
                    anchors.centerIn: parent
                    color: window.tc("#07120b")
                    font.pixelSize: 13
                    font.weight: Font.DemiBold
                    font.letterSpacing: 1.2
                    text: "TRY AGAIN"
                }

                MouseArea {
                    id: retryLauncherUpdateMouse

                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: launcherState.CheckForLauncherUpdates()
                }
            }
        }

        Text {
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.rightMargin: 18
            anchors.bottomMargin: 14
            color: window.tc("#60766b")
            font.pixelSize: 11
            text: "v" + launcherState.launcherVersion
        }

    }

    Window {
        id: logWindow

        width: 820
        height: 520
        minimumWidth: 520
        minimumHeight: 320
        visible: launcherState.logWindowVisible
        title: "Everstead Game Logs"
        color: window.tc("#091012")

        onClosing: close => {
            close.accepted = false
            launcherState.logWindowVisible = false
        }

        Rectangle {
            anchors.fill: parent
            gradient: Gradient {
                GradientStop { position: 0; color: window.tc("#10231e") }
                GradientStop { position: 1; color: window.tc("#080d10") }
            }
        }

        Column {
            anchors.fill: parent
            anchors.margins: 22
            spacing: 14

            Row {
                width: parent.width

                Text {
                    color: window.tc("#eff9f3")
                    font.pixelSize: 20
                    font.weight: Font.DemiBold
                    font.letterSpacing: 2.4
                    text: "GAME LOGS"
                }

                Item { width: parent.width - logStatus.width - 126; height: 1 }

                Text {
                    id: logStatus

                    anchors.verticalCenter: parent.verticalCenter
                    color: launcherState.gameRunning ? window.tc("#63d796") : window.tc("#8fa99c")
                    font.pixelSize: 13
                    text: launcherState.gameRunning ? "RUNNING" : "CLOSED"

                    Behavior on color { ColorAnimation { duration: 150 } }
                }
            }

            Rectangle {
                width: parent.width
                height: parent.height - 43
                radius: 12
                color: window.tc("#07100f")
                border.width: 1
                border.color: window.tc("#315346")
                clip: true

                ListView {
                    id: logView

                    anchors.left: parent.left
                    anchors.right: logScrollTrack.left
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.leftMargin: 14
                    anchors.rightMargin: 10
                    anchors.topMargin: 14
                    anchors.bottomMargin: 14
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds
                    model: launcherState.gameLogModel
                    reuseItems: true
                    cacheBuffer: height * 2

                    property bool followOutput: true
                    property int previousLogCount: 0

                    function scrollToEnd() {
                        if (count > 0) {
                            positionViewAtEnd()
                        }
                    }

                    onMovementStarted: followOutput = false
                    onMovementEnded: followOutput = atYEnd
                    onContentHeightChanged: {
                        if (followOutput) {
                            Qt.callLater(scrollToEnd)
                        }
                    }

                    delegate: TextEdit {
                        required property string content

                        width: logView.width
                        height: Math.max(18, contentHeight)
                        readOnly: true
                        selectByMouse: true
                        textFormat: TextEdit.RichText
                        wrapMode: TextEdit.Wrap
                        color: window.tc("#c8d8d0")
                        selectionColor: window.tc("#376d55")
                        selectedTextColor: window.tc("#f3fbf6")
                        font.family: "monospace"
                        font.pixelSize: 13
                        text: content

                        onContentHeightChanged: {
                            if (logView.followOutput) {
                                Qt.callLater(logView.scrollToEnd)
                            }
                        }
                    }
                }

                Text {
                    anchors.centerIn: logView
                    visible: logView.count === 0
                    color: window.tc("#c8d8d0")
                    font.family: "monospace"
                    font.pixelSize: 13
                    text: "Waiting for game output..."
                }

                Rectangle {
                    id: logScrollTrack

                    width: 8
                    anchors.right: parent.right
                    anchors.rightMargin: 10
                    anchors.top: parent.top
                    anchors.topMargin: 14
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 14
                    radius: 4
                    visible: logView.contentHeight > logView.height + 1
                    color: window.tc("#172622")

                    property real scrollRange: Math.max(0, logView.contentHeight - logView.height)
                    property real scrollProgress: scrollRange > 0
                        ? Math.max(0, Math.min(1, (logView.contentY - logView.originY) / scrollRange))
                        : 0

                    Rectangle {
                        id: logScrollThumb

                        width: parent.width
                        height: Math.min(parent.height, Math.max(36, parent.height * logView.height / logView.contentHeight))
                        y: Math.max(0, Math.min(parent.height - height, (parent.height - height) * logScrollTrack.scrollProgress))
                        radius: 4
                        color: logScrollMouse.containsMouse || logScrollMouse.pressed ? window.tc("#67d59a") : window.tc("#496a5c")

                        Behavior on color { ColorAnimation { duration: 100 } }
                    }

                    MouseArea {
                        id: logScrollMouse

                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor

                        property real dragOffset: 0

                        function setScrollPosition(thumbPosition) {
                            let thumbRange = height - logScrollThumb.height
                            if (thumbRange <= 0) {
                                return
                            }

                            let progress = Math.max(0, Math.min(1, thumbPosition / thumbRange))
                            logView.contentY = logView.originY + progress * logScrollTrack.scrollRange
                        }

                        onPressed: mouse => {
                            logView.followOutput = false
                            dragOffset = mouse.y >= logScrollThumb.y && mouse.y <= logScrollThumb.y + logScrollThumb.height
                                ? mouse.y - logScrollThumb.y
                                : logScrollThumb.height / 2
                            setScrollPosition(mouse.y - dragOffset)
                        }
                        onPositionChanged: mouse => {
                            if (pressed) {
                                setScrollPosition(mouse.y - dragOffset)
                            }
                        }
                        onReleased: logView.followOutput = logView.atYEnd
                    }
                }
            }
        }

        Connections {
            target: launcherState

            function onGameLogChanged() {
                if (logView.count < logView.previousLogCount) {
                    logView.followOutput = true
                }

                logView.previousLogCount = logView.count
                if (logView.followOutput) {
                    Qt.callLater(logView.scrollToEnd)
                }
            }
        }
    }
}
