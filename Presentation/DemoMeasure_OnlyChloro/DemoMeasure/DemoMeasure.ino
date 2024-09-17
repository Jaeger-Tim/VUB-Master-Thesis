#include "SparkFun_AS7265X.h" // Click here to get the library: http://librarymanager/All#SparkFun_AS7265X
#include <Wire.h>

AS7265X sensor;

// Define verbosity levels
#define VERBOSITY 4
#define N_WAVELENGTHS 18
#define MEASUREMENTS_PER_ITERATION 6
#define N_ITERATIONS 4
#define REF_CONCENTRATION 60.0

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
const int highPin = 4;
const int lowPin = 5;
const int ledPin = 13;   // Pin for an LED

// Variables
int mode = 0;

float tmpIntensity[MEASUREMENTS_PER_ITERATION][N_WAVELENGTHS];
float tmpMeanIntensitiesDark[N_ITERATIONS][N_WAVELENGTHS];
float tmpMeanIntensitiesLight[N_ITERATIONS][N_WAVELENGTHS];

float I_0[N_WAVELENGTHS];
float I_ref[N_WAVELENGTHS];
float I_s[N_WAVELENGTHS];

float K[N_WAVELENGTHS];

float c[N_WAVELENGTHS] = {REF_CONCENTRATION, REF_CONCENTRATION, REF_CONCENTRATION, REF_CONCENTRATION, REF_CONCENTRATION, REF_CONCENTRATION, REF_CONCENTRATION, REF_CONCENTRATION, REF_CONCENTRATION, REF_CONCENTRATION, REF_CONCENTRATION, REF_CONCENTRATION, REF_CONCENTRATION, REF_CONCENTRATION, REF_CONCENTRATION, REF_CONCENTRATION, REF_CONCENTRATION, REF_CONCENTRATION};

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

  // Extra GND and VCC pin for testing
  pinMode(highPin, OUTPUT);
  digitalWrite(highPin, HIGH);
  pinMode(lowPin, OUTPUT);
  digitalWrite(lowPin, LOW);

  // Initialize LED pin as output
  pinMode(ledPin, OUTPUT);

  serialLog("Setup complete.", 2);
}

void loop() {
  serialLog("Start of loop iteration. Mode = " + String(mode), 3);
  static int iteration = 0;
  static int measurement = 0;

  switch (mode) {
    case 0:
      promptForInitialMeasurement();
      break;

    case 1:
      takeMeasurement(measurement, iteration, false); // Dark measurement
      break;

    case 2:
      takeMeasurement(measurement, iteration, true); // Light measurement
      break;

    case 3:
      promptForNextIteration(iteration);
      break;

    case 4:
      calculateI0();
      break;

    case 5:
      promptForReferenceMeasurement();
      break;

    case 6:
      takeMeasurement(measurement, iteration, false); // Dark measurement for I_ref
      break;

    case 7:
      takeMeasurement(measurement, iteration, true); // Light measurement for I_ref
      break;

    case 8:
      promptForNextIteration(iteration);
      break;

    case 9:
      calculateIref();
      break;

    case 10:
      calculateK();
      break;

    case 11:
      promptForSampleMeasurement();
      break;

    case 12:
      takeMeasurement(measurement, iteration, false); // Dark measurement for sample
      break;

    case 13:
      takeMeasurement(measurement, iteration, true); // Light measurement for sample
      break;

    case 14:
      promptForNextIteration(iteration);
      break;

    case 15:
      calculateSampleConcentration();
      break;

    case 16:
      // Instead of stopping, reset to prompt for a new sample measurement
      serialLog("Sample measurement complete. Ready for new sample.", 2);
      mode = 11; // Go back to prompt for sample measurement
      iteration = 0; // Reset iteration
      measurement = 0; // Reset measurement
      break;

    default:
      serialLog("Default case: Invalid state. Returning to mode = 0", 2);
      mode = 0;
      break;
  }

  delay(500);
}

void promptForInitialMeasurement() {
  serialLog("---------------------------------------", 2);
  serialLog("\nInsert the I_0 test tube and then type 'start' in the serial monitor.\n", 2);

  while (!Serial.available() || Serial.readString() != "start");

  mode++;
  serialLog("Case " + String(mode) + ": Dark measurements of I_0. Iteration 1.", 3);
}

