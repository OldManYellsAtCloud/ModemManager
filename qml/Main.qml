import QtQuick 2.15
import QtQuick.Controls

import org.gspine.modem

Window {
    id: root
    visible: true

    CommandListModel{
        id: cmdListModel
        onModemMessageUpdated: {
            debugLog.text += modemMessage
            // have to subtract the scrollbar's own size, otherwise
            // it scrolls out of the screen
            debugLogScroll.ScrollBar.vertical.position = 1.0 - debugLogScroll.ScrollBar.vertical.size
        }
    }

    ComboBox {
        id: cmdComboBox
        model: cmdListModel.getData()
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: 30
        onCurrentTextChanged: {
            cmdText.text = cmdListModel.getCmd(currentText)
        }
    }

    Text {
        id: cmdText
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: cmdComboBox.bottom
        height: 30
        font.pixelSize: 20
    }

    Item {
        id: argumentItem
        visible: cmdText.text.indexOf('}') > -1
        anchors.top: cmdText.bottom
        anchors.right: parent.right
        anchors.left: parent.left
        Text {
            id: cmdArgLabel
            text: "Input cmd:"
            anchors.left: parent.left
            height: 30
            font.pixelSize: 15
        }

        Rectangle {
            anchors.left: cmdArgLabel.right
            anchors.right: parent.right
            height: 30
            color: "lightgreen"
            TextInput {
                id: argInput
                anchors.fill: parent
                font.pixelSize: 20
            }
        }
    }

    Button {
        id: goButton
        text: "Go"
        anchors.left: parent.left
        anchors.right: parent.right
        height: 30
        font.pixelSize: 20
        y: cmdComboBox.height + cmdText.height + argumentItem.height + 50
        onClicked: {
            var cmd = cmdText.text
            if (cmd.indexOf("}") > -1){
                var arg = argInput.text
                cmd = cmd.replace("{}", arg)
            }
            cmdListModel.runCmd(cmd)
        }
    }

    ScrollView {
        id: debugLogScroll
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: goButton.bottom
        anchors.bottom: parent.bottom

        TextArea {
            id: debugLog
        }
    }


}
