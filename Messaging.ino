void serialSynchronize(byte frontImgIdx)
{
  if (!slaveMode)
  {
    txdata.message = SYNCHRONIZE_PATTERN;
    txdata.backImgIdx = backImgIdx;
    txdata.tCounter = tCounter;
  
    txdata.backImgFunctionIndex = fxIdx[backImgIdx];
    txdata.frontImgFunctionIndex = fxIdx[frontImgIdx];
    txdata.alphaFunctionIndex = fxIdx[2];
    txdata.transitionTime = transitionTime;
    
    for (int idx = 0; idx < SYNCHRONIZED_EFFECT_BUFFERS; idx++)
    {
      for (int i = 0; i < SYNCHRONIZED_EFFECT_VARIABLES; i++)
      {
        txdata.fxVars[idx][i] = fxVars[idx][i];
      }
    }
    
    //then we will go ahead and send that data out
    outTransfer.sendData();
  }
  
  //there's a loop here so that we run the recieve function more often then the 
  //transmit function. This is important due to the slight differences in 
  //the clock speed of different Arduinos. If we didn't do this, messages 
  //would build up in the buffer and appear to cause a delay.
  int receiveLoops = slaveMode ? 1 : 2;
  for(int i=0; i < receiveLoops; i++){
    if (inTransfer.receiveData())
    {
      if (rxdata.message == SYNCHRONIZE_PATTERN)
      {
        slaveMode = true;
        digitalWrite(13, HIGH);   // turn the LED on (HIGH is the voltage level)
        
        backImgIdx = rxdata.backImgIdx;
        tCounter = rxdata.tCounter;
      
        fxIdx[backImgIdx] = rxdata.backImgFunctionIndex;
        fxIdx[frontImgIdx] = rxdata.frontImgFunctionIndex;
        fxIdx[2] = rxdata.alphaFunctionIndex;
        transitionTime = rxdata.transitionTime;
        
        for (int idx = 0; idx < SYNCHRONIZED_EFFECT_BUFFERS; idx++)
        {
          for (int i = 0; i < SYNCHRONIZED_EFFECT_VARIABLES; i++)
          {
            fxVars[idx][i] = rxdata.fxVars[idx][i];
          }
        }
      }
    }
  }
}


