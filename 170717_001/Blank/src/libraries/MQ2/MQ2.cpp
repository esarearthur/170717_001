#include "Arduino.h"
#include "MQ2.h"

float* __values = (float*)malloc(sizeof(float) * 3);

MQ2::MQ2(int pin) {
  _pin = pin;
}

void MQ2::begin() {
  Ro = MQCalibration();
  Serial.print("Ro: ");
  Serial.print(Ro);
  Serial.println(" kohm");
}

float* MQ2::read(bool print) {
  float rs_ro = MQRead() / Ro;
  lpg = MQGetGasPPM(rs_ro, GAS_LPG);
  co = MQGetGasPPM(rs_ro, GAS_CO);
  smoke = MQGetGasPPM(rs_ro, GAS_SMOKE);

  if (print) {
    Serial.print("LPG:");
    Serial.print(lpg);
    Serial.print("ppm");
    Serial.print("    ");
    Serial.print("CO:");
    Serial.print(co);
    Serial.print("ppm");
    Serial.print("    ");
    Serial.print("SMOKE:");
    Serial.print(smoke);
    Serial.print("ppm");
    Serial.print("\n");
  }

  lastReadTime = millis();
  __values[0] = lpg;
  __values[1] = co;
  __values[2] = smoke;
  return __values;
}

float MQ2::readLPG() {
  if (millis() < (lastReadTime + 10000) && lpg != 0) {
    return lpg;
  }
  else {
    return lpg = MQGetGasPercentage(MQRead() /10, GAS_LPG);
  }
}

float MQ2::readCO() {
  if (millis() < (lastReadTime + 10000) && co != 0) {
    return co;
  }
  else {
    return co = MQGetGasPercentage(MQRead() /10, GAS_CO);
  }
}

float MQ2::readSmoke() {
  if (millis() < (lastReadTime + 10000) && smoke != 0) {
    return smoke;
  }
  else {
    return smoke = MQGetGasPercentage(MQRead() /10, GAS_SMOKE);
  }
}

float MQ2::MQResistanceCalculation(int raw_adc) {
  float rl = (float) RL_VALUE;
  float MQ_Res = 1023/raw_adc;
  MQ_Res -= 1;
  MQ_Res *= rl;
  return MQ_Res;
  // return (((float)RL_VALUE*((1023/raw_adc)-1));
}

float MQ2::MQCalibration() {
  float val = 0;
  for (int i = 0; i < CALIBARAION_SAMPLE_TIMES; i++) {
    // take multiple samples
    val += MQResistanceCalculation(analogRead(_pin));
    delay(CALIBRATION_SAMPLE_INTERVAL);
  }
  val = val/CALIBARAION_SAMPLE_TIMES; // calculate the average value
  val = val/RO_CLEAN_AIR_FACTOR; // divided by RO_CLEAN_AIR_FACTOR yields the Ro
  // according to the chart in the datasheet
  return val;
}

float MQ2::MQRead() {
  int i;
  float rs = 0;
  int val = analogRead(_pin);
  for (i = 0; i < READ_SAMPLE_TIMES; i++) {
    rs += MQResistanceCalculation(val);
    delay(READ_SAMPLE_INTERVAL);
  }
  rs = rs/READ_SAMPLE_TIMES;
  return rs;
}

float MQ2::MQGetGasPercentage(float rs_ro_ratio, int gas_id) {
  float gasPercentage = 0;
  switch (gas_id) {
    case GAS_LPG:
      gasPercentage = MQGetPercentage(rs_ro_ratio, LPGCurve);
      break;
    case GAS_CO:
      gasPercentage = MQGetPercentage(rs_ro_ratio, COCurve);
      break;
    case GAS_SMOKE:
      gasPercentage = MQGetPercentage(rs_ro_ratio, SmokeCurve);
      break;
  }
  return gasPercentage;
}

float MQ2::MQGetGasPPM(float rs_ro_ratio, int gas_id) {
  float gasPPM = 0;
  switch (gas_id) {
    case GAS_LPG:
      gasPPM = MQGetPPM(rs_ro_ratio, _LPGCurve);
      break;
    case GAS_CO:
      gasPPM = MQGetPPM(rs_ro_ratio, _COCurve);
      break;
    case GAS_SMOKE:
      gasPPM = MQGetPPM(rs_ro_ratio, _SmokeCurve);
      break;
  }
  return gasPPM;
}

int MQ2::MQGetPercentage(float rs_ro_ratio, float * pcurve) {
  return(pow(10,(((log(rs_ro_ratio) - pcurve[1]) /pcurve[2]) + pcurve[0])));
}

float MQ2::MQGetPPM(float rs_ro_ratio, float * pcurve) {
  return pow(10, (pcurve[0] * log(rs_ro_ratio)) + pcurve[1]);
}
