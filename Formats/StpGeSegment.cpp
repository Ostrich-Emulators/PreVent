/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "StpGeSegment.h"

#include "CircularBuffer.h"
#include "Log.h"

#include <sstream>
#include <iomanip>

namespace FormatConverter{
  const int StpGeSegment::HEADER_SIZE = 64;
  const int StpGeSegment::VITALS_SIZE = 66;

  // <editor-fold defaultstate="collapsed" desc="block configs">
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_SKIP = BlockConfig::skip( );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_SKIP2 = BlockConfig::skip( 2 );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_SKIP4 = BlockConfig::skip( 4 );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_SKIP5 = BlockConfig::skip( 5 );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_SKIP6 = BlockConfig::skip( 6 );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_HR = BlockConfig::vital( "HR", "Bpm" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_PVC = BlockConfig::vital( "PVC", "Bpm" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_STI = BlockConfig::div10( "ST-I", "mm", 2, false );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_STII = BlockConfig::div10( "ST-II", "mm", 2, false );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_STIII = BlockConfig::div10( "ST-III", "mm", 2, false );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_STAVR = BlockConfig::div10( "ST-AVR", "mm", 2, false );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_STAVL = BlockConfig::div10( "ST-AVL", "mm", 2, false );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_STAVF = BlockConfig::div10( "ST-AVF", "mm", 2, false );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_STV = BlockConfig::div10( "ST-V", "mm", 1, false );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_STV1 = BlockConfig::div10( "ST-V1", "mm", 2, false );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_STV2 = BlockConfig::div10( "ST-V2", "mm", 2, false );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_STV3 = BlockConfig::div10( "ST-V3", "mm", 2, false );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_BT = BlockConfig::div10( "BT", "Deg C", 2 );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_IT = BlockConfig::div10( "IT", "Deg C", 2 );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_RESP = BlockConfig::vital( "RESP", "BrMin" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_APNEA = BlockConfig::vital( "APNEA", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_NBP_M = BlockConfig::vital( "NBP-M", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_NBP_S = BlockConfig::vital( "NBP-S", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_NBP_D = BlockConfig::vital( "NBP-D", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_CUFF = BlockConfig::vital( "CUFF", "mmHg", 2, false );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_AR1_M = BlockConfig::vital( "AR1-M", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_AR1_S = BlockConfig::vital( "AR1-S", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_AR1_D = BlockConfig::vital( "AR1-D", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_AR1_R = BlockConfig::vital( "AR1-R", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_AR2_M = BlockConfig::vital( "AR2-M", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_AR2_S = BlockConfig::vital( "AR2-S", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_AR2_D = BlockConfig::vital( "AR2-D", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_AR2_R = BlockConfig::vital( "AR2-R", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_AR3_M = BlockConfig::vital( "AR3-M", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_AR3_S = BlockConfig::vital( "AR3-S", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_AR3_D = BlockConfig::vital( "AR3-D", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_AR3_R = BlockConfig::vital( "AR3-R", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_AR4_M = BlockConfig::vital( "AR4-M", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_AR4_S = BlockConfig::vital( "AR4-S", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_AR4_D = BlockConfig::vital( "AR4-D", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_AR4_R = BlockConfig::vital( "AR4-R", "mmHg" );

  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_SPO2_P = BlockConfig::vital( "SPO2-%", "%" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_SPO2_R = BlockConfig::vital( "SPO2-R", "Bpm" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_SPO2M_P = BlockConfig::vital( "SPO2M-%", "%" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_SPO2M_R = BlockConfig::vital( "SPO2M-R", "Bpm" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_VENT = BlockConfig::vital( "Vent Rate", "BrMin" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_IN_HLD = BlockConfig::div10( "IN_HLD", "Sec", 2, false );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_PRS_SUP = BlockConfig::vital( "PRS-SUP", "cmH2O" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_INSP_TM = BlockConfig::div100( "INSP-TM", "Sec", 2, false );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_INSP_PC = BlockConfig::vital( "INSP-PC", "%" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_I_E = BlockConfig::div10( "I:E", "", 2, false );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_SET_PCP = BlockConfig::vital( "SET-PCP", "cmH2O" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_SET_IE = BlockConfig::div10( "SET-IE", "", 2, false );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_APRV_LO_T = BlockConfig::div10( "APRV-LO-T", "Sec", 2, false );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_APRV_HI_T = BlockConfig::div10( "APRV-HI-T", "Sec", 2, false );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_APRV_LO = BlockConfig::vital( "APRV-LO", "cmH20", 2, false );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_APRV_HI = BlockConfig::vital( "APRV-HI", "cmH20", 2, false );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_RESIS = BlockConfig::div10( "RESIS", "cmH2O/L/Sec", 2, false );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_MEAS_PEEP = BlockConfig::vital( "MEAS-PEEP", "cmH2O" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_INTR_PEEP = BlockConfig::vital( "INTR-PEEP", "cmH2O", 2, false );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_INSP_TV = BlockConfig::vital( "INSP-TV", "" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_COMP = BlockConfig::vital( "COMP", "ml/cmH20" );

  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_SPONT_MV = BlockConfig::vital( "SPONT-MV", "L/Min", 2, false );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_SPONT_R = BlockConfig::vital( "SPONT-R", "BrMin", 2, false );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_SET_TV = BlockConfig::vital( "SET-TV", "ml", 2, false );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_B_FLW = BlockConfig::vital( "B-FLW", "L/Min", 2, false );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_FLW_R = BlockConfig::vital( "FLW-R", "cmH20", 2, false );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_FLW_TRIG = BlockConfig::vital( "FLW-TRIG", "L/Min", 2, false );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_HF_FLW = BlockConfig::vital( "HF-FLW", "L/Min", 2, false );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_HF_R = BlockConfig::vital( "HF-R", "Sec", 2, false );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_HF_PRS = BlockConfig::vital( "HF-PRS", "cmH2O", 2, false );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_TMP_1 = BlockConfig::div10( "TMP-1", "Deg C" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_TMP_2 = BlockConfig::div10( "TMP-2", "Deg C" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_DELTA_TMP = BlockConfig::div10( "DELTA-TMP", "Deg C" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_LA1 = BlockConfig::vital( "LA1", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_LA2 = BlockConfig::vital( "LA2", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_LA3 = BlockConfig::vital( "LA3", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_LA4 = BlockConfig::vital( "LA4", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_CVP1 = BlockConfig::vital( "CVP1", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_CVP2 = BlockConfig::vital( "CVP2", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_CVP3 = BlockConfig::vital( "CVP3", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_CVP4 = BlockConfig::vital( "CVP4", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_CPP1 = BlockConfig::vital( "CPP1", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_CPP2 = BlockConfig::vital( "CPP2", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_CPP3 = BlockConfig::vital( "CPP3", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_CPP4 = BlockConfig::vital( "CPP4", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_ICP1 = BlockConfig::vital( "ICP1", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_ICP2 = BlockConfig::vital( "ICP2", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_ICP3 = BlockConfig::vital( "ICP3", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_ICP4 = BlockConfig::vital( "ICP4", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_SP1 = BlockConfig::vital( "SP1", "mmHg" );

  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_PA1_S = BlockConfig::vital( "PA1-S", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_PA1_D = BlockConfig::vital( "PA1-D", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_PA1_R = BlockConfig::vital( "PA1-R", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_PA1_M = BlockConfig::vital( "PA1-M", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_PA2_S = BlockConfig::vital( "PA2-S", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_PA2_D = BlockConfig::vital( "PA2-D", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_PA2_R = BlockConfig::vital( "PA2-R", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_PA2_M = BlockConfig::vital( "PA2-M", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_PA3_S = BlockConfig::vital( "PA3-S", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_PA3_D = BlockConfig::vital( "PA3-D", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_PA3_R = BlockConfig::vital( "PA3-R", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_PA3_M = BlockConfig::vital( "PA3-M", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_PA4_S = BlockConfig::vital( "PA4-S", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_PA4_D = BlockConfig::vital( "PA4-D", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_PA4_R = BlockConfig::vital( "PA4-R", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_PA4_M = BlockConfig::vital( "PA4-M", "mmHg" );

  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_FE1_S = BlockConfig::vital( "FE1-S", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_FE1_D = BlockConfig::vital( "FE1-D", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_FE1_R = BlockConfig::vital( "FE1-R", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_FE1_M = BlockConfig::vital( "FE1-M", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_FE2_S = BlockConfig::vital( "FE2-S", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_FE2_D = BlockConfig::vital( "FE2-D", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_FE2_R = BlockConfig::vital( "FE2-R", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_FE2_M = BlockConfig::vital( "FE2-M", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_FE3_S = BlockConfig::vital( "FE3-S", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_FE3_D = BlockConfig::vital( "FE3-D", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_FE3_R = BlockConfig::vital( "FE3-R", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_FE3_M = BlockConfig::vital( "FE3-M", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_FE4_S = BlockConfig::vital( "FE4-S", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_FE4_D = BlockConfig::vital( "FE4-D", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_FE4_R = BlockConfig::vital( "FE4-R", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_FE4_M = BlockConfig::vital( "FE4-M", "mmHg" );

  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_UAC1_S = BlockConfig::vital( "UAC1-S", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_UAC1_D = BlockConfig::vital( "UAC1-D", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_UAC1_R = BlockConfig::vital( "UAC1-R", "Bpm" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_UAC1_M = BlockConfig::vital( "UAC1-M", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_UAC2_S = BlockConfig::vital( "UAC2-S", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_UAC2_D = BlockConfig::vital( "UAC2-D", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_UAC2_R = BlockConfig::vital( "UAC2-R", "Bpm" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_UAC2_M = BlockConfig::vital( "UAC2-M", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_UAC3_S = BlockConfig::vital( "UAC3-S", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_UAC3_D = BlockConfig::vital( "UAC3-D", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_UAC3_R = BlockConfig::vital( "UAC3-R", "Bpm" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_UAC3_M = BlockConfig::vital( "UAC3-M", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_UAC4_S = BlockConfig::vital( "UAC4-S", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_UAC4_D = BlockConfig::vital( "UAC4-D", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_UAC4_R = BlockConfig::vital( "UAC4-R", "Bpm" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_UAC4_M = BlockConfig::vital( "UAC4-M", "mmHg" );

  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_PT_RR = BlockConfig::vital( "PT-RR", "BrMin" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_PEEP = BlockConfig::vital( "PEEP", "cmH20" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_MV = BlockConfig::div10( "MV", "L/min" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_Fi02 = BlockConfig::vital( "Fi02", "%" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_TV = BlockConfig::vital( "TV", "ml" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_PIP = BlockConfig::vital( "PIP", "cmH20" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_PPLAT = BlockConfig::vital( "PPLAT", "cmH20" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_MAWP = BlockConfig::vital( "MAWP", "cmH20" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_SENS = BlockConfig::div10( "SENS", "cmH20" );

  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_CO2_EX = BlockConfig::vital( "CO2-EX", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_CO2_IN = BlockConfig::vital( "CO2-IN", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_CO2_RR = BlockConfig::vital( "CO2-RR", "BrMin", 2, true );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_O2_EXP = BlockConfig::div10( "O2-EXP", "%" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_O2_INSP = BlockConfig::div10( "O2-INSP", "%" );

  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_RWOBVT = BlockConfig::vital( "rWOBVT", "J/L" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_RI_E = BlockConfig::vital( "rI:E", "" );
  //</editor-fold>

  // <editor-fold defaultstate="collapsed" desc="block configs">
  const std::map<StpGeSegment::VitalsBlock::Signal, std::map<int, std::vector<StpGeSegment::VitalsBlock::BlockConfig>>> StpGeSegment::VitalsBlock::LOOKUP = {
    { Signal::NBP,{
      { 0x0A, { BC_SKIP2, BC_NBP_M, BC_NBP_S,BC_NBP_D, BC_SKIP2, BC_CUFF } } }
    },
    { Signal::RESP,{
      { 0x08, { BC_SKIP2, BC_RESP, BC_APNEA } },
      { 0x09, { BC_SKIP2, BC_BT, BC_IT } },
      { 0x0C, { BC_SKIP2, BC_TMP_1, BC_TMP_2, BC_DELTA_TMP } } }
    },
    { Signal::TEMP,{
      { 0x0C, { BC_SKIP2, BC_TMP_1, BC_TMP_2, BC_DELTA_TMP } } }
    },
    { Signal::SPO2,{
      {0x0B, { BC_SKIP2, BC_SPO2_P, BC_SPO2_R } } }
    },
    { Signal::CO2,{
      {0x0B, { BC_SKIP2, BC_CO2_EX, BC_CO2_IN, BC_CO2_RR, BC_SKIP2, BC_O2_EXP, BC_O2_INSP  } },
      {0x0E, { BC_SKIP2, BC_CO2_EX, BC_CO2_IN, BC_CO2_RR, BC_SKIP2, BC_O2_EXP, BC_O2_INSP  } } }
    },
    { Signal::HR,{
      {0x01, { BC_SKIP2, BC_HR, BC_PVC } } }
    },
    {Signal::MSDR1,{
      {0x02, { BC_SKIP2, BC_AR1_M, BC_AR1_S, BC_AR1_D, BC_SKIP2, BC_AR1_R } },
      {0x03, { BC_SKIP2, BC_PA1_M, BC_PA1_S, BC_PA1_D, BC_SKIP2, BC_PA1_R } },
      {0x04, { BC_SKIP2, BC_LA1 } },
      {0x05, { BC_SKIP2, BC_CVP1 } },
      {0x06, { BC_SKIP2, BC_ICP1, BC_CPP1 } },
      {0x07, { BC_SKIP2, BC_SP1 } },
      {0x10, { BC_SKIP2, BC_UAC1_M, BC_UAC1_S, BC_UAC1_D, BC_SKIP2, BC_UAC1_R } },
      {0x12, { BC_SKIP2, BC_FE1_M, BC_FE1_S, BC_FE1_D, BC_SKIP2, BC_FE1_R } },
      {0x2A, { BC_SKIP2, BC_INSP_TV } } }
    },
    {Signal::MSDR2,{
      {0x02, { BC_SKIP2, BC_AR2_M, BC_AR2_S, BC_AR2_D, BC_SKIP2, BC_AR2_R } },
      {0x03, { BC_SKIP2, BC_PA2_M, BC_PA2_S, BC_PA2_D, BC_SKIP2, BC_PA2_R } },
      {0x04, { BC_SKIP2, BC_LA2 } },
      {0x05, { BC_SKIP2, BC_CVP2 } },
      {0x06, { BC_SKIP2, BC_ICP2, BC_CPP2 } },
      // {0x07, { BC_SKIP2, BC_SP2 } },
      {0x10, { BC_SKIP2, BC_UAC2_M, BC_UAC2_S, BC_UAC2_D, BC_SKIP2, BC_UAC2_R } },
      {0x12, { BC_SKIP2, BC_FE2_M, BC_FE2_S, BC_FE2_D, BC_SKIP2, BC_FE2_R } } }
      // {0x2A, { BC_SKIP2, BC_INSP_TV } } }
    },
    {Signal::MSDR3,{
      {0x02, { BC_SKIP2, BC_AR3_M, BC_AR3_S, BC_AR3_D, BC_SKIP2, BC_AR3_R } },
      {0x03, { BC_SKIP2, BC_PA3_M, BC_PA3_S, BC_PA3_D, BC_SKIP2, BC_PA3_R } },
      {0x04, { BC_SKIP2, BC_LA3 } },
      {0x05, { BC_SKIP2, BC_CVP3 } },
      {0x06, { BC_SKIP2, BC_ICP3, BC_CPP3 } },
      // {0x07, { BC_SKIP2, BC_SP2 } },
      {0x10, { BC_SKIP2, BC_UAC3_M, BC_UAC3_S, BC_UAC3_D, BC_SKIP2, BC_UAC3_R } },
      {0x12, { BC_SKIP2, BC_FE3_M, BC_FE3_S, BC_FE3_D, BC_SKIP2, BC_FE3_R } } }
      // {0x2A, { BC_SKIP2, BC_INSP_TV } } }
    },
    {Signal::MSDR4,{
      {0x02, { BC_SKIP2, BC_AR4_M, BC_AR4_S, BC_AR4_D, BC_SKIP2, BC_AR4_R } },
      {0x03, { BC_SKIP2, BC_PA4_M, BC_PA4_S, BC_PA4_D, BC_SKIP2, BC_PA4_R } },
      {0x04, { BC_SKIP2, BC_LA4 } },
      {0x05, { BC_SKIP2, BC_CVP4 } },
      {0x06, { BC_SKIP2, BC_ICP4, BC_CPP4 } },
      // {0x07, { BC_SKIP2, BC_SP2 } },
      {0x10, { BC_SKIP2, BC_UAC4_M, BC_UAC4_S, BC_UAC4_D, BC_SKIP2, BC_UAC4_R } },
      {0x12, { BC_SKIP2, BC_FE4_M, BC_FE4_S, BC_FE4_D, BC_SKIP2, BC_FE4_R } } }
      // {0x2A, { BC_SKIP2, BC_INSP_TV } } }
    },
    {Signal::ST1,{
      {0x0D, { BC_SKIP2, BC_STI, BC_STII, BC_STIII } } }
    },
    {Signal::STV,{
      {0x0D, { BC_SKIP2, BC_STV1, BC_STV2, BC_STV3 } } }
    },
    {Signal::NONE1,{
      {0x0D, {} } }
    },
    {Signal::STAVL,{
      {0x0D, { BC_SKIP2, BC_STAVR, BC_STAVL, BC_STAVF } } }
    },
    {Signal::RWOBVT,{
      {0x3C, { BC_SKIP2, BC_RWOBVT } } }
    },
    {Signal::RIE,{
      {0x3C, { BC_SKIP2, BC_RI_E } } }
    },
    {Signal::APRV,{
      {0x3C, { BC_SKIP2, BC_APRV_LO, BC_APRV_HI, BC_APRV_LO_T, BC_SKIP2, BC_APRV_HI_T, BC_COMP, BC_RESIS, BC_MEAS_PEEP, BC_INTR_PEEP, BC_SPONT_R } } }
    },
    {Signal::AR1,{
      {0x3C, { BC_SKIP2, BC_AR1_M, BC_AR1_S, BC_AR1_D, BC_SKIP2, BC_AR1_R } } }
    },
    {Signal::AR2,{
      {0x3C, { BC_SKIP2, BC_AR2_M, BC_AR2_S, BC_AR2_D, BC_SKIP2, BC_AR2_R } } }
    },
    {Signal::AR3,{
      {0x3C, { BC_SKIP2, BC_AR3_M, BC_AR3_S, BC_AR3_D, BC_SKIP2, BC_AR3_R } } }
    },
    {Signal::AR4,{
      {0x3C, { BC_SKIP2, BC_AR4_M, BC_AR4_S, BC_AR4_D, BC_SKIP2, BC_AR4_R } } }
    },
    {Signal::PEEP,{
      {0x3C, { BC_SKIP2, BC_PT_RR, BC_PEEP, BC_MV, BC_SKIP2, BC_Fi02, BC_TV, BC_PIP, BC_PPLAT, BC_MAWP, BC_SENS } } }
    },
    {Signal::FLOW,{
      {0x2A, { BC_SKIP2, BC_VENT, BC_FLW_R, BC_SKIP4, BC_IN_HLD, BC_SKIP2, BC_PRS_SUP, BC_INSP_TM, BC_INSP_PC, BC_I_E } } }
    },
    {Signal::HF,{
      {0x3C, { BC_SKIP2, BC_HF_FLW, BC_HF_R, BC_HF_PRS, BC_SPONT_MV, BC_SKIP2, BC_SET_TV, BC_SET_PCP, BC_SET_IE, BC_B_FLW, BC_FLW_TRIG } } }
    }
  };
  // </editor-fold>

  const unsigned int StpGeSegment::WaveFA0DBlock::READCOUNTS[] = { 1, 2, 4, 8, 16, 32, 64, 128 };

  StpGeSegment::StpGeSegment( Header h, const std::vector<VitalsBlock>& vitals,
      const std::vector<WaveFA0DBlock>& waves ) : header( h ), vitals( vitals ), waves( waves ) { }

  StpGeSegment::~StpGeSegment( ) { }

  StpGeSegment::Header::Header( unsigned long magic, dr_time t, const std::string& p )
      : time( t ), patient( p ), unity( 0 == magic ), magic( magic ) { }

  StpGeSegment::Header::Header( const Header& o )
      : time( o.time ), patient( o.patient ), unity( o.unity ), magic( o.magic ) { }

  StpGeSegment::Header StpGeSegment::Header::parse( const std::vector<unsigned char>& data ) {
    auto magic = readUInt4(data, 0);
    auto timestart = 18;
    time_t time = ( ( data[timestart + 1] << 24 ) | ( data[timestart] << 16 )
        | ( data[timestart + 3] << 8 ) | data[timestart + 2] );
    time *= 1000;

    // name is 32 bytes long
    const auto NAME_START = 24;
    const auto NAME_END = 56;
    std::vector<char> chars;
    chars.reserve( NAME_END - NAME_START );
    for ( size_t i = NAME_START; i < NAME_END && 0 != data[i]; i++ ) {
      chars.push_back( (char) data[i] );
    }
    auto namevec = std::string( chars.begin( ), chars.end( ) );

    return Header( magic, time, namevec );
  }

  StpGeSegment::VitalsBlock::VitalsBlock( MajorMode major, Signal s, int leed, unsigned int start,
      const std::vector<BlockConfig>& cfg ) : major( major), signal( s ), lead( leed ),
      datastart( start ), config( cfg ) { }

  StpGeSegment::VitalsBlock StpGeSegment::VitalsBlock::index( const std::vector<unsigned char>& data,
      unsigned long pos, GEParseError& errcode ) {
    const int major = data[pos];
    const int sigint = data[pos + 1];
    const int lead = data[pos + 64];

    auto sigs = std::vector{ NBP, RESP, TEMP, SPO2, CO2, HR, MSDR1, MSDR2, MSDR3, MSDR4, ST1, STV,
        NONE1, STAVL, RWOBVT, RIE, APRV, AR1, AR2, AR3, AR4, PEEP, FLOW, HF
    };

    if( major != MajorMode::STANDARD ) {
      throw std::runtime_error( "unknown major mode: " + major );
    }

    for ( auto& signal : sigs ) {
      if ( signal == sigint ) {
        Log::trace( ) << "new block: [" << std::dec << pos << " - " << pos + VITALS_SIZE << "); type: "
            << std::setfill( '0' ) << std::setw( 2 ) << std::hex << lead << " "
            << std::setfill( '0' ) << std::setw( 2 ) << std::hex << signal << std::endl;
        
        if( 1 == LOOKUP.count(signal)){
          const auto& leadmap = LOOKUP.at(signal);

          if ( 0 == leadmap.count( lead ) ) {
            if ( 1 == leadmap.size( ) ) {
              for( auto& mm : leadmap ){
                Log::warn( ) << "unrecognized lead: "
                    << std::setfill( '0' ) << std::setw( 2 ) << std::hex << lead << "; using "
                    << signal << ":"
                    << std::setfill( '0' ) << std::setw( 2 ) << std::hex << mm.first<<" instead"
                    << std::endl;
                return VitalsBlock( MajorMode::STANDARD, signal, lead, pos + 2, mm.second );
              }
            }            
          }
          else {
            return VitalsBlock( MajorMode::STANDARD, signal, lead, pos + 2, leadmap.at( lead ) );
          }
        }
      }
    }

    std::stringstream ss;
    ss << "unhandled block: " << std::setfill( '0' ) << std::setw( 2 ) << std::hex
        << lead << " " << std::setfill( '0' ) << std::setw( 2 ) << std::hex << sigint
        << " starting at " << std::dec << pos;
    throw std::runtime_error( ss.str( ) );
  }

  int StpGeSegment::VitalsBlock::hex( ) const {
    return (lead >> 8 | ( signal & 0xFF ) );
  }

  StpGeSegment::WaveFA0DBlock::WaveFA0DBlock( int seq, unsigned long start, unsigned long end,
      const std::vector<WaveFormIndex>& data ) : wavedata( data ), sequence( seq ),
      startpos( start ), endpos( end ) { }

  StpGeSegment::WaveFA0DBlock::WaveFA0DBlock::WaveFormIndex::WaveFormIndex( unsigned int waveid,
      unsigned int vcnt, unsigned int start ) : waveid( waveid ), valcount( vcnt ),
      datastart( start ) { }


  StpGeSegment::WaveFA0DBlock StpGeSegment::WaveFA0DBlock::index( const std::vector<unsigned char>& data,
      unsigned long& start, const std::vector<StpGeSegment::VitalsBlock>& vitals,
      GEParseError& errcode ) {
    // waves block always starts with 0x04, then a sequence number, then two 0x00 bytes
    // followed by wave data. wave data comes in X-byte blocks of:
    // <wave form id> <sample count> <data>
    // wave data sections usually end with 0XFA0D followed by some indexing data whose
    // size depends on the monitor model (either 33 or 49 bytes). However, sometimes, the wave
    // sections end abruptly, so we have to ensure we don't walk past the end of the data array
    Log::trace( ) << "wave block at " << std::dec << start << std::endl;
    auto wstart = start;
    auto seq = readUInt( data, ++start );
    start += 3; // (2) 0x00 bytes, and position read head over wave id

    auto indexes = std::vector<WaveFormIndex>();
    auto waveid = readUInt(data, start++);
    auto countbyte = readUInt(data, start++);

    while ( !( 0xFA == waveid && 0x0D == countbyte ) && start < data.size() ) {
      // Log::trace( ) << "wave bytes: 0x" << std::hex << std::setw( 2 ) << waveid << " "
      //     << std::setw( 2 ) << countbyte << " at " << std::dec << ( start - 2 ) << std::endl;

      // usually, we get 0x0B for the countbyte, but sometimes we get 0x3B. I don't
      // know what that means, but the B part seems to be the only thing that matters
      // so zero out the most significant bits
      unsigned int shifty = ( countbyte & 0b00000111 );
      if ( 0 == shifty || shifty > 7 ) {
        errcode = GEParseError::WAVE_INCONSISTENT_VALCOUNT;
        throw std::runtime_error( "Inconsistent wave information...file is corrupt?" );
      }
      unsigned int valstoread = READCOUNTS[shifty - 1];

      if ( valstoread > 4 ) {
        std::stringstream ss;
        ss << "don't really think we want to read " << valstoread << " values for wave/count:"
            << std::setfill( '0' ) << std::setw( 2 ) << std::hex << waveid << " "
            << std::setfill( '0' ) << std::setw( 2 ) << std::hex << countbyte;
        std::string ex = ss.str( );
        errcode = GEParseError::WAVE_TOO_MANY_VALUES;
        throw std::runtime_error( ex );
      }

      auto dstart = start;
      // skip the data bytes
      start += valstoread * 2;
      if ( start < data.size( ) ) {
        indexes.push_back( WaveFormIndex( waveid, valstoread, dstart ) );

        if ( start + 2 < data.size( ) ) {
          waveid = readUInt( data, start++ );
          countbyte = readUInt( data, start++ );

          // Error correction code: we can get spurious 0x00s in the wave data on occasion
          // if our waveid is 00, skip it and try again
          if ( 0 == waveid && countbyte > 0x04 ) {
            Log::warn( ) << "improper wave id detected...ECC code enabled" << std::endl;
            waveid = countbyte;
            countbyte = readUInt( data, start++ );
          }
          else if ( 0x04 == waveid ) {
            start -= 2;
            Log::warn( ) << "unexpected end of wave data at " << start << std::endl;
            break;
          }
        }
      }
    }

    // hit the 0xFA0D byte (or an unexpected 0x04)...count forward to the next 0x04
    while ( waveid != 0x04 && start + 1 < data.size( ) ) {
      waveid = readUInt( data, ++start );
    }

    return WaveFA0DBlock( seq, wstart, start, indexes );
  }

  std::unique_ptr<StpGeSegment> StpGeSegment::index( const std::vector<unsigned char>& data,
      bool skipwaves, GEParseError& err ) {

//    Log::error( ) << "indexing " << data.size( ) << " bytes" << std::endl;
//    for ( auto i = 0U; i < data.size( ); i++ ) {
//      if ( 0 == i % 32 ) {
//        Log::error( ) << std::endl;
//      }
//      auto x = readUInt( data, i );
//      Log::error( ) << std::setfill( '0' ) << std::setw( 2 ) << std::hex << x << " ";
//    }
//    Log::error( ) << std::endl;
    Header h = Header::parse( data );

    auto wavestart = 60LU + readUInt2( data, 58 );
    auto vitstart = 0LU + VITALS_SIZE;

    err = NO_ERROR;
    auto vitals = std::vector<VitalsBlock>( );
    while ( vitstart < wavestart ) {
      if ( vitstart + VITALS_SIZE > wavestart ) {
        err = VITALSBLOCK_OVERFLOW;
      }
      try {
        auto v = VitalsBlock::index( data, vitstart, err );
        vitals.push_back( v );
      }
      catch ( std::runtime_error& x ) {
        Log::error( ) << x.what( ) << std::endl;
        err = UNKNOWN_VITALSTYPE;
      }
      vitstart += VITALS_SIZE + 2; // 2 blank bytes at the end of every segment
    }

    auto waves = std::vector<WaveFA0DBlock>( );
    if ( !skipwaves ) {
      // +8 here because if we have a wave block, it will contain the 0x04, sequence, 2 0x00s,
      // wave id, and byte counter, plus at least one value
      try {
        while ( wavestart + 8 < data.size( ) ) {
          WaveFA0DBlock w = WaveFA0DBlock::index( data, wavestart, vitals, err );
          waves.push_back( w );
        }
      }
      catch ( std::runtime_error& x ) {
        Log::error( ) << x.what( ) << std::endl;
      }
    }

    return std::unique_ptr<StpGeSegment>( new StpGeSegment( h, vitals, waves ) );
  }

  unsigned int StpGeSegment::readUInt( const std::vector<unsigned char>& rawdata, unsigned long start ) {
    return rawdata[start];
  }

  unsigned int StpGeSegment::readUInt2( const std::vector<unsigned char>& rawdata, unsigned long start ) {
    unsigned char b1 = rawdata[start];
    unsigned char b2 = rawdata[start + 1];
    return ( b1 << 8 | b2 );
  }

  unsigned long StpGeSegment::readUInt4( const std::vector<unsigned char>& rawdata, unsigned long start ) {
    unsigned char b0 = rawdata[start];
    unsigned char b1 = rawdata[start + 1];
    unsigned char b2 = rawdata[start + 2];
    unsigned char b3 = rawdata[start + 3];
    return ( b0 << 24 | b1 << 16 | b2 << 8 | b3 );
  }

  int StpGeSegment::readInt( const std::vector<unsigned char>& rawdata, unsigned long start ) {
    return (char) rawdata[start];
  }

  int StpGeSegment::readInt2( const std::vector<unsigned char>& rawdata, unsigned long start ) {
    unsigned char b1 = rawdata[start];
    unsigned char b2 = rawdata[start + 1];

    short val = ( b1 << 8 | b2 );
    return val;
  }
}