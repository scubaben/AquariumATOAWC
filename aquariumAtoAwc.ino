//declare variables
unsigned long waterChangeInterval = 43200000; // how often the water change should be executed - 12 hours
unsigned long maxExportRuntime = 300000; //the max time the export pump should run - 5 minutes - peristaltic pumps move about 400 ml/min, and desired water change volume is 1.2 liters or 3 minutes
unsigned long maxImportRuntime = 300000; //the max time the import pump should run - 5 minutes
unsigned long maxAtoRuntime = 120000; //the max time the ato pump should run - 2 minutes
unsigned long lastExportStart = 0; //the last time a water change export was started
unsigned long lastImportStart = 0;  //the last time a water change import was started
unsigned long lastAtoStart = 0; //the list time an ATO was started
unsigned long lastErrorPulse = 0; //the last time the ATO pump was pulsed to signify an error
unsigned long errorPulseDuration = 75; //the length of time the ATO pump should be pulsed when there is an error
unsigned long errorPulseInterval = 120000;  //the time between ATO pump pulses when there is an error
unsigned long lastDebounceTime = 0; //used for debouncing button presses
unsigned long debounceDelay = 50; //length of debounce delay
unsigned long lastSettingModeStart = 0; //last time the setting mode was started
unsigned long settingModeLenth = 3000; //length of the setting mode
unsigned long settingBlinkTime = 300;

bool errorPulseActive = false;
bool waterChangeExportRunning = false;
bool waterChangeImportRunning = false;
bool atoRunning = false;
bool atoFault = false;
bool exportFault = false;
bool importFault = false;
bool lastButtonState = false;
bool settingModeActive = false;

const int highTargetPin = 2;
const int lowTargetPin = 3;
const int lowReservoirPin = 4;
const int overfillPin = 5;
const int atoPin = 6;
const int exportPumpPin = 7;
const int exportValvePin = 8;
const int importPumpPin = 9;
const int buttonPin = 10;
const int atoLedPin = 11;
const int waterChangeLedPin = 12;
const int errorLedPin = 13;

void setup() {
  pinMode(highTargetPin, INPUT);
  pinMode(lowTargetPin, INPUT);
  pinMode(lowReservoirPin, INPUT);
  pinMode(overfillPin, INPUT_PULLUP);
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(atoPin, OUTPUT);
  pinMode(exportPumpPin, OUTPUT);
  pinMode(exportValvePin, OUTPUT);
  pinMode(importPumpPin, OUTPUT);
  pinMode(atoLedPin, OUTPUT);
  pinMode(waterChangeLedPin, OUTPUT);
  pinMode(errorLedPin, OUTPUT);
  delay(1000); //wait a second for all the sensors to initialize 
}

void loop() {
  updateDisplay();
  if(readButton()){
    lastSettingModeStart = millis();
    settingModeActive = true;
  }
  if(millis() > (lastSettingModeStart + settingModeLength)){
    settingModeActive = false;
  }

  if(settingModeActive){

  }

  //check for errors
  if(errorCheck() || reservoirEmpty()){
    errorAlert(true);
  }
  else {
    errorAlert(false);
  }

  //check if ATO should be started
  if(!highTarget() && !errorCheck() && !atoRunning && !waterChangeExportRunning && !waterChangeImportRunning && !atoFault){
    startAto();
  }
  //check if ATO should be ended
  if((highTarget() || overfilled() || errorCheck() || atoFault) && atoRunning){
    endAto();
  }
  //check if water change export should be started
  if(!errorCheck() && !reservoirEmpty() && !exportFault && !importFault && !atoRunning && !waterChangeExportRunning && !waterChangeImportRunning && (millis() > (lastExportStart + waterChangeInterval))){
    startWaterChangeExport();
  }
  //check if water change export should be ended
  if((!lowTarget() || errorCheck() || exportFault) && waterChangeExportRunning){
    endWaterChangeExport();
    //check if water change import should be started
    if(!errorCheck() && !waterChangeImportRunning && !importFault && !highTarget() && !overfilled()){
    startWaterChangeImport();
    }
  }
  //check if water change import should be ended
  if((errorCheck() || overfilled() || importFault || highTarget()) && waterChangeImportRunning){
    endWaterChangeImport();
  }
}

void softReset(){
  waterChangeExportRunning = false;
  waterChangeImportRunning = false;
  atoRunning = false;
  atoFault = false;
  exportFault = false;
  importFault = false;
  lastAtoStart = millis();
  lastExportStart = millis();
  lastImportStart = millis();
  digitalWrite(atoPin, LOW);
  digitalWrite(exportPumpPin, LOW);
  digitalWrite(exportValvePin, LOW);
  digitalWrite(importPumpPin, LOW);
  digitalWrite(atoLedPin, LOW);
  digitalWrite(waterChangeLedPin, LOW);
  digitalWrite(errorLedPin, LOW);
}

