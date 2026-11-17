import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ApplicationWindow {
    id: win
    width: 1180
    height: 760
    minimumWidth: 880
    minimumHeight: 600
    visible: true
    title: "CTRL"
    color: "#050608"

    // Native telemetry; process and tuning controls remain explicitly staged demos.
    function updateMonitoringContext() {
        systemMonitor.setMonitoringContext(!visible || visibility === Window.Minimized, page)
    }
    onPageChanged: updateMonitoringContext()
    onVisibilityChanged: updateMonitoringContext()
    Component.onCompleted: updateMonitoringContext()
    readonly property color bg: "#050608"
    readonly property color surface: "#0C1015"
    readonly property color surfaceRaised: "#121922"
    readonly property color line: "#161D26"
    readonly property color border: "#26313E"
    readonly property color accent: "#168BFF"
    readonly property color cyan: "#36C8FF"
    readonly property color good: "#4ACFA0"
    readonly property color warning: "#E5AD58"
    readonly property color danger: "#E0675F"
    readonly property color ink: "#E8EDF4"
    readonly property color muted: "#788898"
    readonly property color faint: "#55616F"
    readonly property string mono: "Consolas"
    property int page: 0
    property int hardwareTab: 0
    property string profile: "BALANCED"
    property string gpuMode: "STANDARD"
    property string boostMode: "EFFICIENT"
    property string fanMode: "AUTO"
    property int softTemp: 75
    property int cpuSustained: 25
    property int cpuBurst: 35
    property int selectedProcess: -1
    property bool consoleExpanded: false
    property bool automaticSwitch: true
    property bool startup: false
    property bool startMinimized: false
    property bool trayEnabled: true
    property bool processMonitor: true
    property bool hardwareStaged: false
    property string consoleMessage: "CTRL ready · Windows telemetry connected; hardware writes require Apply; process/profile demos"
    property var pageNames: ["Overview", "Processes", "Hardware & Cooling", "Profiles", "Settings"]
    property var pageCodes: ["OVR", "PRC", "HW", "PRF", "SET"]
    property var sampleProcesses: [
        {pid: 1840, name: "Discord.exe", cpu: "0.2%", ram: "230 MB", status: "RUNNING"},
        {pid: 2932, name: "OneDrive.exe", cpu: "0.1%", ram: "84 MB", status: "RUNNING"},
        {pid: 4576, name: "explorer.exe", cpu: "0.4%", ram: "145 MB", status: "PROTECTED"},
        {pid: 1124, name: "svchost.exe", cpu: "0.3%", ram: "41 MB", status: "PROTECTED"},
        {pid: 5120, name: "Spotify.exe", cpu: "0.5%", ram: "312 MB", status: "RUNNING"},
        {pid: 6244, name: "Teams.exe", cpu: "0.8%", ram: "402 MB", status: "RUNNING"},
        {pid: 7010, name: "msedge.exe", cpu: "1.2%", ram: "540 MB", status: "RUNNING"},
        {pid: 8112, name: "Steam.exe", cpu: "0.3%", ram: "176 MB", status: "RUNNING"}
    ]

    function logMessage(message) { consoleMessage = message }
    function setProfile(name) {
        profile = name
        logMessage("[SIM] profile " + name + " selected — UI only")
    }
    function processAction(action) {
        if (selectedProcess < 0) return
        var p = sampleProcesses[selectedProcess]
        if (p.status === "PROTECTED") { logMessage("[WARN] protected Windows process"); return }
        var next = sampleProcesses.slice()
        next[selectedProcess] = {pid:p.pid,name:p.name,cpu:p.cpu,ram:p.ram,status:action === "TERMINATE" ? "TERMINATED" : (action === "BLOCK" ? "BLOCKED" : "RUNNING")}
        sampleProcesses = next
        logMessage("[SIM] " + action.toLowerCase() + " " + p.name + " — no Windows change")
    }
    function command(raw) {
        var cmd = raw.trim().toLowerCase()
        if (cmd === "help") logMessage("help · status · eco · balanced · performance · processes · hardware · clear")
        else if (cmd === "status") logMessage("[LIVE] battery=" + systemMonitor.batteryPercent + "% cpu=" + systemMonitor.cpuPercent.toFixed(1) + "% discharge=" + (systemMonitor.dischargeWatts >= 0 ? systemMonitor.dischargeWatts.toFixed(1) + "W" : "N/A") + " cpuTemp=" + (systemMonitor.cpuTemperatureC >= 0 ? systemMonitor.cpuTemperatureC.toFixed(1) + "C" : "N/A") + " · profile UI=" + profile)
        else if (cmd === "eco" || cmd === "balanced" || cmd === "performance") setProfile(cmd.toUpperCase())
        else if (cmd === "processes") { page = 1; logMessage("Opened processes") }
        else if (cmd === "hardware") { page = 2; logMessage("Opened hardware") }
        else if (cmd === "clear") logMessage("_")
        else if (cmd.length) logMessage("[WARN] unknown command: " + cmd)
    }
    Dialog {
        id: hardwareConsent
        parent: Overlay.overlay
        anchors.centerIn: parent
        width: Math.min(win.width - 60, 650)
        title: "Apply native hardware changes?"
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel
        palette.window: win.surface
        palette.windowText: win.ink
        palette.button: win.surfaceRaised
        palette.buttonText: win.ink
        contentItem: ColumnLayout {
            MonoText { text: "This writes firmware or Windows power policies. Fan modes may reset custom curves. GPU changes may reconfigure displays; ECO checks for active GPU clients/displays and may initialize NVIDIA to do so. No applications will be terminated."; Layout.fillWidth: true; wrapMode: Text.WordWrap }
            MonoText { text: "ASUS active performance mode and custom curves cannot be read back. A fan-curve Apply FIRST selects the editor's base firmware mode, which may reset previous custom curves. Previous unreadable settings cannot be restored automatically; factory curves can be reapplied. Firmware thermal protection stays active."; color: win.warning; Layout.fillWidth: true; wrapMode: Text.WordWrap }
            MonoText { text: JSON.stringify(hardwareManager.pending); Layout.fillWidth: true; wrapMode: Text.WrapAnywhere; font.pixelSize: 11 }
        }
        onAccepted: hardwareManager.applyConfirmed(true)
    }
    Dialog {
        id: nvidiaConsent
        parent: Overlay.overlay
        anchors.centerIn: parent
        width: Math.min(win.width - 60, 550)
        title: "Inspect NVIDIA GPU?"
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel
        palette.window: win.surface
        palette.windowText: win.ink
        palette.button: win.surfaceRaised
        palette.buttonText: win.ink
        contentItem: MonoText { text: "This on-demand inspection initializes NVML and may wake the dGPU in Standard mode. ECO mode skips NVML. There is no continuous NVIDIA polling. Pending settings must be discarded first."; wrapMode: Text.WordWrap }
        onAccepted: hardwareManager.inspectNvidia()
    }
    Shortcut { sequence: "Ctrl+1"; onActivated: win.page = 0 }
    Shortcut { sequence: "Ctrl+2"; onActivated: win.page = 1 }
    Shortcut { sequence: "Ctrl+3"; onActivated: win.page = 2 }
    Shortcut { sequence: "Ctrl+4"; onActivated: win.page = 3 }
    Shortcut { sequence: "Ctrl+5"; onActivated: win.page = 4 }
    Shortcut { sequence: "Ctrl+`"; onActivated: win.consoleExpanded = !win.consoleExpanded }

    component MonoText: Text {
        color: win.ink
        font.family: win.mono
        font.pixelSize: 13
        renderType: Text.QtRendering
    }
    component SectionTag: MonoText {
        color: win.faint
        font.pixelSize: 10
        font.letterSpacing: 1.7
    }
    component ThinRule: Rectangle {
        color: win.line
        implicitHeight: 1
        Layout.fillWidth: true
    }
    component ConsoleButton: Button {
        id: cb
        property bool primary: false
        property color highlight: win.accent
        implicitHeight: 30
        implicitWidth: Math.max(72, cbLabel.implicitWidth + 26)
        background: Rectangle {
            radius: 2
            color: cb.down ? win.surfaceRaised : (cb.primary ? win.accent : (cb.hovered ? win.surfaceRaised : "transparent"))
            border.color: cb.primary ? win.accent : (cb.hovered ? win.border : win.border)
            border.width: 1
        }
        contentItem: MonoText {
            id: cbLabel
            text: cb.text
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            font.pixelSize: 12
            color: cb.primary ? "#FFFFFF" : (cb.hovered ? win.ink : win.muted)
        }
    }
    component ProfileButton: Rectangle {
        id: pb
        property string choice: "ECO"
        property bool active: win.profile === choice
        signal chosen()
        implicitHeight: 34
        implicitWidth: Math.max(92, txt.implicitWidth + 28)
        color: active ? "#112338" : (pointer.containsMouse ? win.surfaceRaised : "transparent")
        border.color: active ? (choice === "ECO" ? win.good : (choice === "PERFORMANCE" ? win.warning : win.accent)) : win.border
        border.width: 1
        MonoText {
            id: txt
            anchors.centerIn: parent
            text: pb.choice
            font.pixelSize: 12
            font.letterSpacing: 0.7
            color: pb.active ? (pb.choice === "ECO" ? win.good : (pb.choice === "PERFORMANCE" ? win.warning : win.accent)) : win.muted
        }
        MouseArea { id: pointer; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: pb.chosen() }
    }
    function hardwareCap(key) {
        return hardwareManager.state[key] || {state:"UNSUPPORTED", reason:"Discovery pending", enabled:false, value:null, options:[]}
    }
    function stageFanDuty(setting, point, duty) {
        var source = hardwareManager.pending[setting] || hardwareCap(setting).value || []
        var next = []
        for (var i=0; i<source.length; ++i)
            next.push({temperature:source[i].temperature, duty:i === point ? duty : source[i].duty})
        hardwareManager.stage(setting, next)
    }
    component HardwareOptions: ColumnLayout {
        id: optionControl
        property string setting: "performance"
        property string heading: "PERFORMANCE MODE"
        readonly property var report: win.hardwareCap(setting === "boostAC" || setting === "boostDC" ? "boost" : setting)
        readonly property var current: setting === "boostAC" ? report.ac : setting === "boostDC" ? report.dc : report.value
        Layout.fillWidth: true
        spacing: 7
        SectionTag { text: optionControl.heading }
        Row {
            spacing: -1
            Repeater {
                model: optionControl.report.options || []
                ProfileButton {
                    required property var modelData
                    choice: modelData.label
                    enabled: optionControl.report.enabled && !hardwareManager.busy && !(optionControl.setting === "performance" && modelData.value === 1 && systemMonitor.powerSource !== "AC")
                    opacity: enabled ? 1 : 0.4
                    active: (hardwareManager.pending[optionControl.setting] !== undefined ? hardwareManager.pending[optionControl.setting] : optionControl.current) === modelData.value
                    onChosen: hardwareManager.stage(optionControl.setting, modelData.value)
                }
            }
        }
        MonoText { text: optionControl.report.state + " · " + optionControl.report.reason; color: win.muted; font.pixelSize: 11; Layout.fillWidth: true; wrapMode: Text.WordWrap }
    }
    component FanCurveEditor: ColumnLayout {
        id: curveControl
        property string setting: "cpuCurve"
        property string heading: "CPU FACTORY / CUSTOM CURVE"
        readonly property var report: win.hardwareCap(setting)
        readonly property var points: report.value || []
        Layout.fillWidth: true
        spacing: 5
        SectionTag { text: curveControl.heading }
        MonoText { text: curveControl.report.state + " · " + curveControl.report.reason; color: win.muted; font.pixelSize: 11; Layout.fillWidth: true; wrapMode: Text.WordWrap }
        Flow {
            Layout.fillWidth: true
            spacing: 8
            Repeater {
                model: curveControl.points
                Column {
                    required property int index
                    required property var modelData
                    spacing: 3
                    MonoText { text: modelData.temperature + "°C"; color: win.muted; font.pixelSize: 11 }
                    SpinBox {
                        width: 90; height: 30
                        from: modelData.duty; to: 100; stepSize: 1; editable: true
                        enabled: curveControl.report.enabled && !hardwareManager.busy
                        palette.base: win.surface
                        palette.text: win.ink
                        palette.button: win.surfaceRaised
                        palette.buttonText: win.ink
                        value: hardwareManager.pending[curveControl.setting] ? hardwareManager.pending[curveControl.setting][index].duty : modelData.duty
                        onValueModified: win.stageFanDuty(curveControl.setting, index, value)
                    }
                }
            }
        }
        MonoText { text: "Factory temperature bins are fixed; duty (%) may only increase. No direct fan-speed commands."; color: win.faint; font.pixelSize: 11; Layout.fillWidth: true; wrapMode: Text.WordWrap }
    }
    component ToggleSwitch: Item {
        id: toggle
        property bool checked: false
        signal toggled(bool value)
        implicitWidth: 38
        implicitHeight: 20
        Rectangle {
            anchors.fill: parent
            radius: 2
            color: toggle.checked ? "#102B42" : win.surface
            border.color: toggle.checked ? win.accent : win.border
            Rectangle {
                width: 10; height: 10; radius: 1
                x: toggle.checked ? 22 : 4
                y: 4
                color: toggle.checked ? win.accent : win.muted
                Behavior on x { NumberAnimation { duration: 120 } }
            }
        }
        MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: toggle.toggled(!toggle.checked) }
    }
    component ControlSlider: ColumnLayout {
        id: control
        property string label: "Setting"
        property string hint: ""
        property string suffix: " W"
        property int from: 0
        property int to: 100
        property int value: 50
        property bool available: true
        signal edited(int newValue)
        spacing: 5
        opacity: available ? 1 : 0.48
        RowLayout {
            Layout.fillWidth: true
            spacing: 12
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2
                MonoText { text: control.label; font.pixelSize: 13 }
                MonoText { text: control.hint; color: win.faint; font.pixelSize: 11; wrapMode: Text.WordWrap; Layout.fillWidth: true }
            }
            Slider {
                id: slider
                Layout.preferredWidth: 260
                from: control.from; to: control.to; stepSize: 1
                value: control.value
                enabled: control.available
                onMoved: control.edited(Math.round(value))
                background: Rectangle {
                    x: slider.leftPadding; y: slider.topPadding + slider.availableHeight / 2 - 1
                    width: slider.availableWidth; height: 2; color: win.border
                    Rectangle { width: slider.visualPosition * parent.width; height: parent.height; color: win.accent }
                }
                handle: Rectangle {
                    x: slider.leftPadding + slider.visualPosition * (slider.availableWidth - width)
                    y: slider.topPadding + slider.availableHeight / 2 - height / 2
                    width: 12; height: 12; radius: 1
                    color: slider.enabled ? win.cyan : win.faint
                }
            }
            MonoText { text: control.available ? control.value + control.suffix : "Unavailable"; Layout.preferredWidth: 58; horizontalAlignment: Text.AlignRight }
        }
        ThinRule { }
    }
    component SimpleSetting: ColumnLayout {
        id: sr
        property string label: "Setting"
        property string hint: ""
        property bool checked: false
        signal changed(bool value)
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 47
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2
                MonoText { text: sr.label }
                MonoText { text: sr.hint; color: win.faint; font.pixelSize: 11; visible: sr.hint.length > 0 }
            }
            ToggleSwitch { checked: sr.checked; onToggled: (value) => sr.changed(value) }
        }
        ThinRule { }
    }
    component PageHeader: ColumnLayout {
        property string heading: ""
        property string caption: ""
        spacing: 4
        MonoText { text: heading; font.pixelSize: 20 }
        MonoText { text: caption; color: win.muted; font.pixelSize: 12; wrapMode: Text.WordWrap; Layout.fillWidth: true }
        Item { Layout.preferredHeight: 21 }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0
        // Very slim navigation rail, not a conventional sidebar.
        Rectangle {
            Layout.preferredWidth: 68
            Layout.fillHeight: true
            color: win.bg
            Rectangle { anchors.right: parent.right; width: 1; height: parent.height; color: win.line }
            Column {
                width: parent.width
                MonoText {
                    width: parent.width; height: 53
                    text: "V_"; color: win.accent; font.pixelSize: 18; font.bold: true
                    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                }
                Repeater {
                    model: 5
                    delegate: Rectangle {
                        required property int index
                        width: 68; height: 58
                        color: mouse.containsMouse ? win.surface : "transparent"
                        Rectangle { anchors.left: parent.left; width: 2; height: 28; anchors.verticalCenter: parent.verticalCenter; color: win.accent; visible: win.page === index }
                        Column {
                            anchors.centerIn: parent; spacing: 2
                            MonoText { text: "0" + (index + 1); font.pixelSize: 15; color: win.page === index ? win.cyan : win.muted; anchors.horizontalCenter: parent.horizontalCenter }
                            MonoText { text: win.pageCodes[index]; font.pixelSize: 9; font.letterSpacing: 1; color: win.page === index ? win.accent : win.faint; anchors.horizontalCenter: parent.horizontalCenter }
                        }
                        MouseArea { id: mouse; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: win.page = index }
                        ToolTip.visible: mouse.containsMouse
                        ToolTip.text: win.pageNames[index]
                        ToolTip.delay: 450
                    }
                }
            }
        }
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0
            // Hairline header; deliberately no large title bar.
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 32
                color: win.bg
                Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: win.line }
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 36
                    anchors.rightMargin: 36
                    spacing: 9
                    MonoText { text: "CTRL"; color: win.accent; font.pixelSize: 11; font.letterSpacing: 1.5 }
                    MonoText { text: "/"; color: win.faint; font.pixelSize: 11 }
                    MonoText { text: win.pageNames[win.page].toUpperCase(); color: win.muted; font.pixelSize: 11; Layout.fillWidth: true }
                    Rectangle {
                        width: 84; height: 19; color: "transparent"; border.color: win.warning; border.width: 1
                        MonoText { anchors.centerIn: parent; text: "PARTIAL LIVE"; color: win.warning; font.pixelSize: 10; font.letterSpacing: 0.7 }
                    }
                }
            }
            StackLayout {
                id: pages
                Layout.fillWidth: true
                Layout.fillHeight: true
                currentIndex: win.page

                // 01 — OVERVIEW. No box-grid. Asymmetric columns and thin data strips.
                Flickable {
                    contentWidth: width; contentHeight: overviewContent.implicitHeight + 64
                    clip: true
                    ColumnLayout {
                        id: overviewContent
                        x: 36; y: 30
                        width: Math.min(parent.width - 72, 900)
                        spacing: 0
                        PageHeader { heading: "ASUS TUF A15 FA507NV"; caption: "AMD Ryzen 7 7735HS  ·  NVIDIA RTX 4060"; Layout.fillWidth: true }
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 64
                            ColumnLayout {
                                Layout.fillWidth: true
                                Layout.preferredWidth: 400
                                spacing: 12
                                Row {
                                    MonoText { text: systemMonitor.batteryPercent < 0 ? "--" : String(systemMonitor.batteryPercent); font.pixelSize: 76; font.weight: Font.Light }
                                    MonoText { text: "%"; color: win.muted; font.pixelSize: 24; anchors.baseline: parent.children[0].baseline }
                                }
                                Row {
                                    Layout.fillWidth: true
                                    spacing: 2
                                    Repeater {
                                        model: 40
                                        Rectangle { width: Math.max(3, (overviewContent.width - 64) / 80); height: 8; color: index < Math.round(Math.max(0, systemMonitor.batteryPercent) / 100 * 40) ? win.accent : win.line }
                                    }
                                }
                                MonoText { text: systemMonitor.powerSource + " · " + systemMonitor.batteryStatus + (systemMonitor.batteryStatus === "Discharging" ? " · " + (systemMonitor.dischargeWatts >= 0 ? systemMonitor.dischargeWatts.toFixed(1) + " W" : "Power Unavailable") : ""); color: win.muted; font.pixelSize: 12 }
                                MonoText { text: systemMonitor.remainingMinutes >= 0 ? Math.floor(systemMonitor.remainingMinutes / 60) + "h " + (systemMonitor.remainingMinutes % 60) + "m (Windows estimate)" : "Remaining time: Unavailable"; color: win.muted; font.pixelSize: 12 }
                                MonoText { text: "Health: " + (systemMonitor.batteryHealthPercent >= 0 ? systemMonitor.batteryHealthPercent.toFixed(1) + "%" : "Unavailable") + " · Full / design: " + (systemMonitor.fullChargeCapacityWh >= 0 ? systemMonitor.fullChargeCapacityWh.toFixed(1) : "Unavailable") + " / " + (systemMonitor.designCapacityWh >= 0 ? systemMonitor.designCapacityWh.toFixed(1) + " Wh" : "Unavailable"); color: win.muted; font.pixelSize: 11 }
                            }
                            ColumnLayout {
                                Layout.fillWidth: true
                                Layout.preferredWidth: 350
                                spacing: 18
                                Repeater {
                                    model: [ {label:"CPU USAGE",value:systemMonitor.cpuPercent < 0 ? "--" : systemMonitor.cpuPercent.toFixed(1) + "%",ratio:Math.max(0, systemMonitor.cpuPercent / 100)}, {label:"CPU TEMP",value:systemMonitor.cpuTemperatureC >= 0 ? systemMonitor.cpuTemperatureC.toFixed(1) + "°C" : "Unavailable",ratio:Math.max(0,systemMonitor.cpuTemperatureC / 100)}, {label:"MEMORY",value:systemMonitor.ramUsedGB.toFixed(1) + " of " + systemMonitor.ramTotalGB.toFixed(1) + " GiB",ratio:systemMonitor.ramRatio} ]
                                    delegate: ColumnLayout {
                                        required property var modelData
                                        Layout.fillWidth: true; spacing: 7
                                        RowLayout {
                                            Layout.fillWidth: true
                                            SectionTag { text: modelData.label; Layout.fillWidth: true }
                                            MonoText { text: modelData.value; font.pixelSize: 14 }
                                        }
                                        Rectangle {
                                            Layout.fillWidth: true; height: 2; color: win.line
                                            Rectangle { width: parent.width * modelData.ratio; height: 2; color: win.accent }
                                        }
                                    }
                                }
                            }
                        }
                        Item { Layout.preferredHeight: 43 }
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 64
                            ColumnLayout {
                                Layout.fillWidth: true
                                Layout.preferredWidth: 400
                                spacing: 13
                                SectionTag { text: "PROFILE" }
                                Row {
                                    spacing: -1
                                    Repeater {
                                        model: ["ECO", "BALANCED", "PERFORMANCE"]
                                        ProfileButton { required property string modelData; choice: modelData; onChosen: win.setProfile(choice) }
                                    }
                                }
                                MonoText { text: win.profile === "ECO" ? "Longest battery life" : (win.profile === "BALANCED" ? "Everyday use" : "Demanding workloads"); color: win.muted; font.pixelSize: 12 }
                            }
                            ColumnLayout {
                                Layout.fillWidth: true
                                Layout.preferredWidth: 350
                                spacing: 10
                                SectionTag { text: "SYSTEM" }
                                Rectangle {
                                    Layout.fillWidth: true
                                    implicitHeight: sysRows.implicitHeight + 27
                                    color: win.surface
                                    Rectangle { width: 2; height: parent.height; color: win.border }
                                    ColumnLayout {
                                        id: sysRows
                                        anchors.left: parent.left; anchors.right: parent.right
                                        anchors.leftMargin: 16; anchors.rightMargin: 10
                                        anchors.verticalCenter: parent.verticalCenter
                                        spacing: 8
                                        Repeater {
                                            model: [ ["MONITOR.STATUS", "LIVE"], ["POWER.SOURCE", systemMonitor.onAC ? "AC" : "BATTERY"], ["GPU.STATE", systemMonitor.gpuActivity], ["PROCESS.GUARD", "SIMULATED"] ]
                                            delegate: RowLayout {
                                                required property var modelData
                                                Layout.fillWidth: true
                                                MonoText { text: modelData[0]; color: win.muted; Layout.preferredWidth: 157; font.pixelSize: 12 }
                                                MonoText { text: modelData[1]; font.pixelSize: 12; color: (modelData[1] === "OPTIMAL" || modelData[1] === "ACTIVE") ? win.good : win.ink }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        Item { Layout.preferredHeight: 40 }
                        MonoText { text: "LIVE: BATTERY / CPU / RAM / DISCHARGE RATE* / CPU TEMP*    ·    *IF REPORTED BY PROVIDER"; color: win.faint; font.pixelSize: 10; font.letterSpacing: 0.5 }
                    }
                }

                // 02 — PROCESSES
                Flickable {
                    contentWidth: width; contentHeight: processContent.implicitHeight + 70
                    clip: true
                    ColumnLayout {
                        id: processContent
                        x: 36; y: 30; width: Math.min(parent.width - 72, 900)
                        spacing: 13
                        PageHeader { heading: "Processes"; caption: "Select an application and choose an action. All process operations are simulated."; Layout.fillWidth: true }
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 9
                            TextField {
                                id: searchField
                                Layout.preferredWidth: 300
                                placeholderText: "Search name or PID"
                                color: win.ink; font.family: win.mono; font.pixelSize: 12
                                placeholderTextColor: win.faint
                                background: Rectangle { color: win.surface; border.color: searchField.activeFocus ? win.accent : win.border; radius: 2 }
                            }
                            ConsoleButton { text: "Terminate"; enabled: win.selectedProcess >= 0; onClicked: win.processAction("TERMINATE") }
                            ConsoleButton { text: "Block"; enabled: win.selectedProcess >= 0; onClicked: win.processAction("BLOCK") }
                            ConsoleButton { text: "Allow"; enabled: win.selectedProcess >= 0; onClicked: win.processAction("ALLOW") }
                        }
                        MonoText { text: win.selectedProcess < 0 ? "No process selected." : "Selected: " + win.sampleProcesses[win.selectedProcess].name; color: win.muted; font.pixelSize: 12 }
                        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: win.border }
                        RowLayout {
                            Layout.fillWidth: true; spacing: 10
                            Repeater {
                                model: ["PID", "PROCESS", "CPU", "MEMORY", "STATUS"]
                                delegate: SectionTag {
                                    required property int index
                                    required property string modelData
                                    text: modelData
                                    Layout.preferredWidth: index === 0 ? 62 : (index === 1 ? 260 : (index === 4 ? 120 : 100))
                                }
                            }
                        }
                        Repeater {
                            model: win.sampleProcesses
                            delegate: Rectangle {
                                required property int index
                                required property var modelData
                                readonly property bool matches: searchField.text === "" || modelData.name.toLowerCase().indexOf(searchField.text.toLowerCase()) >= 0 || String(modelData.pid).indexOf(searchField.text) >= 0
                                visible: matches
                                Layout.fillWidth: true
                                Layout.preferredHeight: matches ? 35 : 0
                                color: win.selectedProcess === index ? win.surfaceRaised : (rowMouse.containsMouse ? win.surface : "transparent")
                                Rectangle { width: 2; height: parent.height; color: win.accent; visible: win.selectedProcess === index }
                                RowLayout {
                                    anchors.fill: parent; anchors.leftMargin: 10; spacing: 10
                                    MonoText { text: modelData.pid; color: win.muted; Layout.preferredWidth: 52; font.pixelSize: 12 }
                                    MonoText { text: modelData.name; Layout.preferredWidth: 260; font.pixelSize: 12 }
                                    MonoText { text: modelData.cpu; color: win.muted; Layout.preferredWidth: 100; font.pixelSize: 12 }
                                    MonoText { text: modelData.ram; color: win.muted; Layout.preferredWidth: 100; font.pixelSize: 12 }
                                    MonoText { text: modelData.status; color: modelData.status === "PROTECTED" ? win.cyan : (modelData.status === "RUNNING" ? win.good : win.warning); font.pixelSize: 11 }
                                    Item { Layout.fillWidth: true }
                                }
                                MouseArea { id: rowMouse; anchors.fill: parent; hoverEnabled: true; onClicked: win.selectedProcess = index }
                            }
                        }
                        Item { Layout.preferredHeight: 20 }
                        SectionTag { text: "RULES" }
                        MonoText { text: "Process rules are visual only; nothing is blocked in Windows."; color: win.muted; font.pixelSize: 12 }
                    }
                }

                // 03 — HARDWARE & COOLING
                Flickable {
                    contentWidth: width; contentHeight: hwContent.implicitHeight + 70
                    clip: true
                    ColumnLayout {
                        id: hwContent
                        x: 36; y: 30; width: Math.min(parent.width - 72, 900)
                        spacing: 16
                        PageHeader { heading: "Hardware & Cooling"; caption: "Native ASUS controls · changes are staged until you confirm Apply."; Layout.fillWidth: true }
                        RowLayout {
                            Layout.fillWidth: true
                            MonoText { text: (hardwareManager.state.model || "Detecting model…") + " · " + systemMonitor.coolingMode; color: win.muted; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                            ConsoleButton { text: "Inspect"; enabled: !hardwareManager.busy; onClicked: hardwareManager.inspect() }
                        }
                        MonoText { text: hardwareManager.state.wmi || "Discovering ASUS interface…"; color: win.faint; font.pixelSize: 11; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                        Row {
                            spacing: 29
                            Repeater {
                                model: ["CPU", "GPU", "COOLING"]
                                delegate: Item {
                                    required property int index
                                    required property string modelData
                                    width: tabText.implicitWidth; height: 29
                                    MonoText { id: tabText; anchors.top: parent.top; text: modelData; color: win.hardwareTab === index ? win.cyan : win.muted; font.letterSpacing: 1.3 }
                                    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 2; color: win.accent; visible: win.hardwareTab === index }
                                    MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: win.hardwareTab = index }
                                }
                            }
                        }
                        ThinRule { }
                        StackLayout {
                            Layout.fillWidth: true
                            currentIndex: win.hardwareTab
                            ColumnLayout {
                                spacing: 14
                                Row { spacing: 52
                                    Column { spacing: 3
                                        SectionTag { text: "TEMPERATURE" }
                                        MonoText { text: systemMonitor.cpuTemperatureC >= 0 ? systemMonitor.cpuTemperatureC.toFixed(1) + "°C" : "Unavailable"; font.pixelSize: 20 } }
                                    Column { spacing: 3
                                        SectionTag { text: "UTILIZATION" }
                                        MonoText { text: systemMonitor.cpuPercent < 0 ? "Unavailable" : systemMonitor.cpuPercent.toFixed(1) + "%"; font.pixelSize: 27 } }
                                    Column { spacing: 3
                                        SectionTag { text: "PACKAGE POWER" }
                                        MonoText { text: systemMonitor.cpuPackagePowerWatts >= 0 ? systemMonitor.cpuPackagePowerWatts.toFixed(1) + " W" : "Unavailable"; font.pixelSize: 20 } }
                                }
                                MonoText { text: systemMonitor.cpuTemperatureSource; color: win.muted; font.pixelSize: 11; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                                HardwareOptions { }
                                ControlSlider { Layout.fillWidth: true; label: "Firmware CPU temperature limit"; hint: win.hardwareCap("cpuThermal").state + " · " + win.hardwareCap("cpuThermal").reason; suffix: "°C"; available: false }
                                ControlSlider { Layout.fillWidth: true; label: "Software temperature target (65 / 70 / 75°C)"; hint: "UNSUPPORTED · requires verified adjustable power limits; no hard thermal guarantee"; suffix: "°C"; available: false }
                                ControlSlider { Layout.fillWidth: true; label: "Sustained power limit (SPL)"; hint: win.hardwareCap("spl").state + " · " + win.hardwareCap("spl").reason; available: false }
                                ControlSlider { Layout.fillWidth: true; label: "Slow package power tracking (sPPT)"; hint: win.hardwareCap("sppt").state + " · " + win.hardwareCap("sppt").reason; available: false }
                                ControlSlider { Layout.fillWidth: true; label: "Fast package power tracking (fPPT)"; hint: win.hardwareCap("fppt").state + " · " + win.hardwareCap("fppt").reason; available: false }
                                HardwareOptions { setting: "boostAC"; heading: "WINDOWS CPU BOOST · AC" }
                                HardwareOptions { setting: "boostDC"; heading: "WINDOWS CPU BOOST · BATTERY" }
                            }
                            ColumnLayout {
                                spacing: 17
                                MonoText { text: systemMonitor.gpuActivity; color: win.cyan; font.pixelSize: 18; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                                Row { spacing: 50
                                    Column { spacing: 3
                                        SectionTag { text: "TEMPERATURE SNAPSHOT" }
                                        MonoText { text: win.hardwareCap("gpuPower").temperatureC >= 0 ? win.hardwareCap("gpuPower").temperatureC + "°C" : "Unavailable"; font.pixelSize: 20 } }
                                    Column { spacing: 3
                                        SectionTag { text: "POWER SNAPSHOT" }
                                        MonoText { text: win.hardwareCap("gpuPower").powerWatts >= 0 ? Number(win.hardwareCap("gpuPower").powerWatts).toFixed(1) + " W" : "Unavailable"; font.pixelSize: 20 } }
                                }
                                HardwareOptions { setting: "gpuMode"; heading: "GPU MODE" }
                                MonoText { text: "OPTIMIZED: unavailable (automatic switching not implemented). ULTIMATE: disabled (restart/display workflow unverified)."; color: win.faint; font.pixelSize: 11; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                                ConsoleButton { text: "Inspect NVIDIA (may wake GPU)"; enabled: !hardwareManager.busy; onClicked: nvidiaConsent.open() }
                                ControlSlider {
                                    Layout.fillWidth: true; label: "GPU power limit (reductions only)"
                                    hint: win.hardwareCap("gpuPower").state + " · " + win.hardwareCap("gpuPower").reason + " · snapshot only"
                                    from: win.hardwareCap("gpuPower").minimumWatts || 0
                                    to: Math.max(from, win.hardwareCap("gpuPower").maximumWatts || 0)
                                    value: hardwareManager.pending.gpuPower !== undefined ? hardwareManager.pending.gpuPower : Math.floor((win.hardwareCap("gpuPower").currentMilliwatts || 0) / 1000)
                                    available: win.hardwareCap("gpuPower").enabled && !hardwareManager.busy
                                    onEdited: (v) => hardwareManager.stage("gpuPower", v)
                                }
                                ControlSlider { Layout.fillWidth: true; label: "GPU firmware thermal target"; hint: win.hardwareCap("gpuThermal").state + " · " + win.hardwareCap("gpuThermal").reason; suffix: "°C"; available: false }
                                ControlSlider { Layout.fillWidth: true; label: "Dynamic Boost"; hint: win.hardwareCap("dynamicBoost").state + " · " + win.hardwareCap("dynamicBoost").reason; available: false }
                                MonoText { text: "No overclocking. A lower power limit or stronger cooling may reduce temperature; 75°C is not guaranteed."; color: win.muted; font.pixelSize: 11; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                            }
                            ColumnLayout {
                                spacing: 16
                                Row { spacing: 55
                                    Column { spacing: 3
                                        SectionTag { text: "CPU FAN" }
                                        MonoText { text: systemMonitor.cpuFanRpm >= 0 ? systemMonitor.cpuFanRpm + " RPM" : "Unavailable"; font.pixelSize: 27 } }
                                    Column { spacing: 3
                                        SectionTag { text: "GPU FAN" }
                                        MonoText { text: systemMonitor.gpuFanRpm >= 0 ? systemMonitor.gpuFanRpm + " RPM" : "Unavailable"; font.pixelSize: 27 } }
                                }
                                SectionTag { text: "FAN PRESET · " + systemMonitor.coolingMode }
                                Row { spacing: -1
                                    Repeater { model: [{label:"QUIET",value:2},{label:"AUTO",value:0},{label:"COOL",value:1}]
                                        ProfileButton {
                                            required property var modelData
                                            choice: modelData.label
                                            active: (hardwareManager.pending.performance !== undefined ? hardwareManager.pending.performance : win.hardwareCap("performance").value) === modelData.value
                                            enabled: win.hardwareCap("performance").enabled && !hardwareManager.busy && (modelData.value !== 1 || systemMonitor.powerSource === "AC")
                                            opacity: enabled ? 1 : 0.4
                                            onChosen: hardwareManager.stage("performance", modelData.value)
                                        }
                                    }
                                    ProfileButton { choice: "CUSTOM"; active: hardwareManager.pending.cpuCurve !== undefined || hardwareManager.pending.gpuCurve !== undefined; enabled: win.hardwareCap("cpuCurve").enabled || win.hardwareCap("gpuCurve").enabled; onChosen: win.logMessage("Edit firmware curve duties below, then Apply. CUSTOM is not a separate firmware mode.") }
                                }
                                MonoText { text: "QUIET → Silent · AUTO → Balanced · COOL → Turbo (AC only). Mode changes can reset custom curves."; color: win.muted; font.pixelSize: 11; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                                Canvas {
                                    id: fanPlot
                                    Layout.fillWidth: true; Layout.preferredHeight: 180
                                    onPaint: {
                                        var ctx = getContext("2d")
                                        ctx.clearRect(0,0,width,height)
                                        ctx.strokeStyle="#26313E"; ctx.lineWidth=1
                                        for (var i=0;i<6;i++) { var y=15+i*(height-35)/5; ctx.beginPath();ctx.moveTo(28,y);ctx.lineTo(width-20,y);ctx.stroke() }
                                        var keys=["cpuCurve","gpuCurve"]
                                        for(var k=0;k<keys.length;k++) {
                                            var pts=hardwareManager.pending[keys[k]] || win.hardwareCap(keys[k]).value || []
                                            ctx.strokeStyle=k===0 ? "#168BFF" : "#36C8FF";ctx.lineWidth=2;ctx.beginPath()
                                            for(var j=0;j<pts.length;j++) { var x=28+pts[j].temperature/100*(width-55); var yy=height-20-pts[j].duty/100*(height-40); if(j===0)ctx.moveTo(x,yy);else ctx.lineTo(x,yy) }
                                            ctx.stroke()
                                        }
                                    }
                                    onWidthChanged: requestPaint()
                                    onHeightChanged: requestPaint()
                                    Connections { target: hardwareManager; function onChanged() { fanPlot.requestPaint() } }
                                }
                                MonoText { text: "CPU blue / GPU cyan · firmware factory curves or pending edits; not live custom-curve readback"; color: win.faint; font.pixelSize: 11; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                                SectionTag { text: "CURVE BASE MODE · EXPLICIT SELECTION, NOT CURRENT MODE" }
                                Row {
                                    spacing: -1
                                    Repeater {
                                        model: [{label:"SILENT",value:2},{label:"BALANCED",value:0},{label:"TURBO",value:1}]
                                        ProfileButton {
                                            required property var modelData
                                            choice: modelData.label
                                            active: hardwareManager.state.curveMode === modelData.value
                                            enabled: !hardwareManager.busy && Object.keys(hardwareManager.pending).length === 0 && (modelData.value !== 1 || systemMonitor.powerSource === "AC")
                                            opacity: enabled ? 1 : 0.4
                                            onChosen: hardwareManager.selectCurveMode(modelData.value)
                                        }
                                    }
                                }
                                MonoText { text: "Applying custom or factory curves first applies this base mode. Active-mode readback is unavailable; avoid concurrent firmware utilities or Fn mode changes during Apply."; color: win.warning; font.pixelSize: 11; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                                FanCurveEditor { }
                                FanCurveEditor { setting: "gpuCurve"; heading: "GPU FACTORY / CUSTOM CURVE" }
                                ConsoleButton { text: "Restore factory fan curves"; enabled: !hardwareManager.busy; onClicked: hardwareManager.restoreFactoryCurves() }
                            }
                        }
                        ThinRule { }
                        MonoText { text: hardwareManager.message; color: win.muted; font.pixelSize: 12; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                        MonoText { text: "PENDING · " + JSON.stringify(hardwareManager.pending); visible: Object.keys(hardwareManager.pending).length > 0; color: win.warning; font.pixelSize: 11; Layout.fillWidth: true; wrapMode: Text.WrapAnywhere }
                        RowLayout {
                            Layout.fillWidth: true
                            ConsoleButton { text: "Restore saved modes / boost"; enabled: !hardwareManager.busy; onClicked: hardwareManager.restoreSavedSettings() }
                            Item { Layout.fillWidth: true }
                            ConsoleButton { text: "Discard"; enabled: !hardwareManager.busy; onClicked: hardwareManager.discard() }
                            ConsoleButton { text: hardwareManager.busy ? "Working…" : "Apply"; primary: true; enabled: !hardwareManager.busy && Object.keys(hardwareManager.pending).length > 0; onClicked: hardwareConsent.open() }
                        }
                        Repeater {
                            model: hardwareManager.results
                            MonoText { required property var modelData; text: (modelData.success ? "[OK] " : "[FAILED] ") + modelData.setting + " · " + modelData.message; color: modelData.success ? win.good : win.danger; font.pixelSize: 11; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                        }
                    }
                }
                // 04 — PROFILES
                Flickable {
                    contentWidth: width; contentHeight: profilesContent.implicitHeight + 70
                    clip: true
                    ColumnLayout {
                        id: profilesContent
                        x: 36; y: 30; width: Math.min(parent.width - 72, 900)
                        spacing: 16
                        PageHeader { heading: "Profiles"; caption: "Choose a system mode. This prototype does not alter Windows power plans."; Layout.fillWidth: true }
                        SectionTag { text: "ACTIVE PROFILE" }
                        Row { spacing: -1
                            Repeater { model: ["ECO", "BALANCED", "PERFORMANCE"]
                                delegate: ProfileButton { required property string modelData; choice: modelData; onChosen: win.setProfile(choice) }
                            }
                        }
                        Item { Layout.preferredHeight: 14 }
                        ThinRule { }
                        Repeater {
                            model: [
                                {name:"ECO",sub:"Longest battery life",cpu:"15 W",boost:"DISABLED",gpu:"ECO",fans:"QUIET"},
                                {name:"BALANCED",sub:"Everyday use",cpu:"25 W",boost:"EFFICIENT",gpu:"STANDARD",fans:"AUTO"},
                                {name:"PERFORMANCE",sub:"Demanding workloads",cpu:"45 W",boost:"AGGRESSIVE",gpu:"OPTIMIZED",fans:"COOL"}
                            ]
                            delegate: ColumnLayout {
                                required property var modelData
                                Layout.fillWidth: true
                                spacing: 9
                                RowLayout {
                                    Layout.fillWidth: true
                                    MonoText { text: modelData.name; color: win.profile === modelData.name ? win.accent : win.ink; font.pixelSize: 15; Layout.preferredWidth: 170 }
                                    MonoText { text: modelData.sub; color: win.muted; Layout.fillWidth: true; font.pixelSize: 12 }
                                    ConsoleButton { text: win.profile === modelData.name ? "ACTIVE" : "Use"; enabled: win.profile !== modelData.name; onClicked: win.setProfile(modelData.name) }
                                }
                                RowLayout {
                                    Layout.fillWidth: true; spacing: 18
                                    Repeater { model: ["CPU " + modelData.cpu, "BOOST " + modelData.boost, "GPU " + modelData.gpu, "FANS " + modelData.fans]
                                        MonoText { required property string modelData; text: modelData; font.pixelSize: 11; color: win.faint }
                                    }
                                }
                                ThinRule { }
                            }
                        }
                        Item { Layout.preferredHeight: 14 }
                        SectionTag { text: "AUTOMATIC SWITCHING" }
                        SimpleSetting { label: "Change profile when power source changes"; checked: win.automaticSwitch; onChanged: (v) => { win.automaticSwitch = v; win.logMessage("[SIM] auto switching: " + v) } }
                        MonoText { text: "AC → PERFORMANCE     BATTERY → ECO"; color: win.muted; font.pixelSize: 12 }
                    }
                }

                // 05 — SETTINGS
                Flickable {
                    contentWidth: width; contentHeight: settingsContent.implicitHeight + 70
                    clip: true
                    ColumnLayout {
                        id: settingsContent
                        x: 36; y: 30; width: Math.min(parent.width - 72, 720)
                        spacing: 12
                        PageHeader { heading: "Settings"; caption: "App preferences · prototype only"; Layout.fillWidth: true }
                        SectionTag { text: "STARTUP" }
                        SimpleSetting { label: "Launch at Windows startup"; checked: win.startup; onChanged: (v) => win.startup = v }
                        SimpleSetting { label: "Start minimized"; checked: win.startMinimized; onChanged: (v) => win.startMinimized = v }
                        SimpleSetting { label: "Minimize to system tray"; checked: win.trayEnabled; onChanged: (v) => win.trayEnabled = v }
                        Item { Layout.preferredHeight: 15 }
                        SectionTag { text: "MONITORING" }
                        SimpleSetting { label: "Enable process monitoring"; checked: win.processMonitor; onChanged: (v) => win.processMonitor = v }
                        RowLayout {
                            Layout.fillWidth: true
                            MonoText { text: "Hardware monitoring interval"; Layout.fillWidth: true }
                            ComboBox {
                                model: ["Adaptive"]; enabled: false
                                currentIndex: 0
                                implicitWidth: 110
                            }
                        }
                        ThinRule { }
                        Item { Layout.preferredHeight: 15 }
                        SectionTag { text: "CONFIGURATION" }
                        RowLayout {
                            Layout.fillWidth: true
                            MonoText { text: "Restore demo defaults"; Layout.fillWidth: true }
                            ConsoleButton {
                                text: "Restore"
                                onClicked: {
                                    win.profile="BALANCED";win.softTemp=75;win.cpuSustained=25;win.cpuBurst=35
                                    win.gpuMode="STANDARD";win.boostMode="EFFICIENT";win.fanMode="AUTO"
                                    win.startup=false;win.startMinimized=false;win.trayEnabled=true;win.processMonitor=true
                                    win.hardwareStaged=false;win.logMessage("[SIM] demo defaults restored")
                                }
                            }
                        }
                        ThinRule { }
                        Item { Layout.preferredHeight: 18 }
                        MonoText { text: "CTRL  /  NATIVE MONITORING  /  NATIVE ASUS CONTROLS / CAPABILITY-GATED"; color: win.faint; font.pixelSize: 10 }
                    }
                }
            }
            // Collapsible console bar, always docked at the bottom.
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: win.consoleExpanded ? 210 : 28
                color: win.surface
                border.color: win.border
                Behavior on Layout.preferredHeight { NumberAnimation { duration: 190; easing.type: Easing.OutCubic } }
                ColumnLayout {
                    anchors.fill: parent; spacing: 0
                    Rectangle {
                        Layout.fillWidth: true; Layout.preferredHeight: 28
                        color: consoleMouse.containsMouse ? win.surfaceRaised : "transparent"
                        RowLayout {
                            anchors.fill: parent; anchors.leftMargin: 36; anchors.rightMargin: 36; spacing: 8
                            MonoText { text: "ctrl@system >"; color: win.accent; font.pixelSize: 11 }
                            MonoText { text: win.consoleExpanded ? "console" : win.consoleMessage; color: win.faint; font.pixelSize: 11; elide: Text.ElideRight; Layout.fillWidth: true }
                            MonoText { text: win.consoleExpanded ? "Ctrl+` close" : "Ctrl+` open"; color: win.faint; font.pixelSize: 11 }
                        }
                        MouseArea { id: consoleMouse; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: { win.consoleExpanded = !win.consoleExpanded; if (win.consoleExpanded) commandInput.forceActiveFocus() } }
                    }
                    ColumnLayout {
                        Layout.fillWidth: true; Layout.fillHeight: true
                        visible: win.consoleExpanded; spacing: 8
                        Item { Layout.preferredHeight: 8 }
                        MonoText { text: "[INFO] Windows telemetry: LIVE when reported · hardware controls: staged Apply; profiles/processes: demo"; color: win.muted; font.pixelSize: 12; Layout.leftMargin: 36 }
                        MonoText { text: win.consoleMessage; color: win.cyan; font.pixelSize: 12; Layout.leftMargin: 36; Layout.fillWidth: true; wrapMode: Text.Wrap }
                        Item { Layout.fillHeight: true }
                        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: win.line }
                        RowLayout {
                            Layout.fillWidth: true; Layout.preferredHeight: 34; Layout.leftMargin: 36; Layout.rightMargin: 36
                            MonoText { text: "ctrl@system >"; color: win.accent }
                            TextField {
                                id: commandInput
                                Layout.fillWidth: true
                                color: win.ink; font.family: win.mono; font.pixelSize: 12
                                placeholderText: "type help"
                                placeholderTextColor: win.faint
                                background: Item {}
                                onAccepted: { win.command(text); text = "" }
                            }
                        }
                    }
                }
            }
        }
    }
}
