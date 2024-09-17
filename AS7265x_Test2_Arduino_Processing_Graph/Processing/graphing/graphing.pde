import processing.serial.*;

Serial myPort;  // Create object from Serial class
String val;      // Data received from the serial port
float[] spectrum = new float[18]; // Array to store the spectral data

void setup()
{
  size(720, 420); // Set the size of the window
  println("Available serial ports:");
  println(Serial.list()); // Print a list of available serial ports

  // Open the port that the Arduino is connected to (change COM4 to the correct port)
  //myPort = new Serial(this, Serial.list()[2], 115200); // Make sure to select the correct port
  myPort = new Serial(this, "COM3", 115200); // Make sure to select the correct port
  myPort.bufferUntil('\n'); // Set a specific byte to buffer until before calling serialEvent
}

float maxMeasuredValue = 0;
float[] wavelengths = {410, 435, 460, 485, 510, 535, 560, 585, 610, 645, 680, 705, 730, 760, 810, 860, 900, 940};


// Function to map wavelengths to RGB colors
int[] wavelengthToRGB(float wavelength) {
  float gamma = 0.8;
  float intensityMax = 255;
  float factor, red, green, blue;

  if ((wavelength >= 380) && (wavelength < 440)) {
    red = -(wavelength - 440) / (440 - 380);
    green = 0.0;
    blue = 1.0;
  } else if ((wavelength >= 440) && (wavelength < 490)) {
    red = 0.0;
    green = (wavelength - 440) / (490 - 440);
    blue = 1.0;
  } else if ((wavelength >= 490) && (wavelength < 510)) {
    red = 0.0;
    green = 1.0;
    blue = -(wavelength - 510) / (510 - 490);
  } else if ((wavelength >= 510) && (wavelength < 580)) {
    red = (wavelength - 510) / (580 - 510);
    green = 1.0;
    blue = 0.0;
  } else if ((wavelength >= 580) && (wavelength < 645)) {
    red = 1.0;
    green = -(wavelength - 645) / (645 - 580);
    blue = 0.0;
  } else if ((wavelength >= 645) && (wavelength <= 700)) {
    red = 1.0;
    green = 0.0;
    blue = 0.0;
  } else if ((wavelength > 700) && (wavelength <= 940)) {
    // Infrared range, we'll use shades of red
    red = 0.8 + (wavelength - 700) / (940 - 700) * 0.2;
    green = 0.0;
    blue = 0.0;
  } else {
    red = 0.0;
    green = 0.0;
    blue = 0.0;
  }

  // Let the intensity fall off near the vision limits
  if ((wavelength >= 380) && (wavelength < 420)) {
    factor = 0.3 + 0.7*(wavelength - 380) / (420 - 380);
  } else if ((wavelength >= 420) && (wavelength < 645)) {
    factor = 1.0;
  } else if ((wavelength >= 645) && (wavelength <= 700)) {
    factor = 0.3 + 0.7*(700 - wavelength) / (700 - 645);
  } else if ((wavelength > 700) && (wavelength <= 940)) {
    factor = 0.3; // Diminish intensity for infrared
  } else {
    factor = 0.0;
  }

  int r = (int)(red * factor * intensityMax);
  int g = (int)(green * factor * intensityMax);
  int b = (int)(blue * factor * intensityMax);

  return new int[]{r, g, b};
}

void draw() {
  background(255); // Set background to white

  // Update maxMeasuredValue with the highest value in the current spectrum
  for (int i = 0; i < spectrum.length; i++) {
    if (spectrum[i] > maxMeasuredValue) {
      maxMeasuredValue = spectrum[i];
    }
  }

  for (int i = 0; i < spectrum.length; i++) {
    // Dynamically map the spectral data to the height of the window based on maxMeasuredValue
    // Ensure maxMeasuredValue is at least 1 to avoid division by zero
    float safeMaxValue = max(maxMeasuredValue, 1);
    float barHeight = map(spectrum[i], 0, safeMaxValue, 0, height);

    // Assign a color based on the wavelength
    int[] rgb = wavelengthToRGB(wavelengths[i]);
    fill(rgb[0], rgb[1], rgb[2]); // Use the RGB color for each bar

    // Draw each bar. The y position and height need to be adjusted so bars grow up
    rect(10 + i*30, height - barHeight, 20, barHeight);
  }
}


void serialEvent(Serial myPort) {
  val = myPort.readStringUntil('\n'); // Read the string until a newline

  if (val != null) {
    //println("Raw data: " + val); // Print the raw data for debugging
    print(val); // Print the raw data for debugging
    val = trim(val); // Trim whitespace and newline
    String[] numbers = split(val, ','); // Split the string into an array at each comma
    //println("A: " + numbers[0]); // Print the raw data for debugging
    //println(numbers.length);
    
    if (numbers.length == 18) { // Check if we have 18 values, as expected
      for (int i = 0; i < numbers.length; i++) {
        try {
          //spectrum[i] = int(float(numbers[i])); // Convert each string to an integer and store in the array
          spectrum[i] = float(numbers[i]); // Convert each string to an integer and store in the array
        } catch (Exception e) {
          println("Error parsing number: " + numbers[i]);
        }
      }
      // After updating the spectrum array, print its contents
      //printArray(spectrum);
    }
  }
}
