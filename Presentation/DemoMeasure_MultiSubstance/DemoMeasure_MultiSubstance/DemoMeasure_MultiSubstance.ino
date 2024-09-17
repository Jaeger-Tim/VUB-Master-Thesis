/****
 * This script measures the concentration of a mixture containing
 * two different solutions at an unknown concentration using
 * the AS7265X based photospectrograph.

 * Author: Tim Jäger
 * For: VUB - Master Thesis
 * Development date: 2023-2024

 * Settings:
 *    Measuring:
 *    - MEASUREMENTS_PER_ITERATION: The script takes a number of 
 *          measurements and takes their median.
 *    - N_ITERATIONS: The above process will happen this amount of
 *          times and then again have the median be taken. Each
 *          iteration, one will be prompted to take out the test
 *          tube and then re-insert it slightly rotated. This
 *          reduces the effects of imperfections and particles.
 *    - REF_CONCENTRATION_A: This is the concentration in the
 *          reference solution A. In this thesis, this is
 *          chlorophyll.
 *    - REF_CONCENTRATION_B: This is the concentration in the
 *          reference solution B. In this thesis, this is
 *          red dye.
 *    - ALWAYS_MEASURE_DARK: A dark measurement is used to decrease
 *          the error when there is some ambient light leakage. In
 *          some cases, this is not needed. If unsure, set this to
 *          true.
 *    Pins:
 *    - BUTTON_PIN: This pin has to be interrupt compatible.
 *    - HIGH_PIN: For convinience, this pin is used to get another
 *          VCC reference, used for the button.
 *    - LOW_PIN: For convinience, this pin is used to get another
 *          GND reference, used for the button.
 *    - LED_PIN: The clasic LED pin.
 *
 *    Misc:
 *    - VERBOSITY: This is the level of detail logged in over serial.
 *          (Cumulative)
 *          0: No serial. Serial will entirely be disabled.
 *          1: Serial will be enabled. Only the data will be transmitted.
 *          2: Default. Instructions will be shown.
 *          3: More detailed data will be shown. Not all.
 *          4: Debug. All details will be shown.
 *    - DEBOUNCE_DELAY: It is in ms. 200 works well. Going higher is
 *          also no problem.
 */

/**** Settings ****/
// Measuring
#define MEASUREMENTS_PER_ITERATION 4
#define N_ITERATIONS 6
#define REF_CONCENTRATION_A 60.0
#define REF_CONCENTRATION_B 60.0
#define ALWAYS_MEASURE_DARK false

// Pins
#define BUTTON_PIN 2
#define HIGH_PIN 4
#define LOW_PIN 5
#define LED_PIN 13

// Misc.
#define VERBOSITY 4
#define DEBOUNCE_DELAY 200  // Debounce delay in milliseconds

/***********************************************************************************************************/
/****                                               INIT                                                ****/
/***********************************************************************************************************/

/**** Maths ****/
#include <BasicLinearAlgebra.h>

/**** Load photospectroscopy sensor ****/
#include "SparkFun_AS7265X.h" // Click here to get the library: http://librarymanager/All#SparkFun_AS7265X
#include <Wire.h>

AS7265X sensor;

#define N_WAVELENGTHS 18

/**** Serial Logging ****/
#if VERBOSITY > 0
  #define serialLogLn(message, logLevel) \
    do { \
      if (logLevel <= VERBOSITY) { \
        Serial.println(message); \
      } \
    } while (0)
  
  #define serialLog(message, logLevel) \
    do { \
      if (logLevel <= VERBOSITY) { \
        Serial.print(message); \
      } \
    } while (0)

  #define serialLog2(message, param2, logLevel) \
  do { \
    if (logLevel <= VERBOSITY) { \
      Serial.print(message, param2); \
    } \
  } while (0)
#else
  #define serialLogLn(message, logLevel) do { } while (0)
  #define serialLog(message, logLevel) do { } while (0)
  #define serialLog2(message, param2, logLevel) do { } while (0)
#endif

/**** Flags and Status ****/
volatile bool buttonPressed = false;
unsigned long lastDebounceTime = 0;
int mode = 0;

/**** Variales ****/
// Spectral
float tmpIntensity[2][MEASUREMENTS_PER_ITERATION][N_WAVELENGTHS]; // 2 is for dark (0) and light (1)
float tmpMeanIntensitiesDark[N_ITERATIONS][N_WAVELENGTHS];
float tmpMeanIntensitiesLight[N_ITERATIONS][N_WAVELENGTHS];
float tmpMeanIntensitiesAdjusted[N_ITERATIONS][N_WAVELENGTHS];

