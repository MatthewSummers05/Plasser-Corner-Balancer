var gateway = `ws://${window.location.hostname}/ws`;
var websocket;
// Init web socket when the page loads
window.addEventListener('load', onload);

function onload(event) {
    initWebSocket();
}

function getReadings(){
    websocket.send("getReadings");
}

function initWebSocket() {
    console.log('Trying to open a WebSocket connection…');
    websocket = new WebSocket(gateway);
    websocket.onopen = onOpen;
    websocket.onclose = onClose;
    websocket.onmessage = onMessage;
}

// When websocket is established, call the getReadings() function
function onOpen(event) {
    console.log('Connection opened');
    getReadings();
}

function onClose(event) {
    console.log('Connection closed');
    setTimeout(initWebSocket, 2000);
}

// Function that receives the message from the ESP32 with the readings
function onMessage(event) {
	if (event.data === "error") {
        // Execute code when error is received from ESP
        // For example, redirect to error page
        window.location.href = 'error.html';
    }
	else{
    console.log(event.data);
    var myObj = JSON.parse(event.data);
    var keys = Object.keys(myObj);

    for (var i = 0; i < keys.length; i++){
        var key = keys[i];
        document.getElementById(key).innerHTML = myObj[key];
		}
	}
}

async function downloadPDF(){
	/*weight1 = front Left
	  weight2 = front Right
	  weight3 = back Left
	  weight4 = back right */
      const weight1 = parseFloat(document.getElementById('weight1').textContent);
      const weight2 = parseFloat(document.getElementById('weight2').textContent);
      const weight3 = parseFloat(document.getElementById('weight3').textContent);
      const weight4 = parseFloat(document.getElementById('weight4').textContent);
      const totalWeight = parseFloat(document.getElementById('totalWeight').textContent);
	  const fuelTank = parseFloat(document.getElementById('fuelTank').value);
	  const hydraulicTank = parseFloat(document.getElementById('hydraulicTank').value);
	  const waterTank = parseFloat(document.getElementById('waterTank').value);
	  const machineNumber = (document.getElementById('machineNumber').value);
	  
      //Calculating all values to place onto the pdf
      const frontLeftWeightRatio = weight1/totalWeight;
	  const backLeftWeightRatio = weight3/totalWeight;
	  const frontRightweightRatio = weight2/totalWeight;
	  const backRightWeightRatio = weight4/totalWeight;
	  const totalLeftWeight = weight1 + weight3;
	  const totalRightWeight = weight2 + weight4;
	  const totalLeftRatio = totalLeftWeight/totalWeight;
	  const totalRightRatio = totalRightWeight/totalWeight;
	  const percentageDiffLeftToRight = totalLeftRatio - totalRightRatio;
	  const perAxleFront = weight1 + weight2;
	  const perAxleBack = weight3 + weight4;
	  const perAxleFrontRatio = perAxleFront/totalWeight;
	  const perAxleBackRatio = perAxleBack/totalWeight;
	  const percentageDiffRearToFront = perAxleBackRatio - perAxleFrontRatio;
	  
	  // Creating the rounded off nubers
	  const roundedWeight1 = weight1.toFixed(0);
	  const roundedWeight2 = weight2.toFixed(0);
	  const roundedWeight3 = weight3.toFixed(0);
	  const roundedWeight4 = weight4.toFixed(0);
	  const roundedTotalWeight = totalWeight.toFixed(0);
	  const roundedTotalLeftWeight = totalLeftWeight.toFixed(0);
	  const roundedTotalRIghtWeight = totalRightWeight.toFixed(0);
	  const roundedPerAxleFront = perAxleFront.toFixed(0);
	  const roundedPerAxleRear = perAxleBack.toFixed(0);
	  const roundedFuelTank = fuelTank.toFixed(0);
	  const roundedWaterTank = waterTank.toFixed(0);
	  const roundedHydraulicTank = hydraulicTank.toFixed(0);
	  
	  const roundedFrontLeftWeightRatio = frontLeftWeightRatio.toFixed(1);
	  const roundedFrontRightWeightRatio = frontRightweightRatio.toFixed(1);
	  const roundedBackLeftWeightRatio = backLeftWeightRatio.toFixed(1);
	  const roundedBackRightWeightRatio = backRightWeightRatio.toFixed(1);
	  const roundedTotalLeftRatio = totalLeftRatio.toFixed(1);
	  const roundedTotalRightRatio = totalRightRatio.toFixed(1);
	  const roundedPercentageDiffLeftToRight = percentageDiffLeftToRight.toFixed(1);
	  const roundedPercentageDiffRearToFront = percentageDiffRearToFront.toFixed(1);
	  const roundedPerAxleFrontRatio = perAxleFrontRatio.toFixed(1);
	  const roundedPerAxleRearRatio = perAxleBackRatio.toFixed(1);

      // Fetch the PDF file
      const response = await fetch('WeightSheetWithLogo.pdf');
      const pdfBytes = await response.arrayBuffer();
      const pdfDoc = await PDFLib.PDFDocument.load(pdfBytes);

      const page = pdfDoc.getPage(0); // Modify the first page, change index as needed
      
      // Add values to the PDF
      const { width, height } = page.getSize();
      page.drawText(`${roundedWeight1}kg`, {x: 507, y: 437, size: 10 });
	  page.drawText(`${roundedFrontLeftWeightRatio}%`, {x: 507, y: 422, size: 10 });
	  page.drawText(`${roundedWeight2}kg`, {x: 507,y: 301, size: 10 });
	  page.drawText(`${roundedFrontRightWeightRatio}%`, {x: 507, y: 287, size: 10 });
	  page.drawText(`${roundedWeight3}kg`, {x: 223,y: 438, size: 10 });
	  page.drawText(`${roundedBackLeftWeightRatio}%`, {x: 223,y: 422,size: 10} );
	  page.drawText(`${roundedWeight4}kg`, {x: 223,y: 301,size: 10} );
	  page.drawText(`${roundedBackRightWeightRatio}%`, {x: 223,y: 286,size: 10} );
	  page.drawText(`${roundedTotalLeftWeight}kg`, {x: 393,y: 437,size: 10} );
	  page.drawText(`${roundedTotalRIghtWeight}kg`, {x: 393,y: 301,size: 10} );
	  page.drawText(`${roundedTotalWeight}kg`, {x: 355,y: 249,size: 10} );
	  page.drawText(`${roundedPerAxleRear}kg`, {x: 165,y: 369,size: 10} );
	  page.drawText(`${roundedPerAxleFront}kg`, {x: 564,y: 369,size: 10} );
	  page.drawText(`${roundedPerAxleRearRatio}%`, {x: 165,y: 355,size: 10} );
	  page.drawText(`${roundedPerAxleFrontRatio}%`, {x: 564,y: 355,size: 10} );
	  page.drawText(`${roundedPercentageDiffLeftToRight}%`, {x: 147,y: 249,size: 10} );
	  page.drawText(`${roundedPercentageDiffRearToFront}%`, {x: 469,y: 249,size: 10} );
	  page.drawText(`${roundedTotalLeftRatio}%`, {x: 393,y: 424,size: 10} );
	  page.drawText(`${roundedTotalRightRatio}%`, {x: 393,y: 286,size: 10} );
	  page.drawText(`${machineNumber}`, {x: 507, y: 491, size: 10 });
	  page.drawText(`${roundedFuelTank}kg`, {x: 222,y: 465,size: 10} );
      page.drawText(`${roundedHydraulicTank}kg`, {x: 374,y: 465,size: 10} );
      page.drawText(`${roundedWaterTank}kg`, {x: 544,y: 465,size: 10} );

      // Save the modified PDF
      const modifiedPdfBytes = await pdfDoc.save();

      // Download the modified PDF
      const blob = new Blob([modifiedPdfBytes], { type: 'application/pdf' });
      const link = document.createElement('a');
      link.href = window.URL.createObjectURL(blob);
	  pdfName = (document.getElementById('machineNumber').value);
      link.download = `report_${pdfName}.pdf`;
      link.click();
}

function restartESP32(){
    websocket.send("restart");
}

function errorRestart(){
	websocket.send("restart");
	window.location.href = 'index.html';
}

function tare(){
	websocket.send("tare");
}

