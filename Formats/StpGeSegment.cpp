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
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_STAVR = BlockConfig::div10( "ST-AVR", "mm", 1, false );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_STAVL = BlockConfig::div10( "ST-AVL", "mm", 1, false );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_STAVF = BlockConfig::div10( "ST-AVF", "mm", 1, false );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_STV = BlockConfig::div10( "ST-V", "mm", 1, false );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_STV1 = BlockConfig::div10( "ST-V1", "mm", 2, false );
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
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_AR2_M = BlockConfig::vital( "AR1-M", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_AR2_S = BlockConfig::vital( "AR1-S", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_AR2_D = BlockConfig::vital( "AR1-D", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_AR2_R = BlockConfig::vital( "AR1-R", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_AR3_M = BlockConfig::vital( "AR1-M", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_AR3_S = BlockConfig::vital( "AR1-S", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_AR3_D = BlockConfig::vital( "AR1-D", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_AR3_R = BlockConfig::vital( "AR1-R", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_AR4_M = BlockConfig::vital( "AR1-M", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_AR4_S = BlockConfig::vital( "AR1-S", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_AR4_D = BlockConfig::vital( "AR1-D", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_AR4_R = BlockConfig::vital( "AR1-R", "mmHg" );

  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_SPO2_P = BlockConfig::vital( "SPO2-%", "%" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_SPO2_R = BlockConfig::vital( "SPO2-R", "Bpm" );
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
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_CVP1 = BlockConfig::vital( "CVP1", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_CVP2 = BlockConfig::vital( "CVP2", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_CVP3 = BlockConfig::vital( "CVP3", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_CVP4 = BlockConfig::vital( "CVP4", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_CPP1 = BlockConfig::vital( "CPP1", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_ICP1 = BlockConfig::vital( "ICP1", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_CPP2 = BlockConfig::vital( "CPP2", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_ICP2 = BlockConfig::vital( "ICP2", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_CPP3 = BlockConfig::vital( "CPP3", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_ICP3 = BlockConfig::vital( "ICP3", "mmHg" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_CPP4 = BlockConfig::vital( "CPP4", "mmHg" );
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
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_CO2_RR = BlockConfig::vital( "CO2-RR", "BrMin" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_O2_EXP = BlockConfig::div10( "O2-EXP", "%" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_O2_INSP = BlockConfig::div10( "O2-INSP", "%" );

  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_RWOBVT = BlockConfig::vital( "rWOBVT", "J/L" );
  const StpGeSegment::VitalsBlock::BlockConfig StpGeSegment::VitalsBlock::BC_RI_E = BlockConfig::vital( "rI:E", "" );
 
  const std::map<StpGeSegment::VitalsBlock::Signal, std::map<int, std::vector<StpGeSegment::VitalsBlock::BlockConfig>>> LOOKUP = {
    { StpGeSegment::VitalsBlock::Signal::HR,{
        {0x3A,
          { StpGeSegment::VitalsBlock::BC_SKIP2, StpGeSegment::VitalsBlock::BC_HR, StpGeSegment::VitalsBlock::BC_PVC } },
        {0x56,
          { StpGeSegment::VitalsBlock::BC_SKIP6, StpGeSegment::VitalsBlock::BC_STI, StpGeSegment::VitalsBlock::BC_STII, StpGeSegment::VitalsBlock::BC_STIII } },
        {0x57,
          { StpGeSegment::VitalsBlock::BC_SKIP6, StpGeSegment::VitalsBlock::BC_STV1 } },
        {0x58,
          { } },
        {0x59,
          { } } }
    },
    { StpGeSegment::VitalsBlock::Signal::AR,{
        {0x4D,
          {StpGeSegment::VitalsBlock::BC_SKIP6, StpGeSegment::VitalsBlock::BC_AR1_M, StpGeSegment::VitalsBlock::BC_AR1_S, StpGeSegment::VitalsBlock::BC_AR1_D, StpGeSegment::VitalsBlock::BC_SKIP2, StpGeSegment::VitalsBlock::BC_AR1_R } },
        {0x4E,
          {StpGeSegment::VitalsBlock::BC_SKIP6, StpGeSegment::VitalsBlock::BC_AR2_M, StpGeSegment::VitalsBlock::BC_AR2_S, StpGeSegment::VitalsBlock::BC_AR2_D, StpGeSegment::VitalsBlock::BC_SKIP2, StpGeSegment::VitalsBlock::BC_AR2_R } },
        {0x4F,
          {StpGeSegment::VitalsBlock::BC_SKIP6, StpGeSegment::VitalsBlock::BC_AR3_M, StpGeSegment::VitalsBlock::BC_AR3_S, StpGeSegment::VitalsBlock::BC_AR3_D, StpGeSegment::VitalsBlock::BC_SKIP2, StpGeSegment::VitalsBlock::BC_AR3_R } },
        {0x50,
          {StpGeSegment::VitalsBlock::BC_SKIP6, StpGeSegment::VitalsBlock::BC_AR4_M, StpGeSegment::VitalsBlock::BC_AR4_S, StpGeSegment::VitalsBlock::BC_AR4_D, StpGeSegment::VitalsBlock::BC_SKIP2, StpGeSegment::VitalsBlock::BC_AR4_R } } } 
    },
    { StpGeSegment::VitalsBlock::Signal::PA,{
        {0x4D,
          {StpGeSegment::VitalsBlock::BC_SKIP6, StpGeSegment::VitalsBlock::BC_PA1_M, StpGeSegment::VitalsBlock::BC_PA1_S, StpGeSegment::VitalsBlock::BC_PA1_D, StpGeSegment::VitalsBlock::BC_SKIP2, StpGeSegment::VitalsBlock::BC_PA1_R } },
        {0x4E,
          {StpGeSegment::VitalsBlock::BC_SKIP6, StpGeSegment::VitalsBlock::BC_PA2_M, StpGeSegment::VitalsBlock::BC_PA2_S, StpGeSegment::VitalsBlock::BC_PA2_D, StpGeSegment::VitalsBlock::BC_SKIP2, StpGeSegment::VitalsBlock::BC_PA2_R } },
        {0x4F,
          {StpGeSegment::VitalsBlock::BC_SKIP6, StpGeSegment::VitalsBlock::BC_PA3_M, StpGeSegment::VitalsBlock::BC_PA3_S, StpGeSegment::VitalsBlock::BC_PA3_D, StpGeSegment::VitalsBlock::BC_SKIP2, StpGeSegment::VitalsBlock::BC_PA3_R } },
        {0x50,
          {StpGeSegment::VitalsBlock::BC_SKIP6, StpGeSegment::VitalsBlock::BC_PA4_M, StpGeSegment::VitalsBlock::BC_PA4_S, StpGeSegment::VitalsBlock::BC_PA4_D, StpGeSegment::VitalsBlock::BC_SKIP2, StpGeSegment::VitalsBlock::BC_PA4_R } } }
    },
    { StpGeSegment::VitalsBlock::Signal::LA,{
        {0x4D,
          {StpGeSegment::VitalsBlock::BC_SKIP6, StpGeSegment::VitalsBlock::BC_CVP1 } },
        {0x4E,
          {StpGeSegment::VitalsBlock::BC_SKIP6, StpGeSegment::VitalsBlock::BC_CVP2 } },
        {0x4F,
          {StpGeSegment::VitalsBlock::BC_SKIP6, StpGeSegment::VitalsBlock::BC_CVP3 } },
        {0x50,
          {StpGeSegment::VitalsBlock::BC_SKIP6, StpGeSegment::VitalsBlock::BC_CVP4 } } }
    },
    { StpGeSegment::VitalsBlock::Signal::ICP,{
        {0x4D,
          {StpGeSegment::VitalsBlock::BC_SKIP6, StpGeSegment::VitalsBlock::BC_ICP1 } },
        {0x4E,
          {StpGeSegment::VitalsBlock::BC_SKIP6, StpGeSegment::VitalsBlock::BC_ICP2 } },
        {0x4F,
          {StpGeSegment::VitalsBlock::BC_SKIP6, StpGeSegment::VitalsBlock::BC_ICP3 } },
        {0x50,
          {StpGeSegment::VitalsBlock::BC_SKIP6, StpGeSegment::VitalsBlock::BC_ICP4 } } }
    },
    { StpGeSegment::VitalsBlock::Signal::SP,{
        {0x4D,
          {StpGeSegment::VitalsBlock::BC_SKIP6, StpGeSegment::VitalsBlock::BC_SP1 } } }
    },
    { StpGeSegment::VitalsBlock::Signal::APNEA,{
        {0x4D,
          {StpGeSegment::VitalsBlock::BC_SKIP6, StpGeSegment::VitalsBlock::BC_RESP, StpGeSegment::VitalsBlock::BC_APNEA  } } }
    },
    { StpGeSegment::VitalsBlock::Signal::BT,{
        {0x22,
          {StpGeSegment::VitalsBlock::BC_SKIP6, StpGeSegment::VitalsBlock::BC_BT, StpGeSegment::VitalsBlock::BC_IT } } }
    },
    { StpGeSegment::VitalsBlock::Signal::NBP,{
        {0x18,
          {StpGeSegment::VitalsBlock::BC_SKIP6, StpGeSegment::VitalsBlock::BC_NBP_M, StpGeSegment::VitalsBlock::BC_NBP_S,StpGeSegment::VitalsBlock::BC_NBP_D, StpGeSegment::VitalsBlock::BC_SKIP2, StpGeSegment::VitalsBlock::BC_CUFF } } }
    },
    { StpGeSegment::VitalsBlock::Signal::SPO2,{
        {0x18,
          {StpGeSegment::VitalsBlock::BC_SKIP6, StpGeSegment::VitalsBlock::BC_SPO2_P, StpGeSegment::VitalsBlock::BC_SPO2_R } } }
    },
    { StpGeSegment::VitalsBlock::Signal::TEMP,{
        {0x22,
          {StpGeSegment::VitalsBlock::BC_SKIP6, StpGeSegment::VitalsBlock::BC_TMP_1, StpGeSegment::VitalsBlock::BC_TMP_2, StpGeSegment::VitalsBlock::BC_DELTA_TMP } },
        {0x23,
          {StpGeSegment::VitalsBlock::BC_SKIP6, StpGeSegment::VitalsBlock::BC_TMP_1, StpGeSegment::VitalsBlock::BC_TMP_2, StpGeSegment::VitalsBlock::BC_DELTA_TMP } } }
    },
    { StpGeSegment::VitalsBlock::Signal::CO2,{
        {0x36,
          {StpGeSegment::VitalsBlock::BC_SKIP6, StpGeSegment::VitalsBlock::BC_CO2_EX, StpGeSegment::VitalsBlock::BC_CO2_IN, StpGeSegment::VitalsBlock::BC_CO2_RR, StpGeSegment::VitalsBlock::BC_SKIP2, StpGeSegment::VitalsBlock::BC_O2_EXP, StpGeSegment::VitalsBlock::BC_O2_INSP } },
        {0x4D,
          {StpGeSegment::VitalsBlock::BC_SKIP6, StpGeSegment::VitalsBlock::BC_CO2_EX, StpGeSegment::VitalsBlock::BC_CO2_IN, StpGeSegment::VitalsBlock::BC_CO2_RR, StpGeSegment::VitalsBlock::BC_SKIP2, StpGeSegment::VitalsBlock::BC_O2_EXP, StpGeSegment::VitalsBlock::BC_O2_INSP } } }
    },
    { StpGeSegment::VitalsBlock::Signal::UAC,{
        {0x4D,
          {StpGeSegment::VitalsBlock::BC_SKIP6, StpGeSegment::VitalsBlock::BC_UAC1_M, StpGeSegment::VitalsBlock::BC_UAC1_S, StpGeSegment::VitalsBlock::BC_UAC1_D, StpGeSegment::VitalsBlock::BC_SKIP2, StpGeSegment::VitalsBlock::BC_UAC1_R } },
        {0x4E,
          {StpGeSegment::VitalsBlock::BC_SKIP6, StpGeSegment::VitalsBlock::BC_UAC2_M, StpGeSegment::VitalsBlock::BC_UAC2_S, StpGeSegment::VitalsBlock::BC_UAC2_D, StpGeSegment::VitalsBlock::BC_SKIP2, StpGeSegment::VitalsBlock::BC_UAC2_R } },
        {0x4F,
          {StpGeSegment::VitalsBlock::BC_SKIP6, StpGeSegment::VitalsBlock::BC_UAC3_M, StpGeSegment::VitalsBlock::BC_UAC3_S, StpGeSegment::VitalsBlock::BC_UAC3_D, StpGeSegment::VitalsBlock::BC_SKIP2, StpGeSegment::VitalsBlock::BC_UAC3_R } },
        {0x50,
          {StpGeSegment::VitalsBlock::BC_SKIP6, StpGeSegment::VitalsBlock::BC_UAC4_M, StpGeSegment::VitalsBlock::BC_UAC4_S, StpGeSegment::VitalsBlock::BC_UAC4_D, StpGeSegment::VitalsBlock::BC_SKIP2, StpGeSegment::VitalsBlock::BC_UAC4_R } } }
    },
    { StpGeSegment::VitalsBlock::Signal::PT,{
        {0xC2,
          {StpGeSegment::VitalsBlock::BC_SKIP6, StpGeSegment::VitalsBlock::BC_PT_RR, StpGeSegment::VitalsBlock::BC_PEEP, StpGeSegment::VitalsBlock::BC_MV, StpGeSegment::VitalsBlock::BC_SKIP2, StpGeSegment::VitalsBlock::BC_Fi02, StpGeSegment::VitalsBlock::BC_TV, StpGeSegment::VitalsBlock::BC_PIP,StpGeSegment::VitalsBlock::BC_PPLAT, StpGeSegment::VitalsBlock::BC_MAWP, StpGeSegment::VitalsBlock::BC_SENS } } }
    },
    { StpGeSegment::VitalsBlock::Signal::NBP2,{
        {0x18,
          {StpGeSegment::VitalsBlock::BC_SKIP6, StpGeSegment::VitalsBlock::BC_NBP_M, StpGeSegment::VitalsBlock::BC_NBP_S,StpGeSegment::VitalsBlock::BC_NBP_D, StpGeSegment::VitalsBlock::BC_SKIP2, StpGeSegment::VitalsBlock::BC_CUFF } } }
    },
    { StpGeSegment::VitalsBlock::Signal::VENT,{
        {0x5C,
          {StpGeSegment::VitalsBlock::BC_SKIP6, StpGeSegment::VitalsBlock::BC_APRV_LO, StpGeSegment::VitalsBlock::BC_APRV_HI, StpGeSegment::VitalsBlock::BC_APRV_LO_T, StpGeSegment::VitalsBlock::BC_SKIP2, StpGeSegment::VitalsBlock::BC_APRV_HI_T, StpGeSegment::VitalsBlock::BC_COMP, StpGeSegment::VitalsBlock::BC_RESIS, StpGeSegment::VitalsBlock::BC_MEAS_PEEP, StpGeSegment::VitalsBlock::BC_INTR_PEEP, StpGeSegment::VitalsBlock::BC_SPONT_R } },
        {0x5D,
          {StpGeSegment::VitalsBlock::BC_SKIP6, StpGeSegment::VitalsBlock::BC_INSP_TV } },
        {0xDB,
          {StpGeSegment::VitalsBlock::BC_SKIP6, StpGeSegment::VitalsBlock::BC_VENT, StpGeSegment::VitalsBlock::BC_FLW_R, StpGeSegment::VitalsBlock::BC_SKIP4, StpGeSegment::VitalsBlock::BC_IN_HLD, StpGeSegment::VitalsBlock::BC_SKIP2, StpGeSegment::VitalsBlock::BC_PRS_SUP, StpGeSegment::VitalsBlock::BC_INSP_TM, StpGeSegment::VitalsBlock::BC_INSP_PC, StpGeSegment::VitalsBlock::BC_I_E } },
        {0xDC,
          {StpGeSegment::VitalsBlock::BC_SKIP6, StpGeSegment::VitalsBlock::BC_HF_FLW, StpGeSegment::VitalsBlock::BC_HF_R, StpGeSegment::VitalsBlock::BC_HF_PRS, StpGeSegment::VitalsBlock::BC_SPONT_MV, StpGeSegment::VitalsBlock::BC_SKIP2, StpGeSegment::VitalsBlock::BC_SET_TV, StpGeSegment::VitalsBlock::BC_SET_PCP, StpGeSegment::VitalsBlock::BC_SET_IE, StpGeSegment::VitalsBlock::BC_B_FLW, StpGeSegment::VitalsBlock::BC_FLW_TRIG } } }
    },
    { StpGeSegment::VitalsBlock::Signal::RWOBVT,{
        {0x5A,
          {StpGeSegment::VitalsBlock::BC_SKIP6, StpGeSegment::VitalsBlock::BC_RWOBVT } },
        {0x5B,
          {StpGeSegment::VitalsBlock::BC_SKIP6, StpGeSegment::VitalsBlock::BC_RI_E } } }
    }
  };
  // </editor-fold>

  // <editor-fold defaultstate="collapsed" desc="wave labels">
  /**
   * WARNING: wave labels can change depending on what vitals are in the file,
   * but that is not yet implemented
   **/
  const std::map<int, std::string> StpGeSegment::WavesBlock::WAVELABELS = {
    {0x07, "I" },
    {0x08, "II" },
    {0x09, "III" },
    {0x0A, "V" },
    {0x0B, "AVR" },
    {0x0C, "AVF" },
    {0x0D, "AVL" },
    {0x17, "RR" },
    {0x1B, "AR1" },
    {0x1C, "ICP2" }, // may also be PA2
    {0x1D, "CVP3" }, // may also be PA3
    {0x1E, "CVP4" },
    {0x27, "SPO2" },
    {0x2A, "CO2" },
    {0xC8, "VNT_PRES" },
    {0xC9, "VNT_FLOW" },
  };
  // </editor-fold>

  StpGeSegment::StpGeSegment( Header h, const std::vector<VitalsBlock>& vitals,
      const std::vector<WavesBlock>& waves ) : header( h ), vitals( vitals ), waves( waves ) { }

  StpGeSegment::~StpGeSegment( ) { }

  StpGeSegment::Header::Header( unsigned long magic, dr_time t, const std::string& p )
      : time( t ), patient( p ), unity( 0 == magic ), magic( magic ) { }

  StpGeSegment::Header::Header( const Header& o )
      : time( o.time ), patient( o.patient ), unity( o.unity ), magic( o.magic ) { }

  StpGeSegment::Header StpGeSegment::Header::parse( std::vector<unsigned char>& data ) {
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

  StpGeSegment::VitalsBlock::VitalsBlock( Signal s, int mode, unsigned long start )
      : signal( s ), mode( mode ), startpos( start ) { }

  StpGeSegment::VitalsBlock StpGeSegment::VitalsBlock::index( std::vector<unsigned char>& data,
      unsigned long pos, GEParseError& errcode ) {
    int sig = data[pos];
    int mode = data[pos + 1];

    auto sigs = std::vector{ HR, AR, PA, LA, CVP, ICP, SP, APNEA, BT, NBP, SPO2, TEMP,
      CO2, UAC, PT, NBP2, VENT, RWOBVT };
    for ( auto& x : sigs ) {
      if ( x == sig ) {
        return VitalsBlock( x, mode, pos );
      }
    }

    std::stringstream ss;
    ss << "unhandled block: " << std::setfill( '0' ) << std::setw( 2 ) << std::hex
        << sig << " " << std::setfill( '0' ) << std::setw( 2 ) << std::hex << mode
        << " starting at " << std::dec << pos;
    throw std::runtime_error( ss.str( ) );
  }

  StpGeSegment::WavesBlock::WavesBlock( int seq, unsigned long start, unsigned long end )
      : sequence( seq ), startpos( start ), endpos( end ) { }

  StpGeSegment::WavesBlock StpGeSegment::WavesBlock::index( std::vector<unsigned char>& data,
      unsigned long& start, GEParseError& errcode ) {
    // waves block always starts with 0x04, then a sequence number
    auto seq = readUInt( data, start + 1 );
    auto wstart = start;
    auto idx = 0LU;
    auto wend = findEndAndIndex( data, start, idx );
    start = wend;

    return WavesBlock( seq, wstart, wend );
  }

  unsigned long StpGeSegment::WavesBlock::findEndAndIndex( std::vector<unsigned char>& rawdata,
      unsigned long start, unsigned long& indexstart ) {
    // every waves block ends with 0xFA0D, plus some indexing data.
    // find the 0xFA0D, and then skip until you find the 0x04 (next wave block)

    start++;
    // search until we find an FA, then see if it's followed by a 0D
    while ( rawdata.size( ) > start + 1 ) {
      auto byte = readUInt( rawdata, start );
      start++;
      if ( 0xFA == byte ) {
        auto byte2 = readUInt( rawdata, start );
        if ( 0x0D == byte2 ) {
          start++;
          break;
        }
        if ( 0xFA != byte2 ) {
          start++;
        }
      }
    }
    indexstart = start;

    while ( rawdata.size( ) > start && 0x04 != readUInt( rawdata, start ) ) {
      start++;
    }

    return start;
 }

  std::unique_ptr<StpGeSegment> StpGeSegment::index( std::vector<unsigned char>& data,
      bool skipwaves, GEParseError& err ) {
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
        err = UNKNOWN_VITALSTYPE;
      }
      vitstart += VITALS_SIZE + 2; // 2 blank bytes at the end of every segment
    }

    auto waves = std::vector<WavesBlock>( );
    if ( !skipwaves ) {
      while ( wavestart < data.size( ) ) {
        WavesBlock w = WavesBlock::index( data, wavestart, err);
        waves.push_back(w);
      }
    }

    return std::unique_ptr<StpGeSegment>( new StpGeSegment( h, vitals, waves ) );
  }

  unsigned int StpGeSegment::readUInt( std::vector<unsigned char>& rawdata, unsigned long start ) {
    return rawdata[start];
  }

  unsigned int StpGeSegment::readUInt2( std::vector<unsigned char>& rawdata, unsigned long start ) {
    unsigned char b1 = rawdata[start];
    unsigned char b2 = rawdata[start + 1];
    return ( b1 << 8 | b2 );
  }

  unsigned long StpGeSegment::readUInt4( std::vector<unsigned char>& rawdata, unsigned long start ) {
    unsigned char b0 = rawdata[start];
    unsigned char b1 = rawdata[start + 1];
    unsigned char b2 = rawdata[start + 2];
    unsigned char b3 = rawdata[start + 3];
    return ( b0 << 24 | b1 << 16 | b2 << 8 | b3 );
  }
}