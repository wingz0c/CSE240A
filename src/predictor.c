//========================================================//
//  predictor.c                                           //
//  Source file for the Branch Predictor                  //
//                                                        //
//  Implement the various branch predictors below as      //
//  described in the README                               //
//========================================================//
#include <stdio.h>
#include <stdbool.h> 
#include "predictor.h"
#include <math.h>

//
// TODO:Student Information
//
const char *studentName = "Ho Chiu";
const char *studentID   = "A15346559";
const char *email       = "h8chiu@ucsd.edu";

//------------------------------------//
//      Predictor Configuration       //
//------------------------------------//
#define TABLE_SIZE 1024 // Determined by threshold and history length
#define THRESHOLD  127 // Floor(1.93 * HIST_LEN + 14) as specified in paper
#define HIST_LEN   59 // Optimal value as determined by paper

// Handy Global for use in output routines
const char *bpName[5] = { "Static", "Gshare",
                          "Tournament","Tournament2", "Custom" };

int ghistoryBits; // Number of bits used for Global History
int lhistoryBits; // Number of bits used for Local History
int pcIndexBits;  // Number of bits used for PC index
int bpType;       // Branch Prediction Type
int verbose;

//------------------------------------//
//      Predictor Data Structures     //
//------------------------------------//

//
//TODO: Add your own Branch Predictor data structures here

// 1st - gShare
// 2nd - Tournament - Local + Global
// 3rd - Custom - Local + gShare

uint32_t ghist;
uint32_t gmask;
uint32_t lmask;
uint32_t pcmask;

//gshare and custom
uint32_t *gs_pht;

//tournament and custom
uint32_t *local_bht;
uint32_t *local_pht;
uint32_t *global_pht;
uint32_t *choice_pht;

//perceptron
uint32_t *ghr;           // global history register
uint32_t perceptronSteps;
uint32_t perceptronTable[TABLE_SIZE][HIST_LEN + 1]; //table of perceptrons

//------------------------------------//
//        Predictor Functions         //
//------------------------------------//

uint32_t make_mask(uint32_t size)
{
  uint32_t mmask = 0;
  for(int i=0;i<size;i++)
  {
    mmask = mmask | 1<<i;
  }
  return mmask;
}

uint32_t HashPC(uint32_t pc){
	
	// Hash the PC so that it can be used as an index for the perceptron table.
	uint32_t PCend = pc % TABLE_SIZE;
	uint32_t ghrend = ((unsigned long)*ghr) % TABLE_SIZE;
	return PCend ^ ghrend;
}
// Initialize the predictor
//
void
init_predictor()
{
  //
  //TODO: Initialize Branch Predictor Data Structures
  //
  int size;
  unsigned long psize;

  ghist = 0;
  if(bpType==TOURNAMENT2)
  {
    ghistoryBits = 13; // Number of bits used for Global History
    lhistoryBits = 11; // Number of bits used for Local History
    pcIndexBits = 11; // Number of bits used for PC index
    // total size of predictor =  2^13 x 2  (Gshare PHT)
    //                          + 2^13 x 2  (Choice PHT)
    //                          + 2^11 x 2  (Local PHT)
    //                          + 2^11 x 11 (Local BHT)
    //                          = 59392 bits < 64000 + 256 bits
  }
  gmask = make_mask(ghistoryBits);
  lmask = make_mask(lhistoryBits);
  pcmask = make_mask(pcIndexBits);

  switch(bpType) {
    case GSHARE:
      size = 1<<ghistoryBits;
      gs_pht = (uint32_t*) malloc(sizeof(uint32_t)*size);
      for(int i=0;i<size;i++)
      {
        gs_pht[i] = 1;
      }

    case TOURNAMENT:
      // Local BHT
      size = 1<<pcIndexBits;
      local_bht = (uint32_t*) malloc(sizeof(uint32_t)*size);
      for(int i=0;i<size;i++)
      {
        local_bht[i] = 0;
      }
      // Local PHT
      size = 1<<lhistoryBits;
      local_pht = (uint32_t*) malloc(sizeof(uint32_t)*size);
      for(int i=0;i<size;i++)
      {
        local_pht[i] = 1;
      }
      // Global PHT
      size = 1<<ghistoryBits;
      global_pht = (uint32_t*) malloc(sizeof(uint32_t)*size);
      for(int i=0;i<size;i++)
      {
        global_pht[i] = 1;
      }

      // Choice PHT
      size = 1<<ghistoryBits;
      choice_pht = (uint32_t*) malloc(sizeof(uint32_t)*size);
      for(int i=0;i<size;i++)
      {
        choice_pht[i] = 2;
      }

    case  TOURNAMENT2:
      // Local BHT
      size = 1<<pcIndexBits;
      local_bht = (uint32_t*) malloc(sizeof(uint32_t)*size);
      for(int i=0;i<size;i++)
      {
        local_bht[i] = 0;
      }
      // Local PHT
      size = 1<<lhistoryBits;
      local_pht = (uint32_t*) malloc(sizeof(uint32_t)*size);
      for(int i=0;i<size;i++)
      {
        local_pht[i] = 1;
      }
      // Global PHT
      size = 1<<ghistoryBits;
      global_pht = (uint32_t*) malloc(sizeof(uint32_t)*size);
      for(int i=0;i<size;i++)
      {
        global_pht[i] = 1;
      }

      // Choice PHT
      size = 1<<ghistoryBits;
      choice_pht = (uint32_t*) malloc(sizeof(uint32_t)*size);
      for(int i=0;i<size;i++)
      {
        choice_pht[i] = 2;
      }

    case CUSTOM:
    // Given the history length and table size, construct the ghr
    // and perceptron table.
      perceptronSteps = 0;
      psize = pow(2,59);
      ghr = (uint32_t*) malloc(sizeof(uint32_t)*size);

      // Initialize each entry in the perceptron table to a value of
      // zero. Initialize number of steps executed for each perceptron to zero.

      for(uint32_t i=0; i < TABLE_SIZE; i++){
        for(uint32_t j=0; j < HIST_LEN; j++){
          perceptronTable[i][j] = 0;
        }
      }
  }      
}

