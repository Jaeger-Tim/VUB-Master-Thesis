#include "SparkFun_AS7265X.h" // Click here to get the library: http://librarymanager/All#SparkFun_AS7265X
#include <Wire.h>

AS7265X sensor;

// Define verbosity levels
#define VERBOSITY 4
#define N_WAVELENGTHS 18
#define MEASUREMENTS_PER_ITERATION 5
#define N_ITERATIONS 3

// Define the log macro
#if VERBOSITY > 0
  #define serialLog(message, logLevel) \
    do { \
      if (logLevel <= VERBOSITY) { \
        Serial.println(message); \
      } \
    } while (0)
#else
  #define serialLog(message, logLevel) do { } while (0)
#endif

// Pin definitions
const int buttonPin = 2; // Pin for the button
const int highPin = 4;
const int lowPin = 5;
const int ledPin = 13;   // Pin for an LED
const unsigned long debounceDelay = 200;  // Debounce delay in milliseconds

// Variables
volatile bool buttonPressed = false;
unsigned long lastDebounceTime = 0;
int mode = 0;

float tmpIntensity[MEASUREMENTS_PER_ITERATION][N_WAVELENGTHS];
float tmpMeanIntensitiesDark[N_ITERATIONS][N_WAVELENGTHS];
float tmpMeanIntensitiesLight[N_ITERATIONS][N_WAVELENGTHS];

float I_0[N_WAVELENGTHS];
float I_ref[N_WAVELENGTHS];
float I_s[N_WAVELENGTHS];

float K[N_WAVELENGTHS];

float c[N_WAVELENGTHS];

// Functions
void(* resetFunc) (void) = 0;

void setup() {
  // Initialize serial communication
  #if VERBOSITY > 0
    Serial.begin(115200);
  #endif

  Wire.begin(); // Initialize I2C communication
  Wire.setClock(20000); // Set I2C clock speed to 20kHz

  if (sensor.begin(Wire) == false)  {
    Serial.println("Sensor does not appear to be connected. Please check wiring. Restarting...");
    delay(300);
    resetFunc();
  }

  // Initialize button pin as input
  pinMode(buttonPin, INPUT_PULLUP);

  // Extra GND and VCC pin for testing
  pinMode(highPin, OUTPUT);
  digitalWrite(highPin, HIGH);
  pinMode(lowPin, OUTPUT);
  digitalWrite(lowPin, LOW);

  // Initialize LED pin as output
  pinMode(ledPin, OUTPUT);

  // Attach interrupt to the button pin
  attachInterrupt(digitalPinToInterrupt(buttonPin), buttonISR, FALLING);

  serialLog("Setup complete.", 2);
}

void loop() {
  serialLog("Start of loop iteration. Mode = " + String(mode), 3);
  static int iteration = 0;
  static int measurement = 0;

  switch (mode) {
    case 0:
      handleMode0();
      break;

    case 1:
      handleMode1(measurement, iteration);
      break;

    case 2:
      handleMode2(iteration);
      break;

    case 3:
      while (1);
      break;

    default:
      serialLog("Default case: Invalid state. Returning to mode = 0", 2);
      mode = 0;
      break;
  }

  delay(500);
}

void handleMode0() {
  serialLog("---------------------------------------", 2);
  serialLog("\nInsert the I_0 test tube and then press the button.\n", 2);

  while (!buttonPressed);

  if (buttonPressed) {
    mode++;
    serialLog("Case " + String(mode) + ": Dark measurements of I_0. Iteration 1.", 3);
    buttonPressed = false;
  }
}

