Pebble.addEventListener('showConfiguration', function(e) {
  // Show config page
  Pebble.openURL('http://misc.myepg.us/fitbit/'+Pebble.getAccountToken());
});

var xhrRequest = function (url, type, callback) {
  var xhr = new XMLHttpRequest();
  xhr.onload = function () {
    callback(this.responseText);
  };
  xhr.open(type, url);
  xhr.send();
};
var steps = 0;
var distance = '0 miles';
var nextCheckProgress = 0.0;

function locationSuccess(pos) {
  // Construct URL
  var url = 'http://api.openweathermap.org/data/2.5/weather?lat=' +
      pos.coords.latitude + '&lon=' + pos.coords.longitude;

  // Send request to OpenWeatherMap
  xhrRequest(url, 'GET', 
    function(responseText) {
      // responseText contains a JSON object with weather info
      var json = JSON.parse(responseText);

      // Temperature in Kelvin requires adjustment
      var temperature = Math.round(((json.main.temp - 273.15) * (9.0 / 5.0)) + 32);
      console.log('Temperature is ' + temperature);

      // Conditions
      var conditions = json.weather[0].main;      
      console.log('Conditions are ' + conditions);
      
      localStorage.setItem('lastTemperature', temperature + '');
      localStorage.setItem('lastConditions', conditions + '');
      localStorage.setItem('lastWeatherCheck', Date.now() + '');
      
      // Assemble dictionary using our keys
      var dictionary = {
        'KEY_TEMPERATURE': temperature,
        'KEY_CONDITIONS': conditions,
        'KEY_STEPS': steps,
        'KEY_DISTANCE': distance,
        'KEY_CHECK_PROGRESS': nextCheckProgress
      };
      
      // Send to Pebble
      Pebble.sendAppMessage(dictionary,
        function(e) {
          console.log('Weather info sent to Pebble successfully!');
        },
        function(e) {
          console.log('Error sending weather info to Pebble!');
        }
      );
    }      
  );
}

function locationError(err) {
  console.log('Error requesting location!');
}

function getWeather() {
  var lastCheckString = localStorage.getItem('lastWeatherCheck');
  if(!lastCheckString || (Date.now() - parseInt(lastCheckString)) > (15 * 60 * 1000)) {
    navigator.geolocation.getCurrentPosition(
      locationSuccess,
      locationError,
      {timeout: 15000, maximumAge: 60000}
    );
  } else {
    var temperature = parseInt(localStorage.getItem('lastTemperature'));
    var conditions = localStorage.getItem('lastConditions');
    
    // Assemble dictionary using our keys
    var dictionary = {
      'KEY_TEMPERATURE': temperature,
      'KEY_CONDITIONS': conditions,
      'KEY_STEPS': steps,
      'KEY_DISTANCE': distance,
      'KEY_CHECK_PROGRESS': nextCheckProgress
    };

    // Send to Pebble
    Pebble.sendAppMessage(dictionary,
      function(e) {
        console.log('Weather info sent to Pebble successfully!');
      },
      function(e) {
        console.log('Error sending weather info to Pebble!');
      }
    );
  }
}

function getFitbitInfo(battery) {
  xhrRequest('http://misc.myepg.us/fitbit/'+Pebble.getAccountToken()+'/data?battery='+battery, 'GET', function(responseText) {
    var json = JSON.parse(responseText);
    steps = json.steps;
    if(steps.match(/^\d+$/) === null) {
      steps = '?';
    }
    distance = json.distance;
    if(json.progress.match(/^[\d.]+$/) !== null) {
      nextCheckProgress = Math.round(144 * parseFloat(json.progress));
    }
    getWeather();
  });
}

// Listen for when the watchface is opened
Pebble.addEventListener('ready', 
  function(e) {
    console.log('PebbleKit JS ready!');

    // Get the initial weather
    getFitbitInfo('?');
  }
);

// Listen for when an AppMessage is received
Pebble.addEventListener('appmessage',
  function(e) {
    console.log('AppMessage received!');
    getFitbitInfo(e.payload['KEY_BATTERY']);
  }                     
);