// Make a prediction for conditional branch instruction at PC 'pc'
// Returning TAKEN indicates a prediction of taken; returning NOTTAKEN
// indicates a prediction of not taken
//
uint8_t make_prediction(uint32_t pc)
{
  //
  //TODO: Implement prediction scheme
  //

  uint32_t prediction;

  // gshare and custom
  uint32_t pcbits;
  uint32_t histbits;
  uint32_t index;
  
  //tournament and custom
  uint32_t choice;
  uint32_t pcidx;
  uint32_t lhist;
  uint32_t ghistbits;
  uint32_t perceptronIndex;
  // Make a prediction based on the bpType
  switch (bpType) {
    case STATIC:
      return TAKEN;

    case GSHARE:
      pcbits = pc & gmask;
      histbits = ghist & gmask;
      index = histbits ^ pcbits;
      prediction = gs_pht[index];
      if(prediction>1)
        return TAKEN;
      else
        return NOTTAKEN;

    case TOURNAMENT:
      ghistbits = gmask & ghist;
      choice = choice_pht[ghistbits];
      if(choice<2)
      {
        pcidx = pcmask & pc;
        lhist = lmask & local_bht[pcidx];
        prediction = local_pht[lhist];
      }
      else
      {
        prediction = global_pht[ghistbits];
      }
      if(prediction>1)
        return TAKEN;
      else
        return NOTTAKEN;

    case TOURNAMENT2:
      pcbits = pc & gmask;
      histbits = ghist & gmask;
      index = histbits ^ pcbits;
      choice = choice_pht[index];
      if(choice<2)
      {
        pcidx = pcmask & pc;
        lhist = lmask & local_bht[pcidx];
        prediction = local_pht[lhist];
      }
      else
      {
        prediction = global_pht[index];
      }
      if(prediction>1)
        return TAKEN;
      else
        return NOTTAKEN;
    
    case CUSTOM:

      perceptronIndex = HashPC(pc);
      prediction = 0;
      // Calculate prediction based on selected perceptron and global history.
      // First add the bias, then all other weights.

      prediction += perceptronTable[perceptronIndex][0]; 
      for(uint32_t i=1; i < HIST_LEN + 1; i++){
        // If history bit is taken, add the weight to the prediction.
        // Else, subtract the weight.
        if(ghr[i - 1] == 1){
          prediction += perceptronTable[perceptronIndex][i];}
        else{
          prediction -= perceptronTable[perceptronIndex][i];}
      }

      // Update perceptron steps to absolute value of the prediction.

      perceptronSteps = abs(prediction);

      // If the prediction is negative, predict not taken. If it is positive,
      // predict taken. Assume zero is positive.
    
    if(prediction >= 0){
        return TAKEN;}
    else{
        return NOTTAKEN;}  

    default:
      break;
  }

  // If there is not a compatable bpType then return NOTTAKEN
  return NOTTAKEN;
}