void handleMode1(int &measurement, int &iteration) {
  //float readings[N_WAVELENGTHS];
  getSensorReadings(tmpIntensity[measurement], false);
/*
  for (int i = 0; i < N_WAVELENGTHS; i++) {
    tmpIntensity[measurement][i] = readings[i];
  }*/

  serialLog("Measurement " + String(measurement + 1) + "/" + String(MEASUREMENTS_PER_ITERATION) + " taken.", 3);
  measurement++;

  if (measurement >= MEASUREMENTS_PER_ITERATION) {
    calculateMedianPerWavelength(tmpIntensity, MEASUREMENTS_PER_ITERATION, N_WAVELENGTHS, tmpMeanIntensitiesDark, iteration);
    iteration++;
    measurement = 0;
    mode++;
    serialLog("Moving to next case: " + String(mode), 3);
  }
}

void handleMode2(int &iteration) {
  serialLog("Case " + String(mode), 3);
  if (iteration < N_ITERATIONS) {
    serialLog("Take the test tube out, mix the content, and re-insert it. When done, press the button.", 2);

    while (!buttonPressed);

    mode--;
    serialLog("Case " + String(mode) + ": Dark measurements of I_0. Iteration " + String(iteration+1) + ".", 3);

    buttonPressed = false;
  } else {
    mode++;
    iteration = 0;
  }
}

void buttonISR() {
  unsigned long currentTime = millis();  // Get the current time

  // Check if the time since the last interrupt is greater than the debounce delay
  if ((currentTime - lastDebounceTime) > debounceDelay) {
    buttonPressed = true;
    serialLog("Interrupt: Button ISR triggered.", 4);
    lastDebounceTime = currentTime;  // Update the last debounce time
  }
}

void getSensorReadings(float* arr, bool light) {
  if (light) {
    sensor.takeMeasurementsWithBulb(); // This is a hard wait while all 18 channels are measured
  } else {
    sensor.takeMeasurements(); // Take measurements without the bulb
  }

  arr[0] = sensor.getCalibratedA(); // 410nm
  arr[1] = sensor.getCalibratedB(); // 435nm
  arr[2] = sensor.getCalibratedC(); // 460nm
  arr[3] = sensor.getCalibratedD(); // 485nm
  arr[4] = sensor.getCalibratedE(); // 510nm
  arr[5] = sensor.getCalibratedF(); // 535nm
  arr[6] = sensor.getCalibratedG(); // 560nm
  arr[7] = sensor.getCalibratedH(); // 585nm
  arr[8] = sensor.getCalibratedR(); // 610nm
  arr[9] = sensor.getCalibratedI(); // 645nm
  arr[10] = sensor.getCalibratedS(); // 680nm
  arr[11] = sensor.getCalibratedJ(); // 705nm
  arr[12] = sensor.getCalibratedT(); // 730nm
  arr[13] = sensor.getCalibratedU(); // 760nm
  arr[14] = sensor.getCalibratedV(); // 810nm
  arr[15] = sensor.getCalibratedW(); // 860nm
  arr[16] = sensor.getCalibratedK(); // 900nm
  arr[17] = sensor.getCalibratedL(); // 940nm
}

void calculateMedianPerWavelength(float inputArray[][N_WAVELENGTHS], int rows, int cols, float outputArray[][N_WAVELENGTHS], int currentIteration) {
  for (int wavelength = 0; wavelength < cols; wavelength++) {
    float tempArray[rows];

    // Collect all measurements for the current wavelength
    for (int i = 0; i < rows; i++) {
      tempArray[i] = inputArray[i][wavelength];
    }

    // Sort the collected measurements
    for (int i = 0; i < rows - 1; i++) {
      for (int j = i + 1; j < rows; j++) {
        if (tempArray[i] > tempArray[j]) {
          float temp = tempArray[i];
          tempArray[i] = tempArray[j];
          tempArray[j] = temp;
        }
      }
    }

    // Calculate the median for the current wavelength
    float median;
    if (rows % 2 == 0) {
      median = (tempArray[rows / 2 - 1] + tempArray[rows / 2]) / 2.0;
    } else {
      median = tempArray[rows / 2];
    }

    // Store the median in the output array for the current iteration
    outputArray[currentIteration][wavelength] = median;
  }
}