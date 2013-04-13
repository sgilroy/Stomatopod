void renderEffectPressureVis(byte idx) {
  if(fxVars[idx][0] == 0) { // Initialize effect?
    gammaRespondsToForce = false;
    fxVars[idx][1] = random(maxHue + 1); // Random hue
    // Frame-to-frame increment (speed) -- may be positive or negative,
    // but magnitude shouldn't be so small as to be boring.  It's generally
    // still less than a full pixel per frame, making motion very smooth.
    fxVars[idx][3] = 4;
    // Reverse direction half the time.
    if(random(2) == 0) fxVars[idx][3] = -fxVars[idx][3];
    fxVars[idx][4] = 0; // Current position
    fxVars[idx][5] = 720 * 8 / numPixels; // wave period (width)
    fxVars[idx][0] = 1; // Effect initialized
  }

  byte *ptr = &imgData[idx][0];
  int alpha;
  int halfPeriod = fxVars[idx][5] / 2;
  int distance;
  long color;
  long hue = pickHue(fxVars[idx][1]);
  for(long i=0; i<numPixels; i++) {
    alpha = getPointChaseAlpha(idx, (i + frontOffset) % numPixels, halfPeriod) + getPointChaseAlpha(idx, (numPixels - 1 - i + frontOffset) % numPixels, halfPeriod);
    if (alpha > 255) alpha = 255;
    
    // Peaks of sine wave are white, troughs are black, mid-range
    // values are pure hue (100% saturated).

    color = hsv2rgb(hue, 255, alpha);
    *ptr++ = color >> 16; *ptr++ = color >> 8; *ptr++ = color;
  }

  // max force = half way around (360 half degrees)
  if (!slaveMode)
    fxVars[idx][4] = map(frontFsrStepFraction, 0, frontFsrStepFraction + backFsrStepFraction, 0, 360);
}

