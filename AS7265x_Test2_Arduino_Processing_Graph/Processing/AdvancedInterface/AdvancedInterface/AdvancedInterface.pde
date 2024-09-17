import processing.serial.*;
import controlP5.*;

Thread serialReader;
PrintWriter output; // For writing data to a file

Serial myPort;
ControlP5 cp5;

/**** UI Elements ****/
DropdownList portsDropdown;
Button connectButton; // Button for Connect/Disconnect
Button logButton; // Start/Stop logging
Textfield filenameField;
//Toggle appendToggle; // Checkbox for Append mode

/**** Constants ****/
static final int nDataEntries = 18;
static final float[] wavelengths = {410, 435, 460, 485, 510, 535, 560, 585, 610, 645, 680, 705, 730, 760, 810, 860, 900, 940};

/**** Flags ****/
boolean isConnected = false; // Connection state
boolean isLogging = false; // Logging state

/**** Global Variables ****/
float[] processedData = new float[18];
float maxMeasuredValue = 0;
String filename = "data.csv"; // Default filename

/**** Main ****/
void setup() {
  size(1200, 600); // Set the size of the window

  cp5 = new ControlP5(this);
  cp5.setFont(createFont("Arial", 20)); // Increase the font size

  // Create a dropdown list for COM ports
  portsDropdown = cp5.addDropdownList("COMPortList")
                      .setPosition(20, 20)
                      .setSize(200, 500)
                      .setBarHeight(40)
                      .setItemHeight(40);

  // Populate the dropdown with available COM ports
  String[] portNames = Serial.list();
  for (int i = 0; i < portNames.length; i++) {
    portsDropdown.addItem(portNames[i], i);
  }

  // Create a Connect/Disconnect button
  connectButton = cp5.addButton("connectButton")
                      .setLabel("Connect")
                      .setPosition(250, 20)
                      .setSize(200, 40);

  connectButton.onRelease(new CallbackListener() {
    public void controlEvent(CallbackEvent theEvent) {
      toggleConnection();
    }
  });
  
  // Start/Stop Logging button
  logButton = cp5.addButton("logButton")
                      .setLabel("START LOG")
                      .setSize(200, 40)
                      .setPosition(width-20-cp5.get("logButton").getWidth(), 20);

  logButton.onRelease(new CallbackListener() {
    public void controlEvent(CallbackEvent theEvent) {
      toggleLogging();
    }
  });
  
  
  // Filename field
  filenameField = cp5.addTextfield("filenameField")
                     .setPosition(logButton.getPosition()[0]-logButton.getWidth()-20, logButton.getPosition()[1])
                     .setSize(200, 40)
                     .setText(filename);
  
  filenameField.getCaptionLabel().hide();

  // Append mode toggle (checkbox)
  //appendToggle = cp5.addToggle("appendToggle")
  //                  .setPosition(250, 70)
  //                  .setSize(50, 20)
  //                  .setValue(false)
  //                  .setMode(ControlP5.SWITCH);
}

void draw() {
  background(200); // Set a background color (optional)
  drawGraph(processedData, 20, 80, width-20, height-20);
}

// Processes the newly acquired data in a separate thread
void processData(String data) {
  processedData = inDataToArray(data);
  printData(data, processedData);
  
  if (isLogging && output != null) {
    for (int i = 0; i < processedData.length; i++) {
      output.print(processedData[i] + ","); // Write the raw data or processed data to the file
    }
    output.println();
  }
}

// This method is called automatically when an item in the dropdown list is selected
public void controlEvent(ControlEvent theEvent) {
  if (theEvent.isFrom(portsDropdown)) {
    // If a port is selected but we're already connected, disconnect first
    if (isConnected) {
      disconnect();
    }
  }
}

/*********************/

void toggleLogging() {
  println("Toggling logging. Flag isLogging: " + isLogging);
  
  isLogging = !isLogging;
  if (isLogging) {
    // Start logging
    logButton.setLabel("Stop Logging");
    filename = filenameField.getText();
    filenameField.lock(); // Disable the filename field
    // Open the file for writing, append if the toggle is checked
    //if (appendToggle.getState()) {
    //  output = createWriter(sketchPath(filename), true); // Append mode
    //} else {
      output = createWriter(sketchPath(filename)); // Overwrite mode
    //}
    println("Started logging");
  } else {
    // Stop logging
    logButton.setLabel("Start Logging");
    filenameField.unlock(); // Enable the filename field
    if (output != null) {
      output.flush(); // Writes the remaining data to the file
      output.close(); // Finishes the file
    }
    println("Stopped logging");
  }
}

void printData(String data, float[] processedData) {
  println("Received data: " + data);

  println("Processed Data: ");
  //for (float value : processedData {
  for(int i = 0; i < nDataEntries; i++){
    println(wavelengths[i] + "nm: " + processedData[i]);
  }
}

