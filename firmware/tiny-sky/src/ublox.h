#pragma once

#include <stdint.h>

enum ublox_pm_t { 
    ePM_POWERON     = 0,
    ePM_BACKUP      = 2,
};


enum ublox_dyn_t { 
    eDYN_PORTABLE    = 0,
    eDYN_STATIONARY  = 2,
    eDYN_PEDESTRIAN  = 3,
    eDYN_AUTOMOTIVE  = 4,
    eDYN_SEA         = 5,
    eDYN_AIRBORNE_1G = 6,
    eDYN_AIRBORNE_2G = 7,
    eDYN_AIRBORNE_4G = 8,
};

void ublox_enable_sfrbx();

void ublox_enable_trkd5();

void ublox_patch_raw_v6_02();

void ublox_patch_raw_v7_03();

void ublox_powermode(ublox_pm_t pm);

void ublox_reset();

void ublox_mon_ver();

void ublox_set_dyn_mode(ublox_dyn_t dyn);

void ublox_cfg_tp5(
    uint32_t freqPeriod, uint32_t freqPeriodLock, 
    uint32_t pulseLenRatio, uint32_t pulseLenRatioLock, 
    bool isFreq, bool isLength);