float I_0[N_WAVELENGTHS]; // Intensity
float I_Ref_A[N_WAVELENGTHS];
float I_Ref_B[N_WAVELENGTHS];
float I_S[N_WAVELENGTHS];

float A_Ref_A[N_WAVELENGTHS];
float A_Ref_B[N_WAVELENGTHS];
float A_S[N_WAVELENGTHS];

float K_A[N_WAVELENGTHS]; // ε[λ]*l
float K_B[N_WAVELENGTHS];

float c_A;//[N_WAVELENGTHS]; // Concentration
float c_B;//[N_WAVELENGTHS];

/**** Misc. ****/
enum MeasurementType_Enum {BASE, REFERENCE_A, REFERENCE_B, SAMPLE};

/***********************************************************************************************************/
/****                                               MAIN                                                ****/
/***********************************************************************************************************/

/**** Special Functions ****/
void(* resetFunc) (void) = 0;

/**** Setup ****/
void setup() {
  /**** Initialise Pins ****/
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  pinMode(HIGH_PIN, OUTPUT);
  pinMode(LOW_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);

  digitalWrite(HIGH_PIN, HIGH);
  digitalWrite(LOW_PIN, LOW);

  /**** Setup Logging ***/
  #if VERBOSITY > 0
    Serial.begin(115200);
  #endif

  /**** Setup AS7265X ***/
  Wire.begin(); // Initialize I2C communication
  Wire.setClock(20000); // Set I2C clock speed to 20kHz

  if (sensor.begin(Wire) == false)  {
    Serial.println("Connecting to sensor failed. This might take a few attempts. Restarting...");
    delay(300);
    resetFunc();
  }

  
  /**** Setup Button ISR ****/
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonISR, FALLING);

  /**** Finish Setup ****/
  serialLogLn("\nSetup complete.", 2);
}

/**** Loop ****/
void loop() {
  switch(mode) {
    case 0: // I_0
      measureIntensity(I_0, BASE);

      #if VERBOSITY >= 2
        serialLog("\nI_0, ", 2); printFloatArray(I_0, 3); serialLogLn("\n", 2);
      #endif

      mode++;

      break;

    case 1: // I_Ref_A
      measureIntensity(I_Ref_A, REFERENCE_A);

      #if VERBOSITY >= 2
        serialLog("\nI_Ref_A, ", 2); printFloatArray(I_Ref_A, 3); serialLogLn("\n", 2);
      #endif

      mode++;
      break;

    case 2: // I_Ref_B
      measureIntensity(I_Ref_B, REFERENCE_B);

      #if VERBOSITY >= 2
        serialLog("\nI_Ref_B, ", 2); printFloatArray(I_Ref_B, 3); serialLogLn("\n", 2);
      #endif

      mode++;
      break;

    case 3: // I_S
      measureIntensity(I_S, SAMPLE);

      #if VERBOSITY >= 2
        serialLog("\nI_Ref_B, ", 2); printFloatArray(I_S, 3); serialLogLn("\n", 2);
      #endif

      mode++;
      break;

    case 4: // A_Ref_A, A_Ref_B, A_S
      intensityToAbsorbance(I_0, I_Ref_A, A_Ref_A);
      intensityToAbsorbance(I_0, I_Ref_B, A_Ref_B);
      intensityToAbsorbance(I_0, I_S, A_S);
      
      #if VERBOSITY >= 2
        serialLog("\nA_Ref_A, ", 2); printFloatArray(A_Ref_A, 3); serialLogLn("\n", 2);
        serialLog("\nA_Ref_B, ", 2); printFloatArray(A_Ref_B, 3); serialLogLn("\n", 2);
        serialLog("\nA_S, ", 2); printFloatArray(A_S, 3); serialLogLn("\n", 2);
      #endif

      mode++;

      break;

    case 5: // K_A, K_B
      for(int lambda = 0; lambda < N_WAVELENGTHS; lambda++) {
        K_A[lambda] = A_Ref_A[lambda] / REF_CONCENTRATION_A;
        K_B[lambda] = A_Ref_B[lambda] / REF_CONCENTRATION_B;
      }

      #if VERBOSITY >= 2
        serialLog("\nK_A, ", 2); printFloatArray(K_A, 4); serialLogLn("\n", 2);
        serialLog("\nK_B, ", 2); printFloatArray(K_B, 4); serialLogLn("\n", 2);
      #endif
      
      mode++;

      break;

    case 6:
      calculateConcentrations(A_S, K_A, K_B, N_WAVELENGTHS, c_A, c_B);
      serialLogLn("*************************"
                  "\n"
                  "c_A, " + String(c_A) + \
                  "\n"
                  "c_B, " + String(c_B) + \
                  "\n"
                  "*************************\n", 1);

      calculateSampleConcentration(I_0, I_S, K_A);

      mode++;
    
      break;

    case 7:
      mode = 3;
      break;

    default:
      serialLogLn("\n\n!!!!!!!!!!\n\nError: Entered default case in main loop, resetting the device.\n\n!!!!!!!!!!\n", 2);
      delay(100); // Give enough time for the message to be sent
      resetFunc();
      break;
  }
}