void takeMeasurement(int &measurement, int &iteration, bool light) {
  getSensorReadings(tmpIntensity[measurement], light);

  serialLog("Measurement " + String(measurement + 1) + "/" + String(MEASUREMENTS_PER_ITERATION) + " taken.", 3);
  #if VERBOSITY >= 3
    if (mode == 1 || mode == 2) {
      Serial.print("I_0_");
    } else if (mode == 6 || mode == 7) {
      Serial.print("I_ref_");
    } else if (mode == 12 || mode == 13) {
      Serial.print("I_s_");
    }
    Serial.print(light ? "Light: " : "Dark: ");
    for (int i = 0; i < N_WAVELENGTHS; i++) {
      Serial.print(tmpIntensity[measurement][i]);
      if(i != N_WAVELENGTHS-1)
        Serial.print(", ");
      else
        Serial.println("");
    }
  #endif
  measurement++;

  if (measurement >= MEASUREMENTS_PER_ITERATION) {
    if (light) {
      calculateMedianPerWavelength(tmpIntensity, MEASUREMENTS_PER_ITERATION, N_WAVELENGTHS, tmpMeanIntensitiesLight, iteration);
    } else {
      calculateMedianPerWavelength(tmpIntensity, MEASUREMENTS_PER_ITERATION, N_WAVELENGTHS, tmpMeanIntensitiesDark, iteration);
    }
    #if VERBOSITY >= 3
      if (mode == 1 || mode == 2) {
        Serial.print("I_0_");
      } else if (mode == 6 || mode == 7) {
        Serial.print("I_ref_");
      } else if (mode == 12 || mode == 13) {
        Serial.print("I_s_");
      }
      Serial.print(light ? "Light_Mean: " : "Dark_Mean: ");
      for (int i = 0; i < N_WAVELENGTHS; i++) {
        Serial.print(light ? tmpMeanIntensitiesLight[iteration][i] : tmpMeanIntensitiesDark[iteration][i]);
        if(i != N_WAVELENGTHS-1)
          Serial.print(", ");
        else
          Serial.println("");
      }
    #endif
    measurement = 0;
    mode++;
    serialLog("Moving to next case: " + String(mode), 3);
  }
}

void promptForNextIteration(int &iteration) {
  serialLog("Case " + String(mode), 3);
  if (iteration < N_ITERATIONS - 1) { // Fix: iterate N_ITERATIONS times
    serialLog("Take the test tube out, mix the content, and re-insert it. When done, type 'next' in the serial monitor.", 2);

    while (!Serial.available() || Serial.readString() != "next");

    mode = (mode == 3 || mode == 8 || mode == 14) ? mode - 2 : mode - 1; // Alternate between dark and light measurements
    serialLog("Case " + String(mode) + ": " + (mode == 1 || mode == 6 || mode == 12 ? "Dark" : "Light") + " measurements of " + (mode == 1 || mode == 2 ? "I_0" : (mode == 6 || mode == 7 ? "I_ref" : "I_s")) + ". Iteration " + String(iteration+2) + ".", 3);

    iteration++; // Increment iteration here
  } else {
    mode++;
    iteration = 0;
  }
}

void promptForReferenceMeasurement() {
  serialLog("---------------------------------------", 2);
  serialLog("\nInsert the I_ref test tube and then type 'start' in the serial monitor.\n", 2);

  while (!Serial.available() || Serial.readString() != "start");

  mode++;
  serialLog("Case " + String(mode) + ": Dark measurements of I_ref. Iteration 1.", 3);
}

void promptForSampleMeasurement() {
  serialLog("---------------------------------------", 2);
  serialLog("\nInsert the sample test tube and then type 'start' in the serial monitor.\n", 2);

  while (!Serial.available() || Serial.readString() != "start");

  mode++;
  serialLog("Case " + String(mode) + ": Dark measurements of sample. Iteration 1.", 3);
}

void calculateI0() {
  for (int wavelength = 0; wavelength < N_WAVELENGTHS; wavelength++) {
    float tempArray[N_ITERATIONS];

    // Calculate the difference between light and dark measurements for each iteration
    for (int i = 0; i < N_ITERATIONS; i++) {
      tempArray[i] = tmpMeanIntensitiesLight[i][wavelength] - tmpMeanIntensitiesDark[i][wavelength];
    }

    // Sort the differences
    for (int i = 0; i < N_ITERATIONS - 1; i++) {
      for (int j = i + 1; j < N_ITERATIONS; j++) {
        if (tempArray[i] > tempArray[j]) {
          float temp = tempArray[i];
          tempArray[i] = tempArray[j];
          tempArray[j] = temp;
        }
      }
    }

    // Calculate the median of the differences
    float median;
    if (N_ITERATIONS % 2 == 0) {
      median = (tempArray[N_ITERATIONS / 2 - 1] + tempArray[N_ITERATIONS / 2]) / 2.0;
    } else {
      median = tempArray[N_ITERATIONS / 2];
    }

    // Store the median in I_0
    I_0[wavelength] = median;
  }

  #if VERBOSITY >= 3
    Serial.print("I_0: ");
    for (int i = 0; i < N_WAVELENGTHS; i++) {
      Serial.print(I_0[i]);
      if(i != N_WAVELENGTHS-1)
        Serial.print(", ");
      else
        Serial.println("");
    }
  #endif

  mode++;
}

