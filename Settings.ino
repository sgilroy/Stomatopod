bool loadSettings() {
  EEPROM.readBlock(settingsAddress, settings);
//  Serial << "loadSettings complete -------------" << endl;
//  printSettings();
  return (String(settings.version) == currentSettingsVersion);
//  return false;
}

void printSettings() {
  Serial << "  settingsLoaded = " << settingsLoaded << endl;
  Serial << "  initialEffectQueued = " << initialEffectQueued << endl;
  Serial << "  tCounter = " << tCounter << endl;
  Serial << "  droppedFrames = " << droppedFrames << endl;
  Serial << "  version = " << settings.version << endl;
  Serial << "  slaveMode = " << settings.slaveMode << endl;
  Serial << "  effectIndex = " << settings.effectIndex << endl;
  Serial << "  autoTransition = " << settings.autoTransition << endl;
}

void saveSettings() {
  currentSettingsVersion.toCharArray(settings.version, sizeof(settings.version));
  settings.slaveMode = slaveMode;
  settings.autoTransition = autoTransition;
  
  if (tCounter >= 0) {
    byte frontImgIdx = 1 - backImgIdx;
    settings.effectIndex = fxIdx[frontImgIdx];
  }
  else
    settings.effectIndex = fxIdx[backImgIdx];
    
  EEPROM.updateBlock(settingsAddress, settings);
  #if debugSettings
    Serial << "saveSettings complete -------------" << endl;
    printSettings();
  #endif
}

