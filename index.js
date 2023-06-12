// Include the Alexa SDK v2
const Alexa = require('ask-sdk-core');
const AWS = require('aws-sdk');
const IoTData = new AWS.IotData({endpoint: 'a3rd2nwokwd1rw-ats.iot.us-east-2.amazonaws.com'});

const TurnOnParams = {
  topic: '$aws/things/mqtt_basico/shadow/update',
  payload: '{"state": {"desired": {"incubatorState": "on"}}}',
  qos: 0
};

const TurnOffParams = {
  topic: '$aws/things/mqtt_basico/shadow/update',
  payload: '{"state": {"desired": {"incubatorState": "off"}}}',
  qos: 0
};

const ShadowParams = {
  thingName: 'mqtt_basico',
};

function getShadowPromise(params) {
  return new Promise((resolve, reject) => {
    IoTData.getThingShadow(params, (err, data) => {
      if (err) {
        console.log(err, err.stack);
        reject(`Failed to get thing shadow: ${err.errorMessage}`);
      } else {
        resolve(JSON.parse(data.payload));
      }
    });
  });
}

// The "LaunchRequest" intent handler - called when the skill is launched
const LaunchRequestHandler = {
  canHandle(handlerInput) {
    return handlerInput.requestEnvelope.request.type === "LaunchRequest";
  },
  handle(handlerInput) {
    const speechText = "Hola, Conmigo puedes consultar el estado de tu empolladora";

    // Speak out the speechText via Alexa
    return handlerInput.responseBuilder
    .speak(speechText)
    .reprompt(speechText)
    .getResponse();
  }
};

const QueryStateIntentHandler = {
  canHandle(handlerInput) {
    return Alexa.getRequestType(handlerInput.requestEnvelope) === 'IntentRequest'
      && Alexa.getIntentName(handlerInput.requestEnvelope) === 'QueryStateIntent';
  },
  
  async handle(handlerInput) {
    var incubatorState = 'Unknown';
    await getShadowPromise(ShadowParams)
      .then((result) => incubatorState = result.state.reported.incubatorState);
    console.log(incubatorState);
    
    var speechText = 'Error;'
    if (incubatorState == 'on') {
      speechText = 'La incubadora esta encendida';
    } else if (incubatorState == 'off') {
      speechText = 'La incubadora esta apagada';
    } else {
      speechText = 'No conozco el estado de la incubadora, intentelo más tarde';
    }
    return handlerInput.responseBuilder
      .speak(speechText)
      .reprompt(speechText)
      .getResponse();
  }
};

const QueryTemperatureIntentHandler = {
  canHandle(handlerInput) {
    return Alexa.getRequestType(handlerInput.requestEnvelope) === 'IntentRequest'
      && Alexa.getIntentName(handlerInput.requestEnvelope) === 'QueryTemperatureIntent';
  },
  
  async handle(handlerInput) {
    var temperature = 'Unknown';
    await getShadowPromise(ShadowParams)
      .then((result) => temperature = result.state.reported.temperature);
    console.log(temperature);
    
    var speechText = 'Error;'
    if (temperature != '') {
      speechText = 'La temperatura es de '+ temperature + ' Grados Celsius';
    } 
    else {
      speechText = 'No conozco la temperatura, inténtelo más tarde';
    }
    return handlerInput.responseBuilder
      .speak(speechText)
      .reprompt(speechText)
      .getResponse();
  }
};

const QueryHumidityIntentHandler = {
  canHandle(handlerInput) {
    return Alexa.getRequestType(handlerInput.requestEnvelope) === 'IntentRequest'
      && Alexa.getIntentName(handlerInput.requestEnvelope) === 'QueryHumidityIntent';
  },
  
  async handle(handlerInput) {
    var humidity = 'Unknown';
    await getShadowPromise(ShadowParams)
      .then((result) => humidity = result.state.reported.humidity);
    console.log(humidity);
    
    var speechText = 'Error;'
    if (humidity != '') {
      speechText = 'La humedad es de '+ humidity;
    } 
    else {
      speechText = 'No conozco la humedad, inténtelo más tarde';
    }
    return handlerInput.responseBuilder
      .speak(speechText)
      .reprompt(speechText)
      .getResponse();
  }
};

const TurnOnIntentHandler = {
  canHandle(handlerInput) {
  return Alexa.getRequestType(handlerInput.requestEnvelope) === 'IntentRequest'
    && Alexa.getIntentName(handlerInput.requestEnvelope) === 'TurnOnIntent';
  },
  handle(handlerInput) {
    var speechText = 'Error';
    IoTData.publish(TurnOnParams, function(err, data) {
        if (err) {
          console.log(err);
        }
      });
    speechText = "Encendiendo";
    return handlerInput.responseBuilder
      .speak(speechText)
      .reprompt(speechText)
      .getResponse();
  }
};


const TurnOffIntentHandler = {
  canHandle(handlerInput) {
    return Alexa.getRequestType(handlerInput.requestEnvelope) === 'IntentRequest'
      && Alexa.getIntentName(handlerInput.requestEnvelope) === 'TurnOffIntent';
  },
  
  handle(handlerInput) {
    var speechText = 'Error';
    IoTData.publish(TurnOffParams, function(err, data) {
        if (err) {
          console.log(err);
        }
      });
    speechText = "Apagando";
    return handlerInput.responseBuilder
      .speak(speechText)
      .reprompt(speechText)
      .getResponse();
  }
};

const HelpIntentHandler = {
  canHandle(handlerInput) {
    return Alexa.getRequestType(handlerInput.requestEnvelope) === 'IntentRequest'
      && Alexa.getIntentName(handlerInput.requestEnvelope) === 'HelpIntent';
  },
  
  handle(handlerInput) {
    const speechText = "Tienes las opciones de consultar o cambiar el estado del led interno de tu objeto inteligente";
    return handlerInput.responseBuilder
      .speak(speechText)
      .reprompt(speechText)
      .getResponse();
  }
};

const CancelAndStopIntentHandler = {
  canHandle(handlerInput) {
    return Alexa.getRequestType(handlerInput.requestEnvelope) === 'IntentRequest'
      && (Alexa.getIntentName(handlerInput.requestEnvelope) === 'AMAZON.CancelIntent'
        || Alexa.getIntentName(handlerInput.requestEnvelope) === 'AMAZON.StopIntent');
  },
  handle(handlerInput) {
    const speechText = 'Adios!';

    return handlerInput.responseBuilder
      .speak(speechText)
      .withSimpleCard('Adios!', speechText)
      .withShouldEndSession(true)
      .getResponse();
  }
};

// Register the handlers and make them ready for use in Lambda
exports.handler = Alexa.SkillBuilders.custom()
  .addRequestHandlers(
    LaunchRequestHandler,
    QueryStateIntentHandler, 
    TurnOnIntentHandler,
    TurnOffIntentHandler,
    HelpIntentHandler,
    CancelAndStopIntentHandler,
    QueryTemperatureIntentHandler,
    QueryHumidityIntentHandler)
  .lambda();