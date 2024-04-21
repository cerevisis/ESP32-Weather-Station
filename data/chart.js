//API URL documentation here https://www.mathworks.com/help/thingspeak/readdata.html
//LIVE
let channelID = "1657561"; //insert your own Thingspeak channel key here

//TEST
//let channelID = "987467";

let dataAvg = "0" //0 minutes. Set to 0 to disable or 10, 15, 20, 30, 60, 240, 720, 1440, "daily".

let dataDays = '7'; //5 days up to max of 8000 points

let apiKey = ""; //Insert your ThingSpeak READ API key here. If you don't have one, access your channel logged in, navigate to API Keys tab and click "Add new read API key"
console.log("date");

 
function formatTSDate(offset) {
  const date = new Date();
  var day = date.getTime() + (offset * 24 * 60 * 60 * 1000);
  date.setTime(day);
  console.log("new date:" + date.toISOString());

  let startDate = date.toISOString();
  console.log("startDate:" + startDate);
  let newStartDate = startDate.replace("T", "%20");
  newStartDate = newStartDate.replace("Z", "");
  let finaldate = newStartDate.split(".");

  console.log("finaldate:" + finaldate[0]);
  return finaldate[0];
}

console.log(formatTSDate(-14));

let requestURL = 'https://api.thingspeak.com/channels/' + channelID + '/feeds.json?days=' + dataDays + '&average=' + dataAvg + '&start=' + formatTSDate(-28) + '&end=' + formatTSDate(0);
console.log("requestURL " + requestURL);

//Public channel
Highcharts.getJSON(requestURL, success);

//Private channel with API key
//Highcharts.getJSON(requestURL + '&api_key=' + apiKey, success);


function success(data) {
  console.log("[INFO] Initial data: ");
  console.log(data);
  populatechart(data);
}
var initialData = [{}];
var series1Data = [{}];
var series2Data = [{}];
var series3Data = [{}];
var series4Data = [{}];
var series5Data = [{}];
var series6Data = [{}];
var series7Data = [{}];
var series8Data = [{}];



function populatechart(wxData) {

  var wxValues = wxData.feeds;


  for (let i = 0; i < wxValues.length; i++) {
    initialData[i] = [];
    initialData[i][0] = wxValues[i].created_at;
    initialData[i][1] = parseFloat(wxValues[i].field1);
    initialData[i][2] = parseFloat(wxValues[i].field2);
    initialData[i][3] = parseFloat(wxValues[i].field3);
    initialData[i][4] = parseFloat(wxValues[i].field4);
    initialData[i][5] = parseFloat(wxValues[i].field5);
    initialData[i][6] = parseFloat(wxValues[i].field6);
    initialData[i][7] = parseFloat(wxValues[i].field7);
    initialData[i][8] = parseFloat(wxValues[i].field8);
  }


  console.log("[INFO] Parsing UNIX date");
  initialData.forEach(function (point) {
    point[0] = Date.parse(point[0])
  });
  console.log(initialData);

  for (let i = 0; i < wxValues.length; i++) {
    series1Data[i] = [];
    series1Data[i][0] = initialData[i][0];

    series2Data[i] = [];
    series2Data[i][0] = initialData[i][0];

    series3Data[i] = [];
    series3Data[i][0] = initialData[i][0];

    series4Data[i] = [];
    series4Data[i][0] = initialData[i][0];

    series5Data[i] = [];
    series5Data[i][0] = initialData[i][0];

    series6Data[i] = [];
    series6Data[i][0] = initialData[i][0];

    series7Data[i] = [];
    series7Data[i][0] = initialData[i][0];

    series8Data[i] = [];
    series8Data[i][0] = initialData[i][0];

  }

  for (let i = 0; i < wxValues.length; i++) {
    //series1Data[i] = [];
    series1Data[i][1] = initialData[i][1];
    series2Data[i][1] = initialData[i][2];
    series3Data[i][1] = initialData[i][3];
    series4Data[i][1] = initialData[i][4];
    series5Data[i][1] = initialData[i][5];
    series6Data[i][1] = initialData[i][6];
    series7Data[i][1] = initialData[i][7];
    series8Data[i][1] = initialData[i][8];
  }

  console.log("[INFO] Date parsed series1Data: ");
  console.log(series1Data);


  createChart1();
  createChart2();
  createChart3();
}