void calculateIref() {
  for (int wavelength = 0; wavelength < N_WAVELENGTHS; wavelength++) {
    float tempArray[N_ITERATIONS];

    // Calculate the difference between light and dark measurements for each iteration
    for (int i = 0; i < N_ITERATIONS; i++) {
      tempArray[i] = tmpMeanIntensitiesLight[i][wavelength] - tmpMeanIntensitiesDark[i][wavelength];
    }

    // Sort the differences
    for (int i = 0; i < N_ITERATIONS - 1; i++) {
      for (int j = i + 1; j < N_ITERATIONS; j++) {
        if (tempArray[i] > tempArray[j]) {
          float temp = tempArray[i];
          tempArray[i] = tempArray[j];
          tempArray[j] = temp;
        }
      }
    }

    // Calculate the median of the differences
    float median;
    if (N_ITERATIONS % 2 == 0) {
      median = (tempArray[N_ITERATIONS / 2 - 1] + tempArray[N_ITERATIONS / 2]) / 2.0;
    } else {
      median = tempArray[N_ITERATIONS / 2];
    }

    // Store the median in I_ref
    I_ref[wavelength] = median;
  }

  #if VERBOSITY >= 3
    Serial.print("I_ref: ");
    for (int i = 0; i < N_WAVELENGTHS; i++) {
      Serial.print(I_ref[i]);
      if(i != N_WAVELENGTHS-1)
        Serial.print(", ");
      else
        Serial.println("");
    }
  #endif

  mode++;
}

void calculateK() {
  for (int wavelength = 0; wavelength < N_WAVELENGTHS; wavelength++) {
    K[wavelength] = (log10(I_0[wavelength] / I_ref[wavelength])) / c[wavelength];
  }

  #if VERBOSITY >= 3
    Serial.print("K: ");
    for (int i = 0; i < N_WAVELENGTHS; i++) {
      Serial.print(K[i]);
      if(i != N_WAVELENGTHS-1)
        Serial.print(", ");
      else
        Serial.println("");
    }
  #endif

  mode++;
}

void calculateSampleConcentration() {
  for (int wavelength = 0; wavelength < N_WAVELENGTHS; wavelength++) {
    float tempArray[N_ITERATIONS];

    // Calculate the difference between light and dark measurements for each iteration
    for (int i = 0; i < N_ITERATIONS; i++) {
      tempArray[i] = tmpMeanIntensitiesLight[i][wavelength] - tmpMeanIntensitiesDark[i][wavelength];
    }

    // Sort the differences
    for (int i = 0; i < N_ITERATIONS - 1; i++) {
      for (int j = i + 1; j < N_ITERATIONS; j++) {
        if (tempArray[i] > tempArray[j]) {
          float temp = tempArray[i];
          tempArray[i] = tempArray[j];
          tempArray[j] = temp;
        }
      }
    }

    // Calculate the median of the differences
    float median;
    if (N_ITERATIONS % 2 == 0) {
      median = (tempArray[N_ITERATIONS / 2 - 1] + tempArray[N_ITERATIONS / 2]) / 2.0;
    } else {
      median = tempArray[N_ITERATIONS / 2];
    }

    // Store the median in I_s
    I_s[wavelength] = median;
  }

  // Calculate the concentration using Beer-Lambert law
  for (int wavelength = 0; wavelength < N_WAVELENGTHS; wavelength++) {
    c[wavelength] = (log10(I_0[wavelength]/I_s[wavelength])) / K[wavelength];
  }

  // Only print the concentration for 560 nm
  #if VERBOSITY >= 3
    Serial.print("Concentration at 560 nm: ");
    Serial.print(c[6]); // Only print 560 nm concentration
    Serial.println("");
  #endif

  mode++;
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