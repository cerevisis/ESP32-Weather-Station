const gateway = `ws://${window.location.hostname}/ws`;
var websocket;
window.addEventListener('load', onload);

// Global variables for mini-chart data
let miniRssi = 0;
let currentTemperature = 0;
let currentHumidity = 0;
let currentPressure = 0;
let currentWindSpeed = 0;
let currentRainHour = 0;

// Global variable for the historical temperature SVG chart object/instance
let historicalTempSVGChartObject;
let historicalHumSVGChartObject;
let historicalPresSVGChartObject;
let historicalWindSVGChartObject;
let historicalRainSVGChartObject;
let historicalBattVoltSVGChartObject;

// Define HISTORICAL_CHART_POINTS for the 24-hour (10-min avg) SVG chart
const HISTORICAL_CHART_POINTS = 144; // 24 hours * 6 points/hour

// Define HISTORICAL_DATA_POINTS for the ESP32's 1-minute resolution buffer (used for fullDayData)
const HISTORICAL_DATA_POINTS = 1440; // 24 hours * 60 minutes

// Client-side buffer for 24 hours of data, 5 sensors
const EMPTY_HOUR_DATA = () => ({ 
    tempData: new Array(60).fill(null), 
    humData: new Array(60).fill(null), 
    presData: new Array(60).fill(null), 
    rainData: new Array(60).fill(null), 
    windData: new Array(60).fill(null) 
});
let fullDayData = new Array(24).fill(null).map(EMPTY_HOUR_DATA); 
let clientOverallHistoryStartIndex = 0; 
let expectedHourlyChunks = 24; // For the old chunking mechanism, if ever re-enabled
let receivedHourlyChunks = 0;  // For the old chunking mechanism

// --- Responsive Chart Handling ---
let activeChartsForResize = []; // Will only contain mini-charts after this revert

function onload(event) {
	initWebSocket();
	rearrangeNav();
	loadErrorLogTable();
	initializeAllMiniCharts(); 
	initializeAllHistoricalSVGCharts(); // Initialize all SVG historical charts
	darkmode("Dtoggle5"); 
}

function rearrangeNav() {
	const desktopStats = document.getElementById("desktop-stats");
	const mobileStats = document.getElementById("mobile-stats");
	const statsHTML = '<ul><li><span id="loadVoltage">Pending</span> V</li><li><span id="currentmA">Pending</span> mA</li><li><span id="power_mW">Pending</span> mW</li><li><span id="rssi">Pending</span> dB</li><li><span id="timeStamp">Pending</span></li></ul>';
	const desktopUniqueHTML = '<strong style="margin-right: 12px;">Status</strong>';

	if (!desktopStats || !mobileStats) return;

	mobileStats.innerHTML = '';
	desktopStats.innerHTML = '';

	if (document.body.clientWidth <= 922) { // MOBILE
		mobileStats.insertAdjacentHTML('afterbegin', statsHTML);
	} else { // DESKTOP
		let desktopVersionStatsHTML = statsHTML.replace('<ul><li>', `<ul><li>${desktopUniqueHTML}</li><li>`);
		desktopStats.insertAdjacentHTML('afterbegin', desktopVersionStatsHTML);
	}
}

function getValues() {
	if (websocket && websocket.readyState === WebSocket.OPEN) {
		websocket.send("getValues");
		console.log('Sent: getValues');
	} else {
		console.log('WebSocket not open. Cannot send getValues.');
	}
}

let reconnectIntervalId = null;

function initWebSocket() {
	console.log('Trying to open a WebSocket connection…');
	websocket = new WebSocket(gateway);
	websocket.onopen = onOpen;
	websocket.onclose = onClose;
	websocket.onmessage = onMessage;
}

function onOpen(event) {
	console.log('Connection opened');
	if (reconnectIntervalId) {
		clearTimeout(reconnectIntervalId);
		reconnectIntervalId = null;
	}
	getValues();
    // requestFullHistory(); // REVERTED: Server sends this on connect automatically. Client can request manually if needed.
}

function onClose(event) {
	console.log('Connection closed');
	if (!reconnectIntervalId) {
		reconnectIntervalId = setTimeout(initWebSocket, 3000);
	}
}

const updateElement = (id, val, toFixedDigits = -1) => {
    const el = document.getElementById(id);
    if (el) {
        let displayValue = val;
        if (typeof val === 'number' && !isNaN(val) && toFixedDigits >= 0) {
            displayValue = val.toFixed(toFixedDigits);
        }
        if (el.tagName === 'INPUT' || el.tagName === 'TEXTAREA') {
            el.value = displayValue;
            if (el.type === 'number' && el.placeholder !== undefined && (id.startsWith("Fdata") || ["dataRefresh", "rssilimit", "wifiinterval", "windRainCutoff", "deepSleepTime"].includes(id))) {
               el.placeholder = displayValue;
            }
        } else {
            el.innerHTML = displayValue;
        }
    }
};