Highcharts.setOptions({
  enablePolling: true,
  dataRefreshRate: 15000,
  rangeSelector: {

    buttons: [{
      type: 'day',
      count: 1,
      text: '1d'
    }, {
      type: 'day',
      count: 3,
      text: '3d'
    }, {
      type: 'week',
      count: 1,
      text: '1w'
    }, {
      type: 'month',
      count: 1,
      text: '1m'
    }, {
      type: 'month',
      count: 6,
      text: '6m'
    }, {
      type: 'year',
      count: 1,
      text: '1y'
    }, {
      type: 'all',
      text: 'All'
    }],
    selected: 3
  },
  chart: {
    displayErrors: false
  }

});


function createChart1() {
  const chart = Highcharts.stockChart('chart-container1', {

    title: {
      text: 'Weather History'
    },

    xAxis: {
      type: 'datetime'

    },

    yAxis: [{
      opposite: false
    }, {
      opposite: true
    }, {
      opposite: false
    }],

    tooltip: {
      shared: true
    },
    legend: {
      enabled: true
    },
    series: [{
      name: 'Temperature',
      yAxis: 0,
      labels: {
        format: '{value}°C',
      },
      data: series1Data
    }, {
      name: 'Dew point',
      yAxis: 0,
      labels: {
        format: '{value}°C',
      },
      data: series4Data
    }, {
      name: 'Pressure',
      yAxis: 1,
      labels: {
        format: '{value} hPa',
      },
      data: series2Data,

    }, {
      name: 'Humidity',
      yAxis: 2,
      labels: {
        format: '{value} %rH',
      },
      data: series3Data,

    }]
  });
}


function createChart2() {
  const chart = Highcharts.stockChart('chart-container2', {
    title: {
      text: 'Wind & Rain'
    },

    xAxis: {
      type: 'datetime'

    },

    yAxis: [{
      opposite: false
    }, {
      opposite: true
    }, {
      opposite: false
    }],

    tooltip: {
      shared: true
    },
    legend: {
      enabled: true
    },
    series: [{
      name: 'Wind speed',
      yAxis: 0,
      labels: {
        format: '{value} km/h',
      },
      lineWidth: 0,
      marker: {
        enabled: true,
        radius: 2
      },
      data: series6Data
    }, {
      name: 'Wind dir',
      yAxis: 1,
      labels: {
        format: '{value}°',
      },
      data: series5Data
    }, {
      name: 'Rainfall',
      yAxis: 2,
      labels: {
        format: '{value} mm',
      },
      data: series8Data,
      type: 'area',
      threshold: null,
      tooltip: {
        valueDecimals: 2
      },
      fillColor: {
        linearGradient: {
          x1: 0,
          y1: 0,
          x2: 0,
          y2: 1
        },
        stops: [
          [0, Highcharts.getOptions().colors[0]],
          [1, Highcharts.color(Highcharts.getOptions().colors[0]).setOpacity(0).get('rgba')]
        ]
      },
      color: 'blue'
    }]
  });
}


function createChart3() {
  const chart = Highcharts.stockChart('chart-container3', {
    title: {
      text: 'Battery'
    },

    xAxis: {
      type: 'datetime'

    },

    yAxis: [{
      opposite: false
    }],

    tooltip: {
      shared: true
    },
    legend: {
      enabled: true
    },
    series: [{
      name: 'Voltage',
      yAxis: 0,
      labels: {
        format: '{value} V',
      },
      data: series7Data
    }]
  });
}