/**** Interrupts ****/
void buttonISR() {
  unsigned long currentTime = millis();  // Get the current time

  // Check if the time since the last interrupt is greater than the debounce delay
  if ((currentTime - lastDebounceTime) > DEBOUNCE_DELAY) {
    buttonPressed = true;
    serialLogLn("Interrupt: Button ISR triggered.", 4);
    lastDebounceTime = currentTime;  // Update the last debounce time
  }
}

/***********************************************************************************************************/
/****                                               MODES                                               ****/
/***********************************************************************************************************/



/***********************************************************************************************************/
/****                                            PERIPHERALS                                            ****/
/***********************************************************************************************************/

/**
 * Stores the sensor readings in the array arr.
 * Light, determines if it is a dark or light measurement.
 */
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


/***********************************************************************************************************/
/****                                           INSTRUCTIONS                                            ****/
/***********************************************************************************************************/

void instructions_I_0_Start() {
  serialLogLn("\n"
              "----------------- I_0 -----------------\n"
              "\n"
              "Mix the base-line test tube by turning it upside down and back a few times and insert into the holder.\n"
              "When ready place the cap on the device and the press the button to start the first measuring itteration.\n"
              "\n"
              "----------------- I_0 -----------------\n"
              "\n"
              , 2);

  waitForButton();
}

void instructions_I_0_Iteration() {
  serialLogLn("\n"
              "-  -  -  -  -  -  I_0  -  -  -  -  -  -\n"
              "\n"
              "Take the test tube out and mix it again. When done, re-insert, put the cap on, and press the button.\n"
              "\n"
              "-  -  -  -  -  -  I_0  -  -  -  -  -  -\n"
              "\n"
              , 2);

  waitForButton();
}

void instructions_I_Ref_A_Start() {
  serialLogLn("\n"
              "--------------- I_Ref_A ---------------\n"
              "\n"
              "Mix the RED reference test tube by turning it upside down and back a few times and insert into the holder.\n"
              "When ready place the cap on the device and the press the button to start the first measuring itteration.\n"
              "\n"
              "--------------- I_Ref_A ---------------\n"
              "\n"
              , 2);

  waitForButton();
}

void instructions_I_Ref_A_Iteration() {
  serialLogLn("\n"
              " -  -  -  -  -  I_Ref_A  -  -  -  -  - \n"
              "\n"
              "Take the test tube out and mix it again. When done, re-insert, put the cap on, and press the button.\n"
              "\n"
              " -  -  -  -  -  I_Ref_A  -  -  -  -  - \n"
              "\n"
              , 2);

  waitForButton();
}

void instructions_I_Ref_B_Start() {
  serialLogLn("\n"
              "--------------- I_Ref_B ---------------\n"
              "\n"
              "Mix the CHLOROPHYLL reference test tube by turning it upside down and back a few times and\n"
              "insert into the holder.\nWhen ready place the cap on the device and the press the button to\n"
              "start the first measuring itteration.\n"
              "\n"
              "--------------- I_Ref_B ---------------\n"
              "\n"
              , 2);

  waitForButton();
}

void instructions_I_Ref_B_Iteration() {
  serialLogLn("\n"
              " -  -  -  -  -  I_Ref_B  -  -  -  -  - \n"
              "\n"
              "Take the test tube out and mix it again. When done, re-insert, put the cap on, and press the button.\n"
              "\n"
              " -  -  -  -  -  I_Ref_B  -  -  -  -  - \n"
              "\n"
              , 2);

  waitForButton();
}

