// ---------------------------------------------------------------------------
//
//                  FSR
//
// ---------------------------------------------------------------------------

int readForce(int fsrPin) {
  int fsrStepFraction = 0;
  int fsrReading;     // the analog reading from the FSR resistor divider
  int fsrVoltage;     // the analog reading converted to voltage
  unsigned long fsrResistance;  // The voltage converted to resistance, can be very big so make "long"
  unsigned long fsrConductance; 
  long fsrForce;       // Finally, the resistance converted to force
  
  fsrReading = analogRead(fsrPin);  
  #if (debugFsrReading)
  {
    Serial.print("Analog reading = ");
    Serial.println(fsrReading);
  }
  #endif
  
  #if numFsrReadings > 1
  {
    long fsrReadingSum = 0;
    long fsrReadingAvg;
    fsrReadings[fsrReadingIndex] = fsrReading;
    fsrReadingIndex = (fsrReadingIndex + 1) % numFsrReadings;
    for (int i = 0; i < numFsrReadings; i++)
    {
      fsrReadingSum += fsrReadings[i];
    }
    fsrReadingAvg = fsrReadingSum / numFsrReadings;
    fsrReading = (int)fsrReadingAvg;
  
    #if (debugFsrReading)
    {
      Serial.print("Average reading = ");
      Serial.println(fsrReading);
    }
    #endif
  }
  #endif
 
  // analog voltage reading ranges from about 0 to 1023 which maps to 0V to 5V (= 5000mV)
  fsrVoltage = map(fsrReading, 0, 1023, 0, 5000);
  #if (debugFsrReading)
  {
    Serial.print("Voltage reading in mV = ");
    Serial.println(fsrVoltage);  
  }
  #endif
 
  if (fsrVoltage == 0) {
    #if (debugFsrReading)
    {
      Serial.println("No pressure");  
    }
    #endif
  } else {
    // The voltage = Vcc * R / (R + FSR) where R = 10K and Vcc = 5V
    // so FSR = ((Vcc - V) * R) / V        yay math!
    fsrResistance = 5000 - fsrVoltage;     // fsrVoltage is in millivolts so 5V = 5000mV
    fsrResistance *= 10000;                // 10K resistor
    fsrResistance /= fsrVoltage;
    #if (debugFsrReading)
    {
      Serial.print("FSR resistance in ohms = ");
      Serial.println(fsrResistance);
    }
    #endif
 
    fsrConductance = 1000000;           // we measure in micromhos so 
    fsrConductance /= fsrResistance;
    #if (debugFsrReading)
    {
      Serial.print("Conductance in microMhos: ");
      Serial.println(fsrConductance);
    }
    #endif
 
    // Use the two FSR guide graphs to approximate the force
//    if (fsrConductance <= 1000) {
//      fsrForce = fsrConductance / 80;
      fsrForce = fsrConductance / 20;
//    } else {
//      fsrForce = fsrConductance - 1000;
//      fsrForce /= 30;
//    }
    fsrStepFraction = fsrForce > fsrStepFractionMax ? fsrStepFractionMax : fsrForce;
    #if (debugFsrReading)
    {
      Serial.print("Force in Newtons: ");
      Serial.println(fsrForce);      
      Serial.print("Step Fraction: ");
      Serial.println(fsrStepFraction);      
  //    Serial.print("Step Fraction: ");
  //    Serial.println(fsrStepFraction);      
      Serial << "ClickButtonFsr value: " << frontButton.fsrValue << endl;
      Serial << " btnFlick: " << frontButton.btnFlick << endl;
      Serial << " depressed: " << frontButton.depressed << endl;
    }
    #endif
  }
  #if (debugFsrReading)
  {
    Serial.println("--------------------");
  }
  #endif
  
  return fsrStepFraction;
}


