var gateway = `ws://${window.location.hostname}/ws`;
var websocket;
window.addEventListener('load', onload);

function onload(event) {
    initWebSocket();
}

function initWebSocket() {
    console.log('Trying to open a WebSocket connection…');
    websocket = new WebSocket(gateway);
    websocket.onopen = onOpen;
    websocket.onclose = onClose;
    websocket.onmessage = onMessage;
}

function onOpen(event) {
    console.log('Connection opened');
}

function onClose(event) {
    console.log('Connection closed');
    setTimeout(initWebSocket, 2000);
}

function submitCalibFactor1(){
    var calibFactor1 = document.getElementById("calFact1Input").value;
    websocket.send("calib1:" + calibFactor1);
}

function submitCalibFactor2(){
    var calibFactor2 = document.getElementById("calFact2Input").value;
    websocket.send("calib2:" + calibFactor2);
}

function submitCalibFactor3(){
    var calibFactor3 = document.getElementById("calFact3Input").value;
    websocket.send("calib3:" + calibFactor3);
}

function submitCalibFactor4(){
    var calibFactor4 = document.getElementById("calFact4Input").value;
    websocket.send("calib4:" + calibFactor4);
}

function submitNetworkSSID(){
	var networkSSID = document.getElementById("newNetworkSSID").value;
	websocket.send("ssid:" + networkSSID);
}

function submitNetworkPassword(){
	var networkPassword = document.getElementById("newNetworkPassword").value;
	websocket.send("password:" + networkPassword);
}