function onMessage(event) {
    let data;
    try {
        data = JSON.parse(event.data);
        console.log("Raw data:", event.data);
    } catch (e) {
        console.error("Failed to parse JSON from WebSocket:", e, "Raw data:", event.data);
        if (typeof event.data === 'string' && document.getElementById("terminalBoxAreaID")) {
            const textarea = document.getElementById("terminalBoxAreaID");
            textarea.value += event.data + "\n";
            textarea.scrollTop = textarea.scrollHeight;
        }
        return;
    }

    if (data.historyDataType === "histHourData") { 
        console.log("Received hourly historical chunk for hourOffset:", data.hourOffset);
        if (receivedHourlyChunks === 0) {
            clientOverallHistoryStartIndex = data.historyStartIndex;
        }
        const hourOffset = data.hourOffset;
        if (hourOffset >= 0 && hourOffset < 24) {
            fullDayData[hourOffset] = { // Still populate fullDayData for potential other uses
                tempData: data.tempData || EMPTY_HOUR_DATA().tempData,
                humData:  data.humData  || EMPTY_HOUR_DATA().humData,
                presData: data.presData || EMPTY_HOUR_DATA().presData,
                rainData: data.rainData || EMPTY_HOUR_DATA().rainData,
                windData: data.windData || EMPTY_HOUR_DATA().windData
            };
        }
        receivedHourlyChunks++;
        if (receivedHourlyChunks >= expectedHourlyChunks) {
            console.log("All 24 (old style) hourly historical chunks received and stored in fullDayData.");
            receivedHourlyChunks = 0; 
        } // End of histHourData
    } else if (data.historyDataType === "hist24h_10minAvgData") {
        console.log("Received 24h historical data (10-min averages).");
        if (historicalTempSVGChartObject && data.tempData && data.tempData.length === HISTORICAL_CHART_POINTS) { // Expect 144 points
            const now = new Date().getTime();
            const tenMinutesMillis = 10 * 60 * 1000;
            let seriesDataForChart = [];
            for (let i = 0; i < HISTORICAL_CHART_POINTS; i++) {
                // Timestamps: newest point represents the 10-min block ending "now", oldest is 24 hours ago.
                const timestamp = now - ((HISTORICAL_CHART_POINTS - 1 - i) * tenMinutesMillis);
                seriesDataForChart.push([timestamp, data.tempData[i]]);
            }
            if (historicalHumSVGChartObject && data.humData && data.humData.length === HISTORICAL_CHART_POINTS) {
                let seriesHumData = [];
                for (let i = 0; i < HISTORICAL_CHART_POINTS; i++) {
                    const timestamp = now - ((HISTORICAL_CHART_POINTS - 1 - i) * tenMinutesMillis);
                    seriesHumData.push([timestamp, data.humData[i]]);
                }
                historicalHumSVGChartObject.updateFullData(seriesHumData);
            }
            if (historicalPresSVGChartObject && data.presData && data.presData.length === HISTORICAL_CHART_POINTS) {
                let seriesPresData = [];
                for (let i = 0; i < HISTORICAL_CHART_POINTS; i++) {
                    const timestamp = now - ((HISTORICAL_CHART_POINTS - 1 - i) * tenMinutesMillis);
                    seriesPresData.push([timestamp, data.presData[i]]);
                }
                historicalPresSVGChartObject.updateFullData(seriesPresData);
            }
            if (historicalWindSVGChartObject && data.windData && data.windData.length === HISTORICAL_CHART_POINTS) {
                let seriesWindData = [];
                for (let i = 0; i < HISTORICAL_CHART_POINTS; i++) {
                    const timestamp = now - ((HISTORICAL_CHART_POINTS - 1 - i) * tenMinutesMillis);
                    seriesWindData.push([timestamp, data.windData[i]]);
                }
                historicalWindSVGChartObject.updateFullData(seriesWindData);
            }
            if (historicalRainSVGChartObject && data.rainData && data.rainData.length === HISTORICAL_CHART_POINTS) {
                let seriesRainData = [];
                for (let i = 0; i < HISTORICAL_CHART_POINTS; i++) {
                    const timestamp = now - ((HISTORICAL_CHART_POINTS - 1 - i) * tenMinutesMillis);
                    seriesRainData.push([timestamp, data.rainData[i]]);
                }
                historicalRainSVGChartObject.updateFullData(seriesRainData);
            }
            if (historicalBattVoltSVGChartObject && data.battVoltData && data.battVoltData.length === HISTORICAL_CHART_POINTS) {
                let seriesBattVoltData = [];
                for (let i = 0; i < HISTORICAL_CHART_POINTS; i++) {
                    const timestamp = now - ((HISTORICAL_CHART_POINTS - 1 - i) * tenMinutesMillis);
                    seriesBattVoltData.push([timestamp, data.battVoltData[i]]);
                }
                historicalBattVoltSVGChartObject.updateFullData(seriesBattVoltData);
            }
            historicalTempSVGChartObject.updateFullData(seriesDataForChart);
        } else {
            console.warn("Received hist24h_10minAvgData but chart object or data is invalid/mismatched.");
            if(data.tempData) console.warn("Temp Data - Expected points: " + HISTORICAL_CHART_POINTS + ", Received: " + data.tempData.length);
        }
        // The fullDayData (1-min resolution) is not directly updated by this 10-min average message.
        // It's updated by "latestMinuteUpdate".
    } else if (data.historyDataType === "latestMinuteUpdate") { // This still handles 1-minute updates
        // console.log("Received latest minute update:", data.latestTemp);
        const latestDataIndexIn1440Buffer = data.latestDataIndex;

        // Update fullDayData (for potential other uses or if needed by a future 24h chart)
        let relativeMinuteIndex = (latestDataIndexIn1440Buffer - clientOverallHistoryStartIndex + HISTORICAL_DATA_POINTS) % HISTORICAL_DATA_POINTS;
        let hourForUpdate = Math.floor(relativeMinuteIndex / 60);
        let minuteInHourForUpdate = relativeMinuteIndex % 60;
        if (hourForUpdate >= 0 && hourForUpdate < 24 && fullDayData[hourForUpdate]) {
            if (data.latestTemp !== null) fullDayData[hourForUpdate].tempData[minuteInHourForUpdate] = data.latestTemp;
            if (data.latestHum  !== null) fullDayData[hourForUpdate].humData[minuteInHourForUpdate]  = data.latestHum;
            if (data.latestPres !== null) fullDayData[hourForUpdate].presData[minuteInHourForUpdate] = data.latestPres;
            if (data.latestRain !== null) fullDayData[hourForUpdate].rainData[minuteInHourForUpdate] = data.latestRain;
            if (data.latestWind !== null) fullDayData[hourForUpdate].windData[minuteInHourForUpdate] = data.latestWind;
        }
        // clientOverallHistoryStartIndex = (latestDataIndexIn1440Buffer + 1) % HISTORICAL_DATA_POINTS; // This might need re-evaluation if fullDayData isn't primary for charts

        // The 10-min average historical chart is NOT updated live by 1-minute data in this simplified version.
        // It will only update when the ESP32 sends a new "hist24h_10minAvgData" message
        // (e.g., on client reconnect, or if you implement a periodic resend from ESP32).
        // Mini-charts are updated by the live values below.
    } else { // This handles live data for dashboard elements and mini-chart source variables
        for (const key in data) {
            if (data.hasOwnProperty(key)) {
                const value = data[key];
                switch (key) {
                    // Handle System Configuration fields
                    case "ssid": updateElement('C_WIFI_SSID', value); break;
                    case "myChannelNumber": updateElement('C_TS_CH_ID', value); break;
                    case "thingSpkAPIwR": updateElement('C_TS_API_WR', value); break;
                    case "stationID": updateElement('C_WINDY_ST_ID', value); break;
                    case "windyAPIKey": updateElement('C_WINDY_API_KEY', value); break;
                    case "wundStationID": updateElement('C_WUND_ST_ID', value); break;
                    case "wundStationPw": updateElement('C_WUND_ST_PW', value); break;
                    case "PWSWxStationID": updateElement('C_PWS_ST_ID', value); break;
                    case "PWSWxStationPw": updateElement('C_PWS_ST_PW', value); break;

                    case "rssi": miniRssi = parseFloat(value) * -1; updateElement("rssi", value); break;
                    case "outsideTemp": currentTemperature = parseFloat(value); updateElement("outsideTemp", currentTemperature, 1); break;
                    case "sensorHum": currentHumidity = parseFloat(value); updateElement("sensorHum", currentHumidity, 1); break;
                    case "sensPress": currentPressure = parseFloat(value); updateElement("sensPress", currentPressure, 0); break;
                    case "windspeedKmH": currentWindSpeed = parseFloat(value); updateElement("windspeedKmH", currentWindSpeed, 1); break;
                    case "winddir": 
                        const windDirValue = parseFloat(value);
                        updateElement("winddir", windDirValue, 0);
                        const winDirArrow = document.getElementById("arrow-wrapper");
                        if (winDirArrow) { winDirArrow.style.transform = `translateX(-50%) rotate(${windDirValue}deg)`; }
                        break;
                    case "windDirCd": updateElement("windDirCd", value); break;
                    case "version": updateElement("version", value); updateElement("firmware", value); break; 
                    case "IPaddress": updateElement("IPaddress", value); break; 
                    case "loadVoltage": updateElement("loadVoltage", parseFloat(value), 2); break; 
                    case "currentmA": updateElement("currentmA", parseFloat(value), 0); break; 
                    case "power_mW": updateElement("power_mW", parseFloat(value), 0); break; 
                    case "timeStamp": updateElement("timeStamp", value); break; 
                    case "freeHeapMem": updateElement("freeHeapMem", value); break; 
                    case "memoryLFS": updateElement("memoryLFS", value); break; 
                    case "windyStatus": updateElement("windyStatus", value); break; 
                    case "wundStatus": updateElement("wundStatus", value); break; 
                    case "thingStatus": updateElement("thingStatus", value); break; 
                    case "rainHourMM": currentRainHour = parseFloat(value); updateElement("rainHourMM", currentRainHour, 1); break; 
                    case "dailyrainMM": updateElement("dailyrainMM", parseFloat(value), 1); break;
                    case "uptime":
                        const ms = parseInt(value, 10);
                        const s = Math.floor(ms / 1000);
                        const d = Math.floor(s / 86400);
                        const h = Math.floor((s % 86400) / 3600);
                        const m = Math.floor((s % 3600) / 60);
                        const sec = s % 60;
                        const uptimeString = `${d}d ${h}h ${m}m ${sec}s`;
                        updateElement("uptime", uptimeString);
                        break;
                    case "windgustKmH": updateElement("windgustKmH", parseFloat(value), 1); break; 
                    case "tempMin": updateElement("tempMin", parseFloat(value), 1); break; 
                    case "tempMax": updateElement("tempMax", parseFloat(value), 1); break; 
                    case "tempAvg": updateElement("tempAvg", parseFloat(value), 1); break; 
                    case "humidityMin": updateElement("humidityMin", parseFloat(value), 1); break; 
                    case "humidityMax": updateElement("humidityMax", parseFloat(value), 1); break; 
                    case "humidityAvg": updateElement("humidityAvg", parseFloat(value), 1); break; 
                    case "pressureMin": updateElement("pressureMin", parseFloat(value), 0); break; 
                    case "pressureMax": updateElement("pressureMax", parseFloat(value), 0); break; 
                    case "pressureAvg": updateElement("pressureAvg", parseFloat(value), 0); break; 
                    case "windMin": updateElement("windMin", parseFloat(value), 1); break; 
                    case "windMax": updateElement("windMax", parseFloat(value), 1); break; 
                    case "windAvg": updateElement("windAvg", parseFloat(value), 1); break; 
                    case "windGMin": updateElement("windGMin", parseFloat(value), 1); break; 
                    case "windGMax": updateElement("windGMax", parseFloat(value), 1); break; 
                    case "windGAvg": updateElement("windGAvg", parseFloat(value), 1); break; 
                    case "rainPHMin": updateElement("rainPHMin", parseFloat(value), 1); break; 
                    case "rainPHMax": updateElement("rainPHMax", parseFloat(value), 1); break; 
                    case "rainPHMAvg": updateElement("rainPHMAvg", parseFloat(value), 1); break; 
                    case "dataRefresh":    updateElement("dataRefresh", value); updateElement("Fdata0", value); break; 
                    case "rssilimit":      updateElement("rssilimit", value); updateElement("Fdata2", value); break; 
                    case "wifiinterval":   updateElement("wifiinterval", value); updateElement("Fdata3", value); break; 
                    case "windRainCutoff":updateElement("windRainCutoff", value); updateElement("Fdata4", value); break; 
                    case "deepSleepTime": updateElement("deepSleepTime", value); updateElement("Fdata5", value); break; 
                    case "terminal":
                        const textarea = document.getElementById("terminalBoxAreaID");
                        // This case now only handles terminal text output
                        if (textarea) {
                            textarea.value += value + "\n";
                            textarea.scrollTop = textarea.scrollHeight;
                        }
                        break;
                    default:
                        if (key.includes("toggle") && key !== "Dtoggle5") { 
                            const toggleElement = document.getElementById(key);
                            if (toggleElement && typeof toggleElement.checked !== 'undefined') {
                                toggleElement.checked = (value == "1" || value === true || value === 1);
                            }
                        } else if (document.getElementById(key)) {
                            updateElement(key, value);
                        }
                        break;
                }
            }
        }
    }
};

