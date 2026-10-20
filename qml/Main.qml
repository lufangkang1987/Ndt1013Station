import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

ApplicationWindow {
    visible: true
    width: 1380; height: 900; minimumWidth: 1000; minimumHeight: 700
    title: "NDT1013 · 射线图像采集工作站"
    color: "#101820"
    font.family: "Microsoft YaHei UI"
    font.pixelSize: 14
    palette.window: "#16212c"
    palette.windowText: "#e1e9f2"
    palette.text: "#e1e9f2"
    palette.buttonText: "#e1e9f2"
    palette.base: "#101820"
    palette.button: "#263b4b"
    palette.highlight: "#20b9ad"
    palette.highlightedText: "#071c20"
    palette.placeholderText: "#a4b5c6"
    palette.disabled.windowText: "#91a3b4"
    palette.disabled.text: "#91a3b4"
    palette.disabled.buttonText: "#91a3b4"
    FileDialog {
        id: rawDialog
        title: "打开 RAW 图像（需同名 JSON）"
        nameFilters: ["RAW 灰度图像 (*.raw)"]
        fileMode: FileDialog.OpenFile
        onAccepted: station.openRaw(selectedFile)
    }
    header: ToolBar {
        implicitHeight: 64
        background: Rectangle { color: "#203442" }
        RowLayout {
            anchors.fill: parent; anchors.margins: 12
            Label { text: "NDT / 1013"; font.pixelSize: 24; font.bold: true; color: "#38d6c4" }
            Label { text: "探测器采集工作站 · 开发版"; Layout.fillWidth: true; color: "#e1e9f2"; leftPadding: 16 }
            Label { text: station.status; color: "#74d9cc" }
        }
    }
    RowLayout {
        anchors.fill: parent; anchors.margins: 18; spacing: 18
        ColumnLayout {
            Layout.preferredWidth: 330; Layout.minimumWidth: 330; Layout.maximumWidth: 330; Layout.fillHeight: true; spacing: 12
            Label { text: "设备与连接"; font.pixelSize: 20; font.bold: true }
            Switch {
                text: "模拟模式"; checked: station.simulation
                enabled: !station.connected && !station.busy
                onToggled: station.simulation = checked
            }
            Label { text: "设备工作目录" }
            TextField {
                id: workDir; text: defaultWorkDir; Layout.fillWidth: true
                implicitWidth: 1
                implicitHeight: 40
                enabled: !station.connected && !station.busy; selectByMouse: true
                color: enabled ? "#e1e9f2" : "#91a3b4"
                background: Rectangle { color: "#192a36"; radius: 4; border.color: workDir.activeFocus ? "#38d6c4" : "#455f72" }
                ToolTip.visible: hovered
                ToolTip.text: text
            }
            Label { text: "NDT1013LA 配置，完整型号须与铭牌核对。\n实机 IP 及校正模板需现场确认。"; wrapMode: Text.Wrap; Layout.fillWidth: true; color: "#98a9ba" }
            RowLayout {
                Button { text: "连接"; enabled: !station.connected && !station.busy; onClicked: station.connectDevice(workDir.text) }
                Button { text: "断开 / 重置"; onClicked: station.disconnectDevice() }
            }
            Button { text: "读取状态与图像尺寸"; enabled: station.connected && !station.busy; onClicked: station.readStatus() }
            Rectangle { Layout.fillWidth: true; height: 1; color: "#304555" }
            Label { text: "图像采集"; font.pixelSize: 20; font.bold: true }
            Button { text: station.simulation ? "生成模拟图像" : "单次清场采集 · ClearAcq"; Layout.fillWidth: true; enabled: station.connected && !station.busy; onClicked: station.capture() }
            Label { text: "实机入口发送 Cmd_ClearAcq。需先确认触发方式与厂家流程；本软件不控制射线源。"; wrapMode: Text.Wrap; Layout.fillWidth: true; color: "#98a9ba" }
            Button { text: "保存原始精度 RAW + JSON"; enabled: station.frames > 0; onClicked: station.saveFrame() }
            Button { text: "打开 RAW 图像…"; enabled: !station.connected && !station.busy; onClicked: rawDialog.open() }
            Label { text: "离线打开需先断开设备，RAW 与同名 JSON 放在同一目录。"; wrapMode: Text.Wrap; Layout.fillWidth: true; color: "#98a9ba" }
            Label { text: "已显示帧数  " + station.frames }
            Item { Layout.fillHeight: true }
            Label { text: "连续采集 / 校正 / 评片 / 报告\n后续按实机能力与需求分阶段接入"; color: "#98a9ba" }
        }
        ColumnLayout {
            Layout.fillWidth: true; Layout.fillHeight: true
            Rectangle {
                Layout.fillWidth: true; Layout.fillHeight: true; color: "#070c11"; radius: 8
                Image { anchors.fill: parent; anchors.margins: 12; source: station.imageUrl; fillMode: Image.PreserveAspectFit; cache: false }
                Label { anchors.centerIn: parent; visible: station.frames === 0; text: "等待采集图像\n可使用模拟模式验证界面与保存流程"; horizontalAlignment: Text.AlignHCenter; color: "#71879a"; font.pixelSize: 20 }
                Label { anchors.top: parent.top; anchors.right: parent.right; anchors.margins: 18; text: station.imageSimulation ? "SIMULATION · 非检测图像" : (station.imageImported ? "导入 RAW · 文件标记为非模拟" : "SDK 图像"); color: "#e7b66b" }
            }
            Label { text: "运行日志"; font.pixelSize: 16 }
            ScrollView {
                Layout.fillWidth: true; Layout.preferredHeight: 180
                TextArea {
                    text: station.logs; readOnly: true; selectByMouse: true
                    wrapMode: TextEdit.Wrap; font.family: "Microsoft YaHei UI"; font.pixelSize: 13
                    color: "#dbe7f1"; padding: 12
                    background: Rectangle { color: "#16232e"; radius: 6; border.color: "#304555" }
                }
            }
        }
    }
}
