// fxVars[idx][2] is Number of repetitions (complete loops around color wheel);
//   any more than 4 per meter just looks too chaotic.
//   Store as distance around complete belt in half-degree units:
//   example: fxVars[idx][2] = (1 + random(4 * ((numPixels + 31) / 32))) * 720;

//  fxVars[idx][4] is current position in half degrees (720 units around full circle)
//  fxVars[idx][5] is wave period (width) in half degrees

int getPointChaseAlpha(byte idx, long i, int halfPeriod)
{
  // position of pixel i in 1/2 degrees
  int offset = 720 * i / numPixels;

//    int theta = fxVars[idx][4] + offset;
//    int foo = fixSin(theta);

  // distance from start of sine wave to current pixel in half degrees
  int distance = (720 - offset - fxVars[idx][4]) % 720;
//  int distance = (720 + offset - fxVars[idx][4]);
  int foo = distance > halfPeriod || distance < 0 ? -127 : fixSin((distance * 360 / halfPeriod) - 180);
//  int foo = fixSin((distance * 360 / halfPeriod) - 180);
  return 127 + foo;
}