function initializeAllHistoricalSVGCharts() {
    historicalTempSVGChartObject = createHistoricalSVGChart(
        "historicalTempSVGChartContainer",
        () => new Array(HISTORICAL_CHART_POINTS).fill(null).map((_, i) => [Date.now() - (HISTORICAL_CHART_POINTS - 1 - i) * 10 * 60000, null]), // Initial empty 144 (10-min interval) points
        0,  // initialYMin (will be auto-adjusted)
        30, // initialYMax (will be auto-adjusted)
        "°C",
        "Temperature (24h)"
    );
    historicalHumSVGChartObject = createHistoricalSVGChart(
        "historicalHumSVGChartContainer",
        () => new Array(HISTORICAL_CHART_POINTS).fill(null).map((_, i) => [Date.now() - (HISTORICAL_CHART_POINTS - 1 - i) * 10 * 60000, null]),
        0,  // initialYMin
        100, // initialYMax
        "%rH",
        "Humidity (24h)"
    );
    historicalPresSVGChartObject = createHistoricalSVGChart(
        "historicalPresSVGChartContainer",
        () => new Array(HISTORICAL_CHART_POINTS).fill(null).map((_, i) => [Date.now() - (HISTORICAL_CHART_POINTS - 1 - i) * 10 * 60000, null]),
        950,  // initialYMin
        1050, // initialYMax
        "hPa",
        "Air Pressure (24h)"
    );
    historicalWindSVGChartObject = createHistoricalSVGChart(
        "historicalWindSVGChartContainer",
        () => new Array(HISTORICAL_CHART_POINTS).fill(null).map((_, i) => [Date.now() - (HISTORICAL_CHART_POINTS - 1 - i) * 10 * 60000, null]),
        0,  // initialYMin
        50, // initialYMax (adjust as needed for typical wind speeds)
        "km/h",
        "Wind Speed (24h)"
    );

     historicalRainSVGChartObject = createHistoricalSVGChart(
        "historicalRainVGChartContainer",
        () => new Array(HISTORICAL_CHART_POINTS).fill(null).map((_, i) => [Date.now() - (HISTORICAL_CHART_POINTS - 1 - i) * 10 * 60000, null]),
        0,  // initialYMin
        10, // initialYMax (adjust as needed for typical rain)
        "mm",
        "Rainfall (24h)"
    );

     historicalBattVoltSVGChartObject = createHistoricalSVGChart(
        "historicalBattVoltVGChartContainer",
        () => new Array(HISTORICAL_CHART_POINTS).fill(null).map((_, i) => [Date.now() - (HISTORICAL_CHART_POINTS - 1 - i) * 10 * 60000, null]),
        3.0,  // initialYMin
        4.2, // initialYMax (adjust as needed for battery voltage)
        "V",
        "Battery Voltage (24h)"
    );
}

