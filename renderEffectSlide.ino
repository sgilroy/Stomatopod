void renderEffectSlide(byte idx) {
  if(fxVars[idx][0] == 0) { // Initialize effect?
    gammaRespondsToForce = false;
    fxVars[idx][1] = random(maxHue + 1); // Random hue
    // Frame-to-frame increment (speed) -- may be positive or negative,
    // but magnitude shouldn't be so small as to be boring.
    fxVars[idx][3] = 1;
    // Reverse direction half the time.
//    if(random(2) == 0) fxVars[idx][3] = -fxVars[idx][3];
    fxVars[idx][5] = 0; // Current position
    fxVars[idx][0] = 1; // Effect initialized
    fxVars[idx][4] = subPixels * 8; // width
  }

  int hue = pickHue(fxVars[idx][1]);

  clearImage(idx);
  
  int x = fxVars[idx][5];
  int width = fxVars[idx][4];
  drawLine(idx, x, width, hue);
  
//  fxVars[idx][3] += fxVars[idx][6];
//  if (abs(fxVars[idx][6]) > 

  // velocity
  int v = map(frontFsrStepFraction, 0, fsrStepFractionMax, 0, 100);
  
//  if (!slaveMode)
//    fxVars[idx][3] = fixSin(720 * millis() / 10000 / 3) / 2;
//    
//  fxVars[idx][4] += fxVars[idx][3];
  fxVars[idx][5] += v;
  if (fxVars[idx][5] < 0)
  {
    fxVars[idx][5] += numPixels * subPixels;
  }
  fxVars[idx][5] %= numPixels * subPixels;
}

void drawLine(byte idx, int x, int width, int hue)
{
  long color, i;
  byte alpha = 0;
  
  byte *ptr;
  int xp = 0;
  // draw a line at position x that is width subpixels wide
  for(long i=x; i<x + width; i++) {
    alpha++;
    if ((i + 1) % subPixels == 0)
    {
      color = hsv2rgb(hue, 255, alpha * 255 / subPixels);
      xp = i / subPixels;
//      ptr = &imgData[idx][xp % numPixels * 3];
//      *ptr++ = color >> 16; *ptr++ = color >> 8; *ptr++ = color;
      setPixel(idx, xp % numPixels, color);
      alpha = 0;
    }
  }

  color = hsv2rgb(hue, 255, alpha * 255 / subPixels);
  setPixel(idx, (xp + 1) % numPixels, color);
}

void setPixel(byte idx, int xp, long color)
{
  if (idx >= 0 && idx < 3 && xp >= 0 && xp < numPixels * 3)
  {
    byte *ptr = &imgData[idx][xp * 3];
    *ptr++ = color >> 16; *ptr++ = color >> 8; *ptr++ = color;
  }
}

void clearImage(byte idx)
{
  byte *ptr = &imgData[idx][0];
  for(long i = 0; i < numPixels; i++) {
    *ptr++ = 0; *ptr++ = 0; *ptr++ = 0;
  }
}
