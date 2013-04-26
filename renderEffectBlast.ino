void renderEffectBlast(byte idx) {
  if(fxVars[idx][0] == 0) { // Initialize effect?
    gammaRespondsToForce = false;
    fxVars[idx][1] = random(maxHue + 1); // Random hue
    // Frame-to-frame increment (speed) -- may be positive or negative,
    // but magnitude shouldn't be so small as to be boring.
    fxVars[idx][3] = 1;
    // Reverse direction half the time.
//    if(random(2) == 0) fxVars[idx][3] = -fxVars[idx][3];
//    fxVars[idx][4] = 0; // Current position
    fxVars[idx][0] = 1; // Effect initialized
    fxVars[idx][4] = subPixels * 4; // width
  }

  int hue = pickHue(fxVars[idx][1]);

  clearImage(idx);
  
  const int centeringOffset = 7;
  int width = fxVars[idx][4];
//  int x = map(frontFsrStepFraction, 0, fsrStepFractionMax, 0, (numPixels - width) * subPixels / 2) + subPixels * centeringOffset;

// enclosure R seems to use front as back and back as front

  int x = map(frontFsrStepFraction, 0, fsrStepFractionMax, 0, (numPixels) * subPixels / 2 - width) + subPixels * centeringOffset;
//  int x = map(backFsrStepFraction, 0, fsrStepFractionMax, 0, (numPixels) * subPixels / 2 - width) + subPixels * centeringOffset;
  drawLine(idx, x, width, hue);
  drawLine(idx, (numPixels + centeringOffset) * subPixels - x, width, hue);
}  

void renderEffectBlast2(byte idx) {
  if(fxVars[idx][0] == 0) { // Initialize effect?
    gammaRespondsToForce = false;
    fxVars[idx][1] = random(maxHue + 1); // Random hue
    // Frame-to-frame increment (speed) -- may be positive or negative,
    // but magnitude shouldn't be so small as to be boring.  It's generally
    // still less than a full pixel per frame, making motion very smooth.
//    fxVars[idx][3] = 1 + random(720) / numPixels;
//    fxVars[idx][3] = 1;
    fxVars[idx][3] = 4;
    // Reverse direction half the time.
    if(random(2) == 0) fxVars[idx][3] = -fxVars[idx][3];
    fxVars[idx][4] = 0; // Current position
    fxVars[idx][0] = 1; // Effect initialized
//    fxVars[idx][5] = 15 + random(360); // wave period
//    fxVars[idx][5] = 30 + random(150); // wave period (width)
    fxVars[idx][5] = 720 * 4 / numPixels; // wave period (width)
//    fxVars[idx][5] = 720 * 8 / numPixels; // wave period (width)
//    fxVars[idx][5] = random(720 * 2 / numPixels, 180); // wave period (width)
  }

  byte *ptr = &imgData[idx][0];
  int alpha;
  int halfPeriod = fxVars[idx][5] / 2;
  int distance;
  long color;
  long hue = pickHue(fxVars[idx][1]);
  for(long i=0; i<numPixels; i++) {
    alpha = getPointChaseAlpha(idx, (i + frontOffset) % numPixels, halfPeriod) + getPointChaseAlpha(idx, (numPixels - 1 - i + frontOffset) % numPixels, halfPeriod);
//    alpha = getPointChaseAlpha(idx, (i + frontOffset) % numPixels, halfPeriod);
    if (alpha > 255) alpha = 255;
    
    // Peaks of sine wave are white, troughs are black, mid-range
    // values are pure hue (100% saturated).

    color = hsv2rgb(hue, 255, alpha);
    *ptr++ = color >> 16; *ptr++ = color >> 8; *ptr++ = color;
  }
//  fxVars[idx][4] += fxVars[idx][3];
//  fxVars[idx][4] %= 720;

  // max force = half way around (360 half degrees)
  if (!slaveMode)
    fxVars[idx][4] = map(frontFsrStepFraction, 0, fsrStepFractionMax, 0, 360);
//    fxVars[idx][4] = map(backFsrStepFraction, 0, fsrStepFractionMax, 0, 360);
//  fxVars[idx][5] = map(fsrStepFraction, 720 / numPixels, fsrStepFractionMax, 0, 720 * 2);
}