function requestFullHistory() {
    if (websocket && websocket.readyState === WebSocket.OPEN) {
        receivedHourlyChunks = 0; 
        fullDayData = new Array(24).fill(null).map(EMPTY_HOUR_DATA); 
        websocket.send("requestFullHistory");
        console.log("Requested full historical data.");
    } else {
        console.log("WebSocket not open. Cannot request history.");
    }
}

function debounce(func, wait) {
    let timeout;
    return function executedFunction(...args) {
        const later = () => {
            clearTimeout(timeout);
            func(...args);
        };
        clearTimeout(timeout);
        timeout = setTimeout(later, wait);
    };
}

function handleGlobalChartResize() {
    activeChartsForResize.forEach(chartInstanceOrObject => {
        if (chartInstanceOrObject && typeof chartInstanceOrObject.redrawChartStructure === 'function') { 
            // This will call redrawChartStructure for both mini-charts and the new historical SVG chart
            // as they will share this method name.
            chartInstanceOrObject.redrawChartStructure(); 
        }
    });
}
window.addEventListener('resize', debounce(handleGlobalChartResize, 250));

function initializeAllMiniCharts() {
	createMiniChart( "tempChart", () => currentTemperature, 0, 40, "°C" );
	createMiniChart( "humidityChart", () => currentHumidity, 0, 100, "%" );
	createMiniChart( "pressureChart", () => currentPressure, 950, 1050, "hPa" );
	createMiniChart( "windSpeedChart", () => currentWindSpeed, 0, 30, "km/h" );
	createMiniChart( "rainChart", () => currentRainHour, 0, 10, "mm/hr", "column" );
}