void instructions_I_Sample_Start() {
  serialLogLn("\n"
              "----------------- I_S -----------------\n"
              "\n"
              "Mix the SAMPLE test tube by turning it upside down and back a few times and insert\n"
              "into the holder. When ready place the cap on the device and the press the button to\n"
              "start the first measuring itteration.\n"
              "\n"
              "----------------- I_S -----------------\n"
              "\n"
              , 2);

  waitForButton();
}

void instructions_I_Sample_Iteration() {
  serialLogLn("\n"
              "-  -  -  -  -  -  I_S  -  -  -  -  -  -\n"
              "\n"
              "Take the test tube out and mix it again. When done, re-insert, put the cap on, and press the button.\n"
              "\n"
              "-  -  -  -  -  -  I_S  -  -  -  -  -  -\n"
              "\n"
              , 2);

  waitForButton();
}


/***********************************************************************************************************/
/****                                             MEASURING                                             ****/
/***********************************************************************************************************/

void measureIntensity(float I[], MeasurementType_Enum measType){
  switch(measType) {
    case BASE:
      instructions_I_0_Start();
      break;
    case REFERENCE_A:
      instructions_I_Ref_A_Start();
      break;
    case REFERENCE_B:
      instructions_I_Ref_B_Start();
      break;
    case SAMPLE:
      instructions_I_Sample_Start();
      break;
    default:
      serialLogLn("Error: measureIntensity() first switch default case.", 2);
      break;
  }

  /**** Make measurements ****/
  for(int i = 0; i < N_ITERATIONS; i++) {
    
    // If we are not in the first iteration, give the instructions for mixing again and wait for confirmation.
    if( i != 0 ) {
      switch(measType) {
        case BASE:
          instructions_I_0_Iteration();
          break;
        case REFERENCE_A:
          instructions_I_Ref_A_Iteration();
          break;
        case REFERENCE_B:
          instructions_I_Ref_B_Iteration();
          break;
        case SAMPLE:
          instructions_I_Sample_Iteration();
          break;
        default:
          serialLogLn("Error: measureIntensity() second switch default case.", 2);
          break;
      }
    }
    
    // Iteration counter in logs
    serialLogLn("Iteration " + String(i+1) + "/" + String(N_ITERATIONS) + ".", 2);
    
    // Will be used if ALWAYS_MEASURE_DARK is false, to speed up readings in a fully dark environment.
    // If it there is still any light measured, it will follow the regular procedure of measuring its intensity.
    bool fullyDark = true;
    
    for(int m = 0; m < MEASUREMENTS_PER_ITERATION; m++) {
      serialLogLn("Measurement " + String(m+1) + "/" + String(MEASUREMENTS_PER_ITERATION) + ".", 2);

      /**** Make Dark Measurement ****/  
      #if ALWAYS_MEASURE_DARK == true
        getSensorReadings(tmpIntensity[0][m], false); // Dark
        #if VERBOSITY >= 4
          serialLog("Dark, ", 4); printFloatArray(tmpIntensity[0][m], 2); serialLogLn("", 4);
        #endif
      #else
        if(m == 0) {
          getSensorReadings(tmpIntensity[0][m], false); // Dark
          #if VERBOSITY >= 4
            serialLog("Dark, ", 4); printFloatArray(tmpIntensity[0][m], 2); serialLogLn("", 4);
          #endif

          for(int j = 0; j < N_WAVELENGTHS; j++) {
            if(tmpIntensity[0][0][j] != 0)
              fullyDark = false;
          }
        } else if (fullyDark) {
          for (int lambda = 0; lambda < N_WAVELENGTHS; lambda++) {
            tmpIntensity[0][m][lambda] = 0;
          }
        } else {
          getSensorReadings(tmpIntensity[0][m], false); // Dark
          #if VERBOSITY >= 4
            serialLog("Dark, ", 4); printFloatArray(tmpIntensity[0][m], 2); serialLogLn("", 4);
          #endif
        }
      #endif

      /**** Make Light Measurements ****/
      getSensorReadings(tmpIntensity[1][m], true); // Light
      #if VERBOSITY >= 4
        serialLog("Light, ", 4); printFloatArray(tmpIntensity[1][m], 2); serialLogLn("", 4);
      #endif
    }
    
    /**** Caculate Median Intensity ****/
    calculateColumnMedians(tmpIntensity[0], tmpMeanIntensitiesDark[i]);
    calculateColumnMedians(tmpIntensity[1], tmpMeanIntensitiesLight[i]);

    #if VERBOSITY >= 3
      serialLog("TMP_Mean_Dark, ", 3); printFloatArray(tmpMeanIntensitiesDark[i], 3);
      serialLog("\nTMP_Mean_Light, ", 3); printFloatArray(tmpMeanIntensitiesLight[i], 3); serialLogLn("\n", 3);
    #endif

    /**** Subtract Dark Intensity ****/
    for(int lambda = 0; lambda < N_WAVELENGTHS; lambda++)
      tmpMeanIntensitiesLight[i][lambda] = tmpMeanIntensitiesLight[i][lambda] - tmpMeanIntensitiesDark[i][lambda];
  }

  /**** Calculate Median and Return it to I ****/
  calculateColumnMedians(tmpMeanIntensitiesLight, I);
}