float[] inDataToArray(String data) {
  // Split the data by commas
  String[] parts = data.split(",");

  // Create an array to hold the processed values
  float[] processedData = new float[nDataEntries];

  // Check if the parts array has exactly 18 elements
  if (parts.length == nDataEntries) {
    // Try to parse each part as a float and store it in the processedData array
    try {
      for (int i = 0; i < parts.length; i++) {
        // Parse each string to a float and ensure it has at least two decimal places
        processedData[i] = Float.parseFloat(parts[i]);
      }
    } catch (NumberFormatException e) {
      // If any value is not a valid float, manually fill the array with zeros
      for (int i = 0; i < processedData.length; i++) {
        processedData[i] = 0;
      }
    }
  } else {
    // If there are not exactly 18 values, manually fill the array with zeros
    for (int i = 0; i < processedData.length; i++) {
      processedData[i] = 0;
    }
  }
  
  return processedData;
}

// Toggle the connection state when the button is pressed
void toggleConnection() {
  println("Toggling connection.");
  if (isConnected) {
    disconnect();
  } else {
    connect();
  }
}

// Connect to the selected COM port
void connect() {
  int selectedPortIndex = (int)portsDropdown.getValue();
  if (selectedPortIndex >= 0) {
    myPort = new Serial(this, Serial.list()[selectedPortIndex], 115200);
    isConnected = true;
    connectButton.setLabel("Disconnect");

    // Start the serial reader thread
    serialReader = new Thread(new Runnable() {
      public void run() {
        while (Thread.currentThread() == serialReader) {
          if (myPort.available() > 0) {
            String incomingLine = myPort.readStringUntil('\n');
            if (incomingLine != null) {
              processData(incomingLine.trim());
            }
          }
          try {
            Thread.sleep(10); // Small delay to prevent high CPU usage
          } catch (InterruptedException e) {
            // Handle interruption
          }
        }
      }
    });
    serialReader.start();
  }
}

void disconnect() {
  if (myPort != null) {
    // Stop the serial reader thread
    if (serialReader != null) {
      Thread tmp = serialReader;
      serialReader = null;
      tmp.interrupt();
    }

    myPort.stop();
    myPort.dispose();
    myPort = null;
    isConnected = false;
    connectButton.setLabel("Connect");
  }
}
void drawGraph(float[] data, float x1, float y1, float x2, float y2) {
  // Update maxMeasuredValue with the highest value in the current spectrum
  for (int i = 0; i < data.length; i++) {
    if (data[i] > maxMeasuredValue) {
      maxMeasuredValue = data[i];
    }
  }

  // Calculate the width of each bar based on the number of data points and the window width
  float windowWidth = x2 - x1;
  float barWidth = windowWidth / data.length - 10; // Adjust the 10 to change spacing between bars

  for (int i = 0; i < data.length; i++) {
    // Dynamically map the spectral data to the height of the window based on maxMeasuredValue
    // Ensure maxMeasuredValue is at least 1 to avoid division by zero
    float safeMaxValue = max(maxMeasuredValue, 1);
    float barHeight = map(data[i], 0, safeMaxValue, 0, y2 - y1);

    // Assign a color based on the wavelength
    int[] rgb = wavelengthToRGB(wavelengths[i]);
    fill(rgb[0], rgb[1], rgb[2]); // Use the RGB color for each bar

    // Calculate the position and size of each bar within the specified window
    float xPos = x1 + i * (barWidth + 10); // Adjust the 10 to change spacing between bars
    float yPos = y2 - barHeight;

    // Draw each bar. The y position and height are adjusted so bars grow up within the window
    rect(xPos, yPos, barWidth, barHeight);
  }
}
//void drawGraph(float[] data) {
//  // Update maxMeasuredValue with the highest value in the current spectrum
//  for(int i = 0; i < data.length; i++) {
//    if (data[i] > maxMeasuredValue) {
//      maxMeasuredValue = data[i];
//    }
//  }

//  for(int i = 0; i < data.length; i++) {
//    // Dynamically map the spectral data to the height of the window based on maxMeasuredValue
//    // Ensure maxMeasuredValue is at least 1 to avoid division by zero
//    float safeMaxValue = max(maxMeasuredValue, 1);
//    float barHeight = map(data[i], 0, safeMaxValue, 0, height);

//    // Assign a color based on the wavelength
//    int[] rgb = wavelengthToRGB(wavelengths[i]);
//    fill(rgb[0], rgb[1], rgb[2]); // Use the RGB color for each bar

//    // Draw each bar. The y position and height need to be adjusted so bars grow up
//    rect(10 + i*30, height - barHeight, 20, barHeight);
//  }
//}

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
