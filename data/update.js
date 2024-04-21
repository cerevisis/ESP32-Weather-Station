var gateway = `ws://${window.location.hostname}/ws`;
var websocket;
window.addEventListener('load', onload);

function onload(event) {
	initWebSocket();
	rearrangeNav();
	loadErrorLogTable();
	darkmode("Dtoggle5");
	//	logLevelLabels();

}
//window.addEventListener("resize", rearrangeNav); // call the event listener based on the resized window

function rearrangeNav() {
	var desktopStats = document.getElementById("desktop-stats"); // retrieve the navigation id
	var mobileStats = document.getElementById("mobile-stats");

	if (document.body.clientWidth <= 922) // MOBILE
	{
		// remove the div
		mobileStats.insertAdjacentHTML('afterbegin', '<ul><li><span id="loadVoltage">Pending</span> V</li><li><span id="currentmA">Pending</span> mA</li><li><span id="power_mW">Pending</span> mW</li><li><span id="rssi">Pending</span> dB</li><li><span id="timeStamp">Pending</span></li></ul>');
	}

	else if (document.body.clientWidth > 922) // DESKTOP
	{
		// remove the div
		desktopStats.insertAdjacentHTML('afterbegin', '<ul><li><strong style="margin-right: 12px;">Status</strong></li><li><span id="loadVoltage">Pending</span> V</li><li><span id="currentmA">Pending</span> mA</li><li><span id="power_mW">Pending</span> mW</li><li><span id="rssi">Pending</span> dB</li><li><span id="timeStamp">Pending</span></li></ul>');
	}
}


function getValues() {
	websocket.send("getValues");
	console.log('getValues');
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
	getValues();
}

function onClose(event) {
	console.log('Connection closed');
	setTimeout(initWebSocket, 1000);
}


function updateToggle(element) {
	var idLength = element.id.length;
	var togType = element.id.charAt(0);
	console.log("togType: " + togType + " idLength: " + idLength);
	if (idLength == 8) {
		var toggNumber = element.id.substr(idLength - 1, 1);
	}
	else if (idLength == 9) {
		var toggNumber = element.id.substr(idLength - 1, 2);
	}
	var isChecked = document.getElementById(element.id).checked;
	console.log("toggle: " + toggNumber + " " + isChecked);

	document.getElementById(togType + "toggle" + toggNumber).innerHTML = isChecked;
	if (isChecked == true) {
		websocket.send(toggNumber + togType + "1");
		console.log("toggle msg: " + toggNumber + togType + "1");
	}
	else if (isChecked == false) {
		websocket.send(toggNumber + togType + "0");
		console.log("toggle msg: " + toggNumber + togType + "0");
	}
}



function updateTimeInput(element) {
	var idLength = element.id.length;
	var timeType = element.id.charAt(0);
	var time = document.getElementById(element.id).value;
	console.log("timeType: " + timeType + " idLength: " + idLength);
	if (idLength == 6) {
		var timeNumber = element.id.substr(idLength - 1, 1);
	}
	else if (idLength == 7) {
		var timeNumber = element.id.substr(idLength - 1, 2);
	}

	websocket.send(timeNumber + timeType + time);
	console.log("time msg: " + timeNumber + timeType + time);

}

function sendForm(element) {

	var idLength = element.id.length;
	var formType = element.id.charAt(0);
	var value = document.getElementById(element.id).value;
	console.log("formType: " + formType + " idLength: " + idLength);
	if (idLength == 6) {
		var formNumber = element.id.substr(idLength - 1, 1);
	}
	else if (idLength == 7) {
		var formNumber = element.id.substr(idLength - 1, 2);
	}
	else if (idLength == 8) {
		var formNumber = element.id.substr(idLength - 1, 3);
	}

	websocket.send(formNumber + formType + value);
	console.log("form msg: " + formNumber + formType + value);


}

function getDropdown(element) {
	var dropDown = document.getElementById(element.id);
	var selectedValue = dropDown.options[dropDown.selectedIndex].text;
	console.log("Selected Text: " + dropDown + " Value: " + selectedValue);

	var idLength = element.id.length;
	var dropType = element.id.charAt(0);
	console.log("dropType: " + dropType + " idLength: " + idLength);
	if (idLength == 8) {
		var dropNumber = element.id.substr(idLength - 1, 1);
	}
	else if (idLength == 9) {
		var dropNumber = element.id.substr(idLength - 1, 2);
	}


	websocket.send(dropNumber + dropType + selectedValue);
	console.log("toggle msg: " + dropNumber + dropType + selectedValue);
}


function onMessage(event) {
	console.log("onMessage: " + event.data);
	var jsonMsg = JSON.parse(event.data);
	var keys = Object.keys(jsonMsg);

	for (var i = 0; i < keys.length; i++) {
		var key = keys[i];

		if ((key.includes("toggle")) && (!key.includes("Dtoggle5"))) {
			if (jsonMsg[key] == "1") {
				document.getElementById(key).checked = true;
			}
			if (jsonMsg[key] == "0") {
				document.getElementById(key).checked = false;
			}

		}
		else if (key.includes("terminal")) {
			var textMsg = jsonMsg[key] + "\n";
			var textarea = document.getElementById("terminalBoxAreaID");
			document.terminalTextBox.terminalBoxArea.value += textMsg;
			textarea.scrollTop = textarea.scrollHeight;
		}
		else if (key.includes("winddir")) {
			var Data = jsonMsg[key];
			var dirdata = Data - 180;
			document.getElementById(key).innerHTML = Data;
			
			const winDirArrow = document.getElementById("arrow-wrapper");

			winDirArrow.style.transform = 'translateX(-50%) rotate(' + dirdata + 'deg)';

		}
		else {
			var Data = jsonMsg[key];
			document.getElementById(key).innerHTML = Data;
		}

	}
}

setInterval(clearLogs, 30000);

function clearLogs() {
	console.clear();
}

function loadErrorLogTable() {

	fetch('systemLog.csv')
		.then(function (response) {
			return response.text();
		})
		.then(function (data) {
			var table_container = document.getElementById('errorTable');
			table_container.innerHTML = "";
			csv_string_to_table(data, table_container);
		});

	function csv_string_to_table(csv_string, element_to_insert_table) {
		var rows = csv_string.trim().split(/\r?\n|\r/); // Regex to split/separate the CSV rows
		var table = '';
		var table_rows = '';
		var table_header = '';

		rows.forEach(function (row, row_index) {
			var table_columns = '';
			var columns = row.split(','); // split/separate the columns in a row
			columns.forEach(function (column, column_index) {
				table_columns += row_index == 0 ? '<th>' + column + '</th>' : '<td class="logLevelLabel">' + column + '</td>';
			});
			if (row_index == 0) {
				table_header += '<tr>' + table_columns + '</tr>';
			} else {
				table_rows += '<tr>' + table_columns + '</tr>';
			}
		});

		table += '<table>';
		table += '<thead>';
		table += table_header;
		table += '</thead>';
		table += '<tbody>';
		table += table_rows;
		table += '</tbody>';
		table += '</table>';

		element_to_insert_table.innerHTML += table;


	}
	var td = document.querySelectorAll('td')
	for (let cell of td) {
		if (cell.innerHTML === 'STATUS') {
			cell.style.backgroundColor = 'blue'
		}
		if (cell.innerHTML === 'WARNING') {
			cell.style.backgroundColor = 'red'
		}
	}

}
setInterval(loadErrorLogTable, 15000);


