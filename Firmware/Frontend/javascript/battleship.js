var shipsSet = false;
var webShips = document.getElementById("webShips");
var boardShips = document.getElementById("boardShips");
var shipNum=0;
var webShipCoords=[];
var boardShipCoords=[];
var settingCoords=[];
var shipLength=0;
var startCoord;
var turn=true;
var obj;
var messageArrived=false;

var player ;
var mode ;
var posi;


var mqtt;
var reconnectTimeout =2000;
var host="192.168.131.125"; //SchoolServer
var port=8080;

for (let i = 1; i <= 100; i++) {
    var boardField = document.createElement("button");
    var webField = document.createElement("button");

    boardField.innerText = i;
    webField.innerText = i;

    boardField.className = "boardButton";
    webField.className = "webButton";

    boardField.id = i+100;
    webField.id = i;

    boardField.onclick = function() { kill(i); };
    webField.onclick = function() { addShip(i); };

    boardShips.append(boardField);
    webShips.append(webField);

    if (i % 10 == 0){
        boardShips.append(document.createElement("br"));
        webShips.append(document.createElement("br"));
    }
}
MQTTconnect();

function kill(coord) {
    if(mqtt.isConnected) {
        if (shipsSet && boardShipCoords.length != 0 && turn) {
            var obj = {Mode: 0, Player: true, Posi: coord};
            var myJson = JSON.stringify(obj);
            sendMessage("Battleship", myJson);
            console.log("KILL");
            console.log(coord);
        } else {
            alert("You can't hit yet!");
        }
    }
    else{
        MQTTconnect();
    }
};



function addShip(coord) {
    console.log("add ship");
    console.log(coord);
    var match=false;

if((shipNum>=0)&&(shipNum<=5)) {
    if (shipNum == 0) {
        shipLength = 5;
    } else if (shipNum == 1) {
        shipLength = 4;
    } else if ((shipNum == 2) || (shipNum == 3)) {
        shipLength = 3;
    } else if (shipNum == 4) {
        shipLength = 2;
    } else if (shipNum == 5) {
        shipLength = 1;
    }
    for (let i = 0; i < webShipCoords.length; i++) {
        for (let j = coord; j <= coord+shipLength; j++) {
            if (j == webShipCoords[i]) {
                match = true;
            }
        }

    }
    if (match == false) {

        var tot = (coord + shipLength - 1);
        if ((settingCoords != null) && (coord != startCoord) && (startCoord != null)) {
            if (document.getElementById(startCoord + 1).className === "Ship") {
                for (let i = 0; i < shipLength; i++) {
                    var ship = document.getElementById(startCoord + i);
                    ship.className = "webButton";
                    for (let j = 0; j < settingCoords.length; j++) {
                        if (settingCoords[j] == startCoord + i) {
                            settingCoords.splice(j, 1);
                        }
                    }
                }
            } else {
                for (let i = 0; i < shipLength * 10; i += 10) {
                    var ship = document.getElementById(startCoord + i);
                    ship.className = "webButton";
                    for (let j = 0; j < settingCoords.length; j++) {
                        if (settingCoords[j] == startCoord + i) {
                            settingCoords.splice(j, 1);
                        }
                    }
                }
            }

            if (tot <= 100) {
                if (checkDuplicate() === true) {
                    alert("Overlapping Ships");
                } else {
                    if ((Math.floor((coord + shipLength - 1) / 10)) == (Math.floor(coord / 10)) || (((coord + shipLength - 1) % 10) == 0)) {
                        for (let i = 0; i < shipLength; i++) {
                            var ship = document.getElementById(coord + i);
                            ship.className = "Ship";
                            settingCoords.push(coord + i);
                        }
                        startCoord = coord;
                    }
                }
            }

        } else {

            if (tot <= 100) {
                if ((Math.floor((coord + shipLength - 1) / 10)) == (Math.floor(coord / 10)) || (((coord + shipLength - 1) % 10) == 0)) {
                    if (checkDuplicate() === true) {
                        alert("Overlapping Ships");
                    } else {
                        for (let i = 0; i < shipLength; i++) {
                            var ship = document.getElementById(coord + i);
                            ship.className = "Ship";
                            settingCoords.push(coord + i);
                        }
                        startCoord = coord;
                    }
                }
            }
        }
    }
    else{
        alert("There is already a ship there");
    }
}
else{
    alert("You can't place more ships!");
}
};

function rotateShip() {

    if (document.getElementById(startCoord + 1).className === "Ship") {

            if ((startCoord + ((shipLength - 1) * 10)) <= 100) {
                for (let i = 0; i < shipLength; i++) {
                    var ship = document.getElementById(startCoord + i);
                    var match=false;
                    for (let j = 0; j < webShipCoords.length; j++) {
                        if((startCoord+i)==webShipCoords[j]){
                            match=true;
                        }
                    }
                    if(!match) ship.className = "webButton";

                }

                for (let i = 0; i < shipLength * 10; i += 10) {
                    var ship = document.getElementById(startCoord + i);
                    ship.className = "Ship";
                    settingCoords[i/10]=startCoord+i;
                }
            }

}
    else{

            for (let i = 1; i < shipLength; i++) {
                var ship = document.getElementById(startCoord + i);
                ship.className = "Ship";
                settingCoords[i]=startCoord + i;
            }

            for (let i = 10; i < shipLength * 10; i += 10) {
                var ship = document.getElementById(startCoord + i);
                var match=false;
                for (let j = 0; j < webShipCoords.length; j++) {
                    if((startCoord+i)==webShipCoords[j]){
                        match=true;
                    }
                }
                if(!match) ship.className = "webButton";

            }
        }
    if (checkDuplicate()===true) {
        alert("Overlapping Ships");
        rotateShip();
    }
    for (let i = 0; i < shipLength; i++) {
        console.log("settingCoords["+i+"]: "+settingCoords[i]);
    }
};