function createHistoricalSVGChart(chartElementId, dataSourceGetter, initialYMin, initialYMax, unit = "", title = "") {
    const chartElement = document.getElementById(chartElementId);
    if (!chartElement) {
        console.error(`Historical SVG Chart element with ID ${chartElementId} not found.`);
        return null;
    }

    const dataPoints = HISTORICAL_CHART_POINTS; // Now 144 points
    let chartHeight = 300; // Default height, can be overridden by CSS
    let chartWidth = 600;  // Default width, can be overridden by CSS

    const yAxisLabelPadding = 45; // Padding on the left for Y-axis labels
    const xAxisLabelPadding = 40; // Padding at the bottom for X-axis labels
    const chartPaddingTop = 30;    // Space for title
    const chartPaddingRight = 15; // Padding on the right of the plot area
    // chartPaddingBottom is effectively handled by xAxisLabelPadding for the overall SVG bottom space

    let drawablePlotWidth, drawablePlotHeight, xScale; // Renamed for clarity
    let yMinVal = initialYMin;
    let yMaxVal = initialYMax;

    // This will store [timestamp, value] pairs
    let currentChartData = dataSourceGetter(); // dataSourceGetter now provides initial 144 [ts, null] pairs

    function mapNumber(value, in_min, in_max, out_min, out_max) {
        if (in_max === in_min) return out_min; // Avoid division by zero
        if (value === null || value === undefined) return out_max; // Plot nulls at the bottom or handle as needed
        return (value - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
    }

    function renderSVG() {
        const isDarkMode = document.body.classList.contains('dark-mode');
        const colors = {
            bg: isDarkMode ? '#2e3338' : '#ffffff',
            axisLines: isDarkMode ? '#adb5bd' : '#cccccc',
            textFill: isDarkMode ? '#f8f9fa' : '#333333',
            dataPathStroke: isDarkMode ? '#4dabf7' : '#007bff',
            titleFill: isDarkMode ? '#f8f9fa' : '#333333'
        };

        // Filter out null data for y-axis scaling, but keep nulls for plotting gaps
        const validDataValues = currentChartData.map(p => p[1]).filter(val => val !== null && !isNaN(parseFloat(val)));
        yMinVal = validDataValues.length > 0 ? Math.min(...validDataValues) : initialYMin;
        yMaxVal = validDataValues.length > 0 ? Math.max(...validDataValues) : initialYMax;
        
        // Ensure yMaxVal is always greater than yMinVal for scaling
        if (yMaxVal <= yMinVal) {
            yMaxVal = yMinVal + (initialYMax > initialYMin ? (initialYMax - initialYMin) * 0.1 : 1); // Add a small default range
        }
        // Add some padding to y-axis
        const yRange = yMaxVal - yMinVal;
        yMinVal -= yRange * 0.05;
        yMaxVal += yRange * 0.05;
        if (yMaxVal <= yMinVal) yMaxVal = yMinVal + 1; // Final fallback


        let out = `<svg xmlns="http://www.w3.org/2000/svg" version="1.1" font-family="Arial, Helvetica, sans-serif" width="${chartWidth}" height="${chartHeight}" style="background-color:${colors.bg};">`;
        
        // Title
        if (title) {
            out += `<text x="${chartWidth / 2}" y="${chartPaddingTop / 2 + 5}" font-size="16px" fill="${colors.titleFill}" text-anchor="middle" dominant-baseline="middle">${title}</text>`;
        }

        // Plot area translation
        out += `<g transform="translate(${yAxisLabelPadding}, ${chartPaddingTop})">`;

        // Y-Axis line and X-Axis line
        out += `<line x1="0" y1="0" x2="0" y2="${drawablePlotHeight}" style="stroke:${colors.axisLines};stroke-width:1" />`; // Y-axis line
        out += `<line x1="0" y1="${drawablePlotHeight}" x2="${drawablePlotWidth}" y2="${drawablePlotHeight}" style="stroke:${colors.axisLines};stroke-width:1" />`; // X-axis line

        // Y-Axis labels and ticks (example: 5 ticks)
        const numYTicks = 5;
        for (let i = 0; i <= numYTicks; i++) {
            const val = yMinVal + (i * (yMaxVal - yMinVal) / numYTicks);
            const yPos = drawablePlotHeight - mapNumber(val, yMinVal, yMaxVal, 0, drawablePlotHeight);
            out += `<text x="-5" y="${yPos}" font-size="10px" fill="${colors.textFill}" text-anchor="end" dominant-baseline="middle">${val.toFixed(1)}</text>`;
            out += `<line x1="0" y1="${yPos}" x2="-3" y2="${yPos}" style="stroke:${colors.axisLines};stroke-width:1" />`;
        }
        if (unit) {
             out += `<text x="${-(yAxisLabelPadding*0.65)}" y="${drawablePlotHeight / 2}" font-size="12px" fill="${colors.textFill}" text-anchor="middle" dominant-baseline="central" transform="rotate(-90, ${-(yAxisLabelPadding*0.65)}, ${drawablePlotHeight / 2})">${unit}</text>`;
        }


        // X-Axis labels and ticks (example: every 4 hours for 24 hours = 6 labels + start)
        // For a 24-hour chart, 6 ticks (every 4 hours) is reasonable.
        const numXTicks = 6; 
        for (let i = 0; i <= numXTicks; i++) {
            const xPos = i * (drawablePlotWidth / numXTicks);
            const timeRatio = i / numXTicks;
            let tickTimestamp = Date.now(); // Fallback
            if (currentChartData && currentChartData.length === dataPoints && currentChartData[0] && currentChartData[dataPoints-1]) {
                 tickTimestamp = currentChartData[0][0] + timeRatio * (currentChartData[dataPoints-1][0] - currentChartData[0][0]); // Timestamps are now 10 mins apart
            }
            
            const tickDate = new Date(tickTimestamp); // Ensure tickTimestamp is valid
            const timeLabel = `${String(tickDate.getHours()).padStart(2, '0')}:${String(tickDate.getMinutes()).padStart(2, '0')}`;
            out += `<text x="${xPos}" y="${drawablePlotHeight + 20}" font-size="10px" fill="${colors.textFill}" text-anchor="middle">${timeLabel}</text>`; // Positioned 20px below x-axis line
            out += `<line x1="${xPos}" y1="${drawablePlotHeight}" x2="${xPos}" y2="${drawablePlotHeight + 4}" style="stroke:${colors.axisLines};stroke-width:1" />`;
        }

        // Data path
        let pathData = "";
        let firstPoint = true;
        for (let i = 0; i < dataPoints; i++) {
            if (currentChartData && currentChartData[i] && currentChartData[i][1] !== null) {
                let xPos = i * xScale; // xScale is (drawablePlotWidth / (dataPoints - 1))
                let yPos = drawablePlotHeight - mapNumber(currentChartData[i][1], yMinVal, yMaxVal, 0, drawablePlotHeight);
                if (firstPoint) {
                    pathData += `M ${xPos.toFixed(2)} ${yPos.toFixed(2)}`;
                    firstPoint = false;
                } else {
                    pathData += ` L ${xPos.toFixed(2)} ${yPos.toFixed(2)}`;
                }
            } else {
                firstPoint = true; // Start new path segment after a null
            }
        }
        if (pathData) {
            out += `<path d="${pathData}" style="fill:none;stroke:${colors.dataPathStroke};stroke-width:1.5" />`;
        }

        out += `</g>`; // End plot area transform
        out += `</svg>`;
        chartElement.innerHTML = out;
    }

    function redrawChartStructure() {
        chartWidth = chartElement.clientWidth || 600;
        chartHeight = chartElement.clientHeight || 300;
        if (chartWidth < 100) chartWidth = 100;
        if (chartHeight < 100) chartHeight = 100;

        drawablePlotWidth = chartWidth - yAxisLabelPadding - chartPaddingRight;
        drawablePlotHeight = chartHeight - chartPaddingTop - xAxisLabelPadding;
        
        xScale = drawablePlotWidth / (dataPoints > 1 ? dataPoints - 1 : 1);
        renderSVG();
    }

    function updateFullData(newDataArray) {
        if (newDataArray && newDataArray.length === dataPoints) {
            currentChartData = newDataArray;
        } else {
            console.warn("updateFullData received invalid data or length mismatch. Expected " + dataPoints + " points.");
            // Fallback to empty data if needed
            currentChartData = new Array(dataPoints).fill(null).map((_, i) => [Date.now() - (dataPoints - 1 - i) * 60000, null]);
        }
        redrawChartStructure(); 
    }

    // addLatestPoint is removed as this chart will update with full 10-min average datasets
    // function addLatestPoint(timestamp, value) { ... } 

    const chartObject = {
        redrawChartStructure: redrawChartStructure,
        updateFullData: updateFullData, // Method to update with a full new dataset
        // addLatestPoint: addLatestPoint, // Removed
        isMiniChart: false, // Differentiate from mini charts
        isHistoricalSVG: true
    };

    activeChartsForResize.push(chartObject);
    redrawChartStructure(); // Initial draw
    return chartObject;
}


function createMiniChart(chartElementId, dataSourceGetter, initialYMin, initialYMax, unit = "", chartType = "line") { 
	const chartElement = document.getElementById(chartElementId);
	if (!chartElement) {
		console.error(`Chart element with ID ${chartElementId} not found.`);
		return null;
	}

	const dataPoints = 300; 
	let chartHeight = 100; 
	let chartWidth = 150;  
	const yAxisLabelPadding = 12; 
	const chartPadding = 5;       
	let plotAreaWidth, plotAreaHeight, yMaxGraphHeight, xScale;
	let yMinVal = initialYMin;
	let yMaxVal = initialYMax;
	const sensorValueArray = new Array(dataPoints).fill(initialYMin);
	const Y1Array = new Array(dataPoints).fill(mapNumber(initialYMin, initialYMin, initialYMax, 0, 100 - (2 * chartPadding) )); 

	function mapNumber(value, in_min, in_max, out_min, out_max) {
		if (in_max === in_min) return out_min; 
		return (value - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
	}

	function miniChartSVG() {
		const isDarkMode = document.body.classList.contains('dark-mode');
		const colors = {
			yAxisAreaBg: 'transparent', plotAreaBg: 'transparent',  
			axisLines: isDarkMode ? '#adb5bd' : '#000000',   
			textFill: isDarkMode ? '#f8f9fa' : '#000000',    
			dataPathStroke: isDarkMode ? '#4dabf7' : '#007bff', 
			currentValueFill: isDarkMode ? '#4dabf7' : 'blue'   
		};
		let out = `<svg xmlns="http://www.w3.org/2000/svg" version="1.1" font-family="Arial, Helvetica, sans-serif" width="${chartWidth}" height="${chartHeight}">`;
		out += `<rect x="0" y="0" width="${yAxisLabelPadding}" height="${chartHeight}" fill="${colors.yAxisAreaBg}" />`; 
		out += `<rect x="${yAxisLabelPadding}" y="0" width="${plotAreaWidth}" height="${plotAreaHeight}" fill="${colors.plotAreaBg}" />`; 
		out += `<g transform="translate(${yAxisLabelPadding}, 0)">`;
		out += `<line x1="0" y1="0" x2="0" y2="${plotAreaHeight}" style="stroke:${colors.axisLines};stroke-width:1" />`; 
		out += `<line x1="0" y1="${plotAreaHeight - chartPadding}" x2="${plotAreaWidth}" y2="${plotAreaHeight - chartPadding}" style="stroke:${colors.axisLines};stroke-width:1" />`; 
		out += `<text x="2" y="${chartPadding + 3}" font-size="10px" fill="${colors.textFill}" dominant-baseline="hanging">${yMaxVal.toFixed(1)}</text>`;
		out += `<text x="2" y="${chartHeight - chartPadding}" font-size="10px" fill="${colors.textFill}" dominant-baseline="alphabetic">${yMinVal.toFixed(1)}</text>`;
		if (unit) {
			out += `<text x="8" y="${chartHeight / 2}" font-size="10px" fill="${colors.textFill}" text-anchor="middle" dominant-baseline="central" transform="rotate(-90, 8, ${chartHeight / 2})">${unit}</text>`;
		}
		const numTicks = 5;
		for (let i = 0; i < numTicks; i++) {
			const tickY = chartPadding + (i * (yMaxGraphHeight / (numTicks -1)));
			out += `<line x1="0" y1="${tickY}" x2="-4" y2="${tickY}" style="stroke:${colors.axisLines};stroke-width:1" />`;
		}
		let lastSensorVal = sensorValueArray[dataPoints - 1];
		let lastMappedY = plotAreaHeight - chartPadding - Y1Array[dataPoints - 1];
		lastMappedY = Math.max(chartPadding + 10, Math.min(plotAreaHeight - chartPadding - 2, lastMappedY)); 
		out += `<text x="${plotAreaWidth - 3}" y="${lastMappedY}" font-size="10px" fill="${colors.currentValueFill}" text-anchor="end" dominant-baseline="alphabetic">${lastSensorVal.toFixed(1)}</text>`;

		if (chartType === "line") {
			let pathData = `M 0 ${plotAreaHeight - chartPadding - Y1Array[0]}`; 
			for (let i = 1; i < dataPoints; i++) {
				pathData += ` L ${(i * xScale).toFixed(2)} ${(plotAreaHeight - chartPadding - Y1Array[i]).toFixed(2)}`;
			}
			out += `<path d="${pathData}" style="fill:none;stroke:${colors.dataPathStroke};stroke-width:1.5" />`;
		} else if (chartType === "column") {
			const barWidth = Math.max(1, xScale * 0.8); 
			const barPadding = xScale - barWidth;
			for (let i = 0; i < dataPoints; i++) {
				const barHeight = Y1Array[i]; 
				if (barHeight > 0) { 
					out += `<rect x="${(i * xScale + barPadding / 2).toFixed(2)}" y="${(plotAreaHeight - chartPadding - barHeight).toFixed(2)}" width="${barWidth.toFixed(2)}" height="${barHeight.toFixed(2)}" fill="${colors.dataPathStroke}" />`;
				}
			}
		}
		out += `</g></svg>`;
		chartElement.innerHTML = out;
	}

	function redrawChartStructure() {
		chartWidth = chartElement.clientWidth || 150; 
		chartHeight = chartElement.clientHeight || 100; 
		if (chartWidth < 50) chartWidth = 50;
		if (chartHeight < 30) chartHeight = 30;
		plotAreaWidth = chartWidth - yAxisLabelPadding;
		plotAreaHeight = chartHeight; 
		yMaxGraphHeight = chartHeight - (2 * chartPadding); 
		xScale = plotAreaWidth / (dataPoints - 1);
		for (let i = 0; i < dataPoints; i++) {
			Y1Array[i] = mapNumber(sensorValueArray[i], yMinVal, yMaxVal, 0, yMaxGraphHeight);
		}
		miniChartSVG(); 
	}

	function updateChart() {
		let currentValue = dataSourceGetter();
		if (typeof currentValue !== 'number' || isNaN(currentValue)) {
			currentValue = (yMinVal + yMaxVal) / 2; 
		}
		sensorValueArray.shift();
		sensorValueArray.push(currentValue);
		let currentDataMin = Math.min(...sensorValueArray);
		let currentDataMax = Math.max(...sensorValueArray);
		const range = currentDataMax - currentDataMin;
		const padding = Math.max(range * 0.1, (initialYMax - initialYMin) * 0.05, 0.1);
		yMinVal = currentDataMin - padding;
		yMaxVal = currentDataMax + padding;
		if (yMaxVal <= yMinVal) {
			yMaxVal = yMinVal + Math.max((initialYMax - initialYMin) * 0.1, 1);
		}
		for (let i = 0; i < dataPoints; i++) {
			Y1Array[i] = mapNumber(sensorValueArray[i], yMinVal, yMaxVal, 0, yMaxGraphHeight);
		}
		miniChartSVG();
	}
    const chartObject = { 
        redrawChartStructure: redrawChartStructure, 
        isMiniChart: true 
    }; // Removed isHistoricalSVG from mini-chart object
	activeChartsForResize.push(chartObject);
	redrawChartStructure(); 
	setInterval(updateChart, 1000);
    return chartObject;
}

document.addEventListener('DOMContentLoaded', () => {
    let allCheckboxes = document.querySelectorAll('input[type=checkbox]');
    allCheckboxes.forEach(checkbox => checkbox.addEventListener("change", updateToggle));

    let allTimeInputs = document.querySelectorAll('input[type=time]');
    allTimeInputs.forEach(input => input.addEventListener("change", updateTimeInput));

    let allNumberInputs = document.querySelectorAll('input[type=number]');
    allNumberInputs.forEach(input => input.addEventListener("change", sendForm));

    let allDropDownInputs = document.querySelectorAll('select'); 
    allDropDownInputs.forEach(select => select.addEventListener("change", getDropdown));
});

// --- Event listener attachment functions ---
// These functions are called from the HTML onchange attributes.
// It's generally better to attach listeners via JavaScript (as done above in DOMContentLoaded),
// but if you keep onchange in HTML, these need to be globally accessible.

function updateToggle(element) { // 'element' here is the HTML element itself
	// When called via addEventListener, 'element' is the Event object.
	// The actual checkbox element is event.target.
	const targetCheckbox = element.target; 
	const id = targetCheckbox.id;
	const isChecked = targetCheckbox.checked;
	const idLength = id.length;
	const togType = id.charAt(0);
	let toggNumber;

	if (idLength == 8) { toggNumber = id.substring(7); } 
    else if (idLength == 9) { toggNumber = id.substring(7); } 
    else { console.error("Unexpected toggle ID format:", id); toggNumber = id.substr(idLength - (idLength - "toggle".length - 1));}

	console.log("Toggle Event: " + id + ", Checked: " + isChecked + ", Type: " + togType + ", Number: " + toggNumber);

	if (toggNumber == "8" && togType == "D") {
		if (isChecked) {
			if (logRefreshIntervalId) clearInterval(logRefreshIntervalId);
			loadErrorLogTable(); 
			logRefreshIntervalId = setInterval(loadErrorLogTable, 15000);
		} else {
			if (logRefreshIntervalId) clearInterval(logRefreshIntervalId);
			logRefreshIntervalId = null;
		}
	}
	if (websocket && websocket.readyState === WebSocket.OPEN) {
		const message = toggNumber + togType + (isChecked ? "1" : "0");
		websocket.send(message);
	}
}

function sendForm(event) {
    const inputElement = event.target;
    const id = inputElement.id;
    const value = inputElement.value;
    const idLength = id.length;
    const formType = id.charAt(0);
    let formNumber;

    if (idLength == 6) { formNumber = id.substr(idLength - 1, 1); } 
    else if (idLength == 7) { formNumber = id.substr(idLength - 2, 2); } 
    else if (idLength == 8) { formNumber = id.substr(idLength - 3, 3); } 
    else { console.error("Unexpected number input ID format:", id); return; }

    console.log("Form Event: " + id + ", Value: " + value);
    if (websocket && websocket.readyState === WebSocket.OPEN) {
        const message = formNumber + formType + value;
        websocket.send(message);
    }
}

function updateTimeInput(element) {
    // This function might be redundant if covered by sendForm or needs specific logic
    console.log("Time Input Changed (not yet sending):", element.id, element.value);
    // If time inputs should also use sendForm logic, ensure their IDs match the pattern
    // or create a specific sender for them.
}

function getDropdown(element) {
    // This function might be redundant if covered by sendForm or needs specific logic
    console.log("Dropdown Changed (not yet sending):", element.id, element.value);
}

function loadErrorLogTable() {
    fetch('/systemLog.csv')
        .then(response => {
            if (!response.ok) {
                throw new Error('Network response was not ok ' + response.statusText);
            }
            return response.text();
        })
        .then(data => {
            const rows = data.trim().split('\n');
            const table = document.getElementById('errorTable');
            if (!table) return;

            // Clear existing rows except the header
            while (table.rows.length > 1) {
                table.deleteRow(1);
            }
            if (table.rows.length === 0) { // Add header if table is completely empty
                 const header = table.insertRow();
                 const dateHeader = header.insertCell(); dateHeader.textContent = 'Date'; dateHeader.style.fontWeight = 'bold';
                 const levelHeader = header.insertCell(); levelHeader.textContent = 'Level'; levelHeader.style.fontWeight = 'bold';
                 const msgHeader = header.insertCell(); msgHeader.textContent = 'Message'; msgHeader.style.fontWeight = 'bold';
            }


            // Skip header row from CSV (index 0) if present
            const dataRows = rows.slice(1); 
            const reversedRows = dataRows.reverse(); // Show newest first

            reversedRows.forEach(rowString => {
                const columns = rowString.split(',');
                if (columns.length >= 3) { // Ensure there are enough columns
                    const row = table.insertRow();
                    const dateCell = row.insertCell();
                    const levelCell = row.insertCell();
                    const messageCell = row.insertCell();

                    dateCell.textContent = columns[0]; // Date & Time
                    levelCell.textContent = columns[1]; // Level
                    messageCell.textContent = columns.slice(2).join(','); // Message (can contain commas)
                }
            });
        })
        .catch(error => {
            console.error('Error loading or parsing error log:', error);
            const table = document.getElementById('errorTable');
            if (table) {
                const row = table.insertRow();
                const cell = row.insertCell();
                cell.colSpan = 3; // Assuming 3 columns
                cell.textContent = 'Error loading log data.';
            }
        });
}