// Train the predictor the last executed branch at PC 'pc' and with
// outcome 'outcome' (true indicates that the branch was taken, false
// indicates that the branch was not taken)
//
void
train_predictor(uint32_t pc, uint8_t outcome)
{
  //
  //TODO: Implement Predictor training
  //

  // gshare and custom
  uint32_t pcbits;
  uint32_t histbits;
  uint32_t index;
  uint8_t prediction = make_prediction(pc);

  // tournament and custom
  uint32_t ghistbits;
  uint32_t choice;
  uint32_t pcidx;
  uint32_t lhist;
  uint32_t lpred;
  uint32_t gpred;
  uint32_t perceptronIndex;
  bool resolveDir;
  bool predDir;

  switch(bpType) {
    
    case GSHARE:
      pcbits = pc & gmask;
      histbits = ghist & gmask;
      index = histbits ^ pcbits;
      if(outcome==TAKEN)
      {
        if(gs_pht[index]<3)
          gs_pht[index]++;
      }
      else
      {
        if(gs_pht[index]>0)
          gs_pht[index]--;
      }
      ghist = ghist<<1 | outcome;
      return;
    
    case TOURNAMENT:
      ghistbits = gmask & ghist;
      choice = choice_pht[ghistbits];
      pcidx = pcmask & pc;
      lhist = lmask & local_bht[pcidx];

      lpred = local_pht[lhist];
      if(lpred>1)
        lpred = TAKEN;
      else
        lpred = NOTTAKEN;

      gpred = global_pht[ghistbits];
      if(gpred>1)
        gpred = TAKEN;
      else
        gpred = NOTTAKEN;

      if(gpred==outcome && lpred!=outcome && choice_pht[ghistbits]!=3)
        choice_pht[ghistbits]++;
      else if(gpred!=outcome && lpred==outcome && choice_pht[ghistbits]!=0)
        choice_pht[ghistbits]--;
      if(outcome==TAKEN)
      {
        if(global_pht[ghistbits]!=3)
          global_pht[ghistbits]++;
        if(local_pht[lhist]!=3)
          local_pht[lhist]++;
      }
      else
      {
        if(global_pht[ghistbits]!=0)
          global_pht[ghistbits]--;
        if(local_pht[lhist]!=0)
          local_pht[lhist]--;
      }
      local_bht[pcidx] = ((local_bht[pcidx]<<1) | outcome) & lmask;
      ghist = ((ghist<<1) | outcome) & gmask;
      return;

    case TOURNAMENT2:
      pcbits = pc & gmask;
      histbits = ghist & gmask;
      index = histbits ^ pcbits;
      choice = choice_pht[index];
      pcidx = pcmask & pc;
      lhist = lmask & local_bht[pcidx];

      lpred = local_pht[lhist];
      if(lpred>1)
        lpred = TAKEN;
      else
        lpred = NOTTAKEN;

      gpred = global_pht[index];
      if(gpred>1)
        gpred = TAKEN;
      else
        gpred = NOTTAKEN;

      if(gpred==outcome && lpred!=outcome && choice_pht[index]!=3)
        choice_pht[index]++;
      else if(gpred!=outcome && lpred==outcome && choice_pht[index]!=0)
        choice_pht[index]--;
      if(outcome==TAKEN)
      {
        if(global_pht[index]!=3)
          global_pht[index]++;
        if(local_pht[lhist]!=3)
          local_pht[lhist]++;
      }
      else
      {
        if(global_pht[index]!=0)
          global_pht[index]--;
        if(local_pht[lhist]!=0)
          local_pht[lhist]--;
      }
      local_bht[pcidx] = local_bht[pcidx]<<1 | outcome;
      ghist = ghist<<1 | outcome;
      return;

    case CUSTOM:
      perceptronIndex = HashPC(pc);

      // Update the perceptron table entry only if the threshold has not been
      // reached or the predicted and true outcomes disagree. Update the bias first, then the weights.
      // Correct for overflow.

      if(resolveDir != predDir || perceptronSteps <= THRESHOLD){
        
        // If the branch was taken, increment the bias value. Else, decrement it.

        if(resolveDir == TAKEN){
          int32_t updateVal = perceptronTable[perceptronIndex][0] + 1;
          if (updateVal > 128){
            perceptronTable[perceptronIndex][0] = 128;}
          else{
            perceptronTable[perceptronIndex][0]++;}
        }
        else{
          int32_t updateVal = perceptronTable[perceptronIndex][0] - 1;
          if (updateVal < -128){
            perceptronTable[perceptronIndex][0] = -128;}
          else{
            perceptronTable[perceptronIndex][0]--;} 
        }
      }
      // Update the weights.

      for(uint32_t i = 1; i < HIST_LEN + 1; i++){
        // If the branch outcome matches the history bit, increment the weight value.
        // Else, decrement the weight value.

        if((resolveDir == TAKEN && ghr[i - 1] == 1) || (resolveDir == NOTTAKEN && ghr[i - 1] == 0)){
          int32_t updateVal = perceptronTable[perceptronIndex][i] + 1;
          if (updateVal > THRESHOLD){
          perceptronTable[perceptronIndex][i] = THRESHOLD;}
          else{
            perceptronTable[perceptronIndex][i]++;}
        }
        else{
          int32_t updateVal = perceptronTable[perceptronIndex][i] - 1;
          if (updateVal < THRESHOLD * (-1)){
            perceptronTable[perceptronIndex][i] = THRESHOLD * (-1);}
          else{
            perceptronTable[perceptronIndex][i]--;}
        }
      }	   

      // update the GHR by shifting left and setting new bit.
      *ghr = (*ghr << 1);
      if(resolveDir == TAKEN){
        *ghr++;}
      return;

    default:
      break;
  }
}