function setShip() {
    if(mqtt.isConnected) {
        if ((shipNum >= 0) && (shipNum <= 5)) {
            if ((webShipCoords != null) && (settingCoords != null)) {
                if (checkDuplicate() === true) {
                    alert("Overlapping Ships");
                } else {
                    for (let i = 0; i < settingCoords.length; i++) {
                        webShipCoords.push(settingCoords[i]);
                        document.getElementById(settingCoords[i]).className = "Ship";

                    }
                    shipNum++;
                }
                settingCoords = [];
                startCoord = null;
            } else {
                for (let i = 0; i < settingCoords.length; i++) {
                    webShipCoords.push(settingCoords[i]);
                    document.getElementById(settingCoords[i]).className = "Ship";

                }
                shipNum++;
                settingCoords = [];
                startCoord = null;
            }
            for (let i = 0; i < webShipCoords.length; i++) {
                console.log("webShipCoords[" + i + "]");
                console.log(webShipCoords[i]);
            }
        } else {
            console.log("HERE");

            sendShips();
        }
    }
    else{
        MQTTconnect();
    }
};

function sendShips(){
    if(!shipsSet){
        var obj ={"Mode":1, "Player": true, "Posi":webShipCoords};
        var msg=JSON.stringify(obj);
        console.log(msg);
        sendMessage("Battleship", msg);
        shipsSet=true;
    }
}


function checkDuplicate() {
    if((webShipCoords!=null)&&(settingCoords!=null)) {
        for (let i = 0; i < webShipCoords.length; i++) {
            for (let j = 1; j < settingCoords.length; j++) {
                if (webShipCoords[i] === settingCoords[j])
                    return true;
            }
        }
    }
    return false;
}




// MQTT
function sendMessage(topic, text) {
    if(!(mqtt.isConnected)){
        MQTTconnect();
    }
    else {
        console.log("sending");
        var msg = new Paho.MQTT.Message(text);
        msg.destinationName = topic;
        mqtt.send(msg);

        console.log(
            "Message sent:\n" +
            "Topic: " + topic + "\n" +
            text
        );
    }
}

function onFailure() {
    console.log("Connection to " + host + ":" + port + " failed");
    setTimeout(MQTTconnect, reconnectTimeout);
}

function onMessageArrived(msg) {
    console.log(msg);
    if (msg.destinationName === "Battleship") {
         obj = JSON.parse(msg);
    }
        console.log(
            "Received Message:\n" +
            "Topic: " + msg.destinationName + "\n" +
            msg.payloadString
        );
        messageArrived=true;

}




setInterval(function () {
    if(messageArrived){

        player = obj.Player;
        mode = obj.Mode;
        posi = obj.Posi;
        console.log("message arrived");
        if (mode === 1) {
            if (player === false) {
                for (let i = 0; i < posi.length; i++) {
                    boardShipCoords[i] = posi[i];
                }
            }
        } else {
            if (player === false && turn === false) {
                var match = false;
                for (let i = 0; i < webShipCoords.length; i++) {
                    if (webShipCoords[i] === posi) {
                        match = true;
                    }
                }
                if (match) {
                    document.getElementById(posi).className = "webHit";
                    webShips.children[posi].innerText = "HIT";
                } else {
                    document.getElementById(posi).className = "webMiss";
                    webShips.children[posi].innerText = "MISS";
                }
                turn = !turn;
            } else if (player === true && turn === true) {
                var match = false;
                for (let i = 0; i < boardShipCoords.length; i++) {
                    if (boardShips[i] === posi) {
                        match = true;
                    }
                }
                if (match) {
                    document.getElementById(posi).className = "boardHit";
                    boardShips.children[posi].innerText = "HIT";
                } else {
                    document.getElementById(posi).className = "boardMiss";
                    boardShips.children[posi].innerText = "MISS";
                }
                turn = !turn;
            }
        }
        messageArrived=false;
        MQTTconnect();
    }
}, 1000);


function onConnect() {
    console.log("connected");
    mqtt.subscribe("Battleship");
}

function MQTTconnect() {
    console.log("Connecting to " + host + ":" + port);

    mqtt = new Paho.MQTT.Client(host, port, "a0c3b989a4e04e32a422fead22fc3ab8");

    var options = {
            timeout: 3,
            userName: "user",
            password: "userpw",
            onSuccess: onConnect,
            onFailure: onFailure
    };

    mqtt.onMessageArrived = onMessageArrived;
    mqtt.connect(options);
}