bool highTarget(){
  return digitalRead(highTargetPin);
}

bool lowTarget(){
  return digitalRead(lowTargetPin);
}

bool overfilled(){
  return !digitalRead(overfillPin);
}

bool reservoirEmpty() {
  return !digitalRead(lowReservoirPin);
}

bool errorCheck(){
  //check if ATO pump has been running too long
  if (atoRunning && (millis() > (lastAtoStart + maxAtoRuntime))){
    atoFault = true;
    digitalWrite(atoPin, LOW);
    atoRunning = false;
    return true;
  }
  // check if export pump has been running too long
  if (waterChangeExportRunning && (millis() > (lastExportStart + maxExportRuntime))){
    exportFault = true;
    digitalWrite(exportPumpPin, LOW);
    digitalWrite(exportValvePin, LOW);
    waterChangeExportRunning = false;
    return true;
  }
  // check if import pump has been running too long
  if (waterChangeImportRunning && (millis() > (lastImportStart + maxImportRuntime))){
    importFault = true;
    digitalWrite(importPumpPin, LOW);
    waterChangeImportRunning = false;
    return true;
  }
  //check if there is an error with any of the level sensors
  if(highTargetError() || lowTargetError() || overfilled()){
    return true;
  }
  else {
    return false;
  }
}
//check if the overfill sensor has been triggered, but the high target sensor has not
bool highTargetError() {
  if(overfilled() && !highTarget()){
    return true;
  }
  else {
    return false;
  }
}
//check if the overfill or high target sensors have been triggered, but the low target sensor has not
bool lowTargetError() {
  if((overfilled() || highTarget()) && !lowTarget()){
    return true;
  }
  else {
    return false;
  }
}

void startAto() {
  lastAtoStart = millis();
  digitalWrite(atoPin, HIGH);
  atoRunning = true;
}

void endAto() {
  digitalWrite(atoPin, LOW);
  atoRunning = false;
}

void startWaterChangeExport() {
  lastExportStart = millis();
  digitalWrite(exportValvePin, HIGH);
  digitalWrite(exportPumpPin, HIGH);
  waterChangeExportRunning = true;
}

void endWaterChangeExport() {
  digitalWrite(exportValvePin, LOW);
  digitalWrite(exportPumpPin, LOW);
  waterChangeExportRunning = false;
}

void startWaterChangeImport() {
  endWaterChangeExport();
  lastImportStart = millis();
  digitalWrite(importPumpPin, HIGH);
  waterChangeImportRunning = true;
}

void endWaterChangeImport() {
  digitalWrite(importPumpPin, LOW);
  waterChangeImportRunning = false;
}

void errorAlert(bool errorActive) {
  if(errorActive){
    if(!atoRunning){
      if(!errorPulseActive && (millis() > (lastErrorPulse + errorPulseInterval))){
        digitalWrite(atoPin, HIGH);
        lastErrorPulse = millis();
        errorPulseActive = true;
      }
      else if(errorPulseActive && (millis() > (lastErrorPulse + errorPulseDuration))){
        digitalWrite(atoPin, LOW);
        errorPulseActive = false;
      }
    }
  }
  else {
    if(!atoRunning){
      digitalWrite(atoPin, LOW);
      errorPulseActive = false;
    }
    digitalWrite(errorLedPin, LOW);
  }
}

bool readButton(){
  bool buttonPressed = !digitalRead(buttonPin); //button is active low, we'll convert it to true when the button is pressed to make it a bit easier to understand
  if(buttonPressed != lastButtonState){
    lastDebounceTime = millis();
  }
  lastButtonState = buttonPressed;
  if(millis() > (lastDebounceTime + debounceDelay)){
    return true;
  }
  else {
    return false;
  }
}

void updateDisplay(){
  if(!settingModeActive){ //if its not in setting mode, then display the ATO / water change / error indicators
    if(atoRunning){
      digitalWrite(atoLedPin, HIGH);
    }
    else {
      digitalWrite(atoLedPin, LOW);
    }

    if(waterChangeExportRunning || waterChangeImportRunning){
      digitalWrite(waterChangeLedPin, HIGH);
    }
    else {
        digitalWrite(waterChangeLedPin, LOW);
    }

    if(errorActive){
      digitalWrite(errorLedPin, HIGH);
    }
    else {
      digitalWrite(errorLedPin, LOW);
    }
  }

}