/***********************************************************************************************************/
/****                                               MATHS                                               ****/
/***********************************************************************************************************/

/**
 * Returns the median of arr[].
 */
float calculateMedian(const float arr[], int size) {
  // Create a copy of the array to avoid altering the original
  float* tmpArr = new float[size];
  for (int i = 0; i < size; i++) {
    tmpArr[i] = arr[i];
  }

  // Sort the copy of the array
  for (int i = 0; i < size - 1; i++) {
    for (int j = i + 1; j < size; j++) {
      if (tmpArr[i] > tmpArr[j]) {
        float temp = tmpArr[i];
        tmpArr[i] = tmpArr[j];
        tmpArr[j] = temp;
      }
    }
  }

  // Calculate median
  float median;
  if (size % 2 == 0) {
    median = (tmpArr[size / 2 - 1] + tmpArr[size / 2]) / 2.0;
  } else {
    median = tmpArr[size / 2];
  }

  // Free the allocated memory
  delete[] tmpArr;

  return median;
}

/**
 * Function to calculate the median of each column in a 2D array.
 * Takes a 2D array and outputs a 1D one with the medians.
 */
template <size_t Rows, size_t Cols> void calculateColumnMedians(float (&array2D)[Rows][Cols], float* medians) {
  for (int col = 0; col < Cols; col++) {
    float columnArray[Rows];
    for (int row = 0; row < Rows; row++) {
      columnArray[row] = array2D[row][col];
    }
    medians[col] = calculateMedian(columnArray, Rows);
  }
}

void intensityToAbsorbance(float I_0[N_WAVELENGTHS], float I[N_WAVELENGTHS], float A[N_WAVELENGTHS]) {
  for(int i = 0; i < N_WAVELENGTHS; i++)
    A[i] = log10(I_0[i]/I[i]);
}

void calculateConcentrations(float* A_S, float* K_A, float* K_B, int numWavelengths, float& c_A, float& c_B) {
  /*****
   * Sometimes the environment doesn't permit the use of certain frequencies.
   * In this case, the plastic test tubes struggle especially with uniformity in the UV and IR ranges.
   * In this array, the representative wavelength can be "disabled", meaning it won't be used in the
   * calculations.
   *****/
  //                                410     435     460     485     510     535     560   585   610     645   ?680  705   ?730  760     810     860     900     940
  const bool includeWavelength[] = {false,  false,  false,  false,  false,  false,  true, true, false,  true, true, true, true, false,  false,  false,  false,  false};
  
  // Create the matrix E and vector A
  BLA::Matrix<2, 2> E;
  BLA::Matrix<2> A;
  BLA::Matrix<2> C;

  // Initialize the matrix E and vector A to zero
  E.Fill(0);
  A.Fill(0);

  // Initialize the matrix E and vector A
  for (int i = 0; i < numWavelengths; i++) {
    // Skip invalid values
    if (!includeWavelength[i] || K_A[i] <= 0 || K_B[i] <= 0) {
      continue;
    }

    E(0, 0) += K_A[i] * K_A[i];
    E(0, 1) += K_A[i] * K_B[i];
    E(1, 0) += K_B[i] * K_A[i];
    E(1, 1) += K_B[i] * K_B[i];
    A(0) += K_A[i] * A_S[i];
    A(1) += K_B[i] * A_S[i];
  }

  // Log the matrix E and vector A
  serialLogLn("Matrix E:", 3);
  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 2; j++) {
      serialLog(String(E(i, j), 6) + " ", 3);
    }
    serialLogLn("", 3);
  }
  serialLogLn("Vector A:", 3);
  for (int i = 0; i < 2; i++) {
    serialLog(String(A(i), 6) + " ", 3);
  }
  serialLogLn("", 3);

  // Calculate the pseudo-inverse of E using the least squares approach
  BLA::Matrix<2, 2> E_transpose = ~E; // Transpose of E
  BLA::Matrix<2, 2> E_pseudo_inv = Inverse(E_transpose * E) * E_transpose;

  // Log the pseudo-inverse of the matrix E
  serialLogLn("Pseudo-Inverse Matrix E:", 3);
  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 2; j++) {
      serialLog(String(E_pseudo_inv(i, j)) + " ", 3);
    }
    serialLogLn("", 3);
  }

  // Calculate the concentrations using the least squares approach
  C = E_pseudo_inv * A;

  // Assign the results to the output variables
  c_A = C(0);
  c_B = C(1);
}
  
  /***      !!!! THIS IS A BAD MATRIX IMPLEMENTATION !!!!      ****/
  /*
  // Create the matrix E and vector A
  BLA::Matrix<2, 2> E;
  BLA::Matrix<2> A;
  BLA::Matrix<2> C;

  // Initialize the matrix E and vector A to zero
  E.Fill(0);
  A.Fill(0);

  // Initialize the matrix E and vector A
  for (int i = 2; i < numWavelengths; i++) {
      // Skip invalid values
      if (!includeWavelength[i] ||K_A[i] <= 0 || K_B[i] <= 0) {
          continue;
      }

      E(0, 0) += K_A[i] * K_A[i];
      E(0, 1) += K_A[i] * K_B[i];
      E(1, 0) += K_B[i] * K_A[i];
      E(1, 1) += K_B[i] * K_B[i];
      A(0) += K_A[i] * A_S[i];
      A(1) += K_B[i] * A_S[i];
  }

  // Log the matrix E and vector A
  serialLogLn("Matrix E:", 3);
  for (int i = 0; i < 2; i++) {
      for (int j = 0; j < 2; j++) {
          serialLog(String(E(i, j), 6) + " ", 3);
      }
      serialLogLn("", 3);
  }
  serialLogLn("Vector A:", 3);
  for (int i = 0; i < 2; i++) {
      serialLog(String(A(i), 6) + " ", 3);
  }
  serialLogLn("", 3);

  // Check if the matrix E is invertible
  float determinant = E(0, 0) * E(1, 1) - E(0, 1) * E(1, 0);
  if (determinant == 0) {
      serialLogLn("Error: Matrix E is not invertible.", 1);
      c_A = 0;
      c_B = 0;
      return;
  }

  // Calculate the pseudo-inverse of E
  BLA::Matrix<2, 2> E_inv = Inverse(E);

  // Log the inverse of the matrix E
  serialLogLn("Inverse Matrix E:", 3);
  for (int i = 0; i < 2; i++) {
      for (int j = 0; j < 2; j++) {
          serialLog(String(E_inv(i, j)) + " ", 3);
      }
      serialLogLn("", 3);
  }

  // Calculate the concentrations
  C = E_inv * A;

  // Assign the results to the output variables
  c_A = C(0);
  c_B = C(1);
}*/

//---
// Legacy for comparing
void calculateSampleConcentration(float* I_0, float* I_S, float* K) {
  float tempC[N_WAVELENGTHS];
  // Calculate the concentration using Beer-Lambert law
  for (int wavelength = 0; wavelength < N_WAVELENGTHS; wavelength++) {
    tempC[wavelength] = (log10(I_0[wavelength]/I_S[wavelength])) / K[wavelength];
  }

  #if VERBOSITY >= 3
    Serial.print("Sample Concentration: ");
    for (int i = 0; i < N_WAVELENGTHS; i++) {
      Serial.print(tempC[i]);
      if(i != N_WAVELENGTHS-1)
        Serial.print(", ");
      else
        Serial.println("");
    }
  #endif
}


/***********************************************************************************************************/
/****                                               MISC                                                ****/
/***********************************************************************************************************/

/**
 * This function exits once the buttonPressed flag is high.
 */
void waitForButton() {
  buttonPressed = false;
  while (!buttonPressed);
  buttonPressed = false;
}

/**
 * This function prints an array on float numbers of any length to any precision
 */
template <size_t Size> void printFloatArray(float (&arr)[Size], int decimals) {
  for (int i = 0; i < Size; i++) {
    serialLog2(arr[i], decimals, 1); // Print float with 6 decimal places
    if(i < Size-1)
      serialLog(", ", 1);
  }
}