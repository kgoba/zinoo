#include "ublox.h"

void gps_send(uint8_t c);
void gps_send(const uint8_t *buf, int length);

void ublox_command(uint8_t cls, uint8_t id, const uint8_t *payload, int length) {
    const uint8_t sync1 = 0xB5;
    const uint8_t sync2 = 0x62;
    uint8_t ck_a = 0, ck_b = 0;

    uint8_t length_lo = (length & 0xFF);
    uint8_t length_hi = (length >> 8) & 0xFF;

    gps_send(sync1);
    gps_send(sync2);
    gps_send(cls);   ck_a += cls; ck_b += ck_a;
    gps_send(id);    ck_a += id; ck_b += ck_a;
    gps_send(length_lo);    ck_a += length_lo; ck_b += ck_a;
    gps_send(length_hi);    ck_a += length_hi; ck_b += ck_a;

    while (length > 0) {
        gps_send(*payload); ck_a += *payload; ck_b += ck_a;
        length--;
        payload++;
    }

    gps_send(ck_a);
    gps_send(ck_b);
}

void ublox_enable_trkd5() {
    const uint8_t payload[] = { 
        0x03, 0x0a, 0x01
    };
    // CFG-MSG: 0x06 0x01
    // See https://wiki.openstreetmap.org/wiki/UbloxRAW
    ublox_command(0x06, 0x01, payload, sizeof(payload));
}

void ublox_enable_sfrbx() {
    const uint8_t payload[] = { 
        0x03, 0x0f, 0x01
    };
    // CFG-MSG: 0x06 0x01
    // See https://wiki.openstreetmap.org/wiki/UbloxRAW
    ublox_command(0x06, 0x01, payload, sizeof(payload));
}

void ublox_patch_raw_v6_02() {
    const uint8_t payload[] = { 
        0xdc, 0x0f, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 
        0x23, 0xcc, 0x21, 0x00, 
        0x00, 0x00, 0x02, 0x10
    };
    // message: 0x09 0x01 (unknown?)
    // See https://wiki.openstreetmap.org/wiki/UbloxRAW
    ublox_command(0x09, 0x01, payload, sizeof(payload));
}

void ublox_patch_raw_v7_03() {
    const uint8_t payload[] = { 
        0xc8, 0x16, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 
        0x97, 0x69, 0x21, 0x00,
        0x00, 0x00, 0x02, 0x10
    };
    // message: 0x09 0x01 (unknown?)
    // See https://wiki.openstreetmap.org/wiki/UbloxRAW
    ublox_command(0x09, 0x01, payload, sizeof(payload));
}

void ublox_powermode(ublox_pm_t pm) {
    const uint8_t payload[] = { 
        0x00, 0x00, 0x00, 0x00,
          pm, 0x00, 0x00, 0x00
    };
    // message: RXM-PMREQ
    ublox_command(0x02, 0x41, payload, sizeof(payload));
}

void ublox_cfg_tp5(
    uint32_t freqPeriod, uint32_t freqPeriodLock, 
    uint32_t pulseLenRatio, uint32_t pulseLenRatioLock, 
    bool isFreq, bool isLength) 
{
    const uint8_t flags = 0x40 | 0x20 | (isLength ? 0x10 : 0) | (isFreq ? 0x08 : 0) | 0x04 | 0x02 | 0x01;
    const uint8_t payload[] = { 
        0x00,       // tpIdx (0 = TIMEPULSE, 1 = TIMEPULSE2)
        0x00,       // reserved0
        0x00, 0x00, // reserved1
        0x00, 0x00, // antCableDelay
        0x00, 0x00, // rfGroupDelay
        (uint8_t)(freqPeriod >> 0), (uint8_t)(freqPeriod >> 8), 
        (uint8_t)(freqPeriod >> 16), (uint8_t)(freqPeriod >> 24),
        (uint8_t)(freqPeriodLock >> 0), (uint8_t)(freqPeriodLock >> 8), 
        (uint8_t)(freqPeriodLock >> 16), (uint8_t)(freqPeriodLock >> 24),
        (uint8_t)(pulseLenRatio >> 0), (uint8_t)(pulseLenRatio >> 8), 
        (uint8_t)(pulseLenRatio >> 16), (uint8_t)(pulseLenRatio >> 24),
        (uint8_t)(pulseLenRatioLock >> 0), (uint8_t)(pulseLenRatioLock >> 8), 
        (uint8_t)(pulseLenRatioLock >> 16), (uint8_t)(pulseLenRatioLock >> 24),
        0x00, 0x00, 0x00, 0x00, // userConfigDelay
        flags, 0x00, 0x00, 0x00  // flags
    };
    // message: CFG-TP5
    ublox_command(0x06, 0x31, payload, sizeof(payload));
}

void ublox_reset() {
    const uint8_t payload[] = { 
        0xFF, 0xFF,     // navBbrMask
        0x00,           // resetMode
        0x00            // reserved
    };
    // message: CFG-RST
    ublox_command(0x06, 0x04, payload, sizeof(payload));
}

void ublox_mon_ver() {
    ublox_command(0x0A, 0x04, 0, 0);
}

void ublox_set_dyn_mode(ublox_dyn_t dyn) {
    const uint8_t payload[] = {
        0x01, 0x80,             // mask
        dyn,                    // dynMode1 
        0x00,                   // fixMode
        0x00, 0x00, 0x00, 0x00, // fixedAlt
        0x00, 0x00, 0x00, 0x00, // fixedAltVar
        0x00,                   // minElev
        0x00,                   // drLimit
        0x00, 0x00,             // pdop
        0x00, 0x00,             // tdop
        0x00, 0x00,             // pacc
        0x00, 0x00,             // tacc
        0x00,                   // staticHoldThresh
        0x00,                   // dgpsTimeOut
        0x00, 0x00, 0x00, 0x00, // reserved2
        0x00, 0x00, 0x00, 0x00, // reserved3
        0x00, 0x00, 0x00, 0x00, // reserved4
    };
    // message: CFG-NAV5
    ublox_command(0x06, 0x24, payload, sizeof(payload));
}

#define NFREQ  3        /* number of carrier frequencies */
#define NEXOBS 0        /* number of extended obs codes */
#define MAXOBS 64       /* max number of obs in an epoch */

typedef uint32_t time_t;

typedef struct {        /* time struct */
    time_t time;        /* time (s) expressed by standard time_t */
    double sec;         /* fraction of second under 1 s */
} gtime_t;

typedef struct {        /* observation data record */
    gtime_t time;       /* receiver sampling time (GPST) */
    //unsigned char sat,rcv; /* satellite/receiver number */
    uint8_t sys;
    uint16_t prn;
    uint8_t SNR [NFREQ+NEXOBS]; /* signal strength (0.25 dBHz) */
    //uint8_t LLI [NFREQ+NEXOBS]; /* loss of lock indicator */
    uint8_t code[NFREQ+NEXOBS]; /* code indicator (CODE_???) */
    double L[NFREQ+NEXOBS]; /* observation data carrier-phase (cycle) */
    double P[NFREQ+NEXOBS]; /* observation data pseudorange (m) */
    float  D[NFREQ+NEXOBS]; /* observation data doppler frequency (Hz) */
} obsd_t;

typedef struct {
    unsigned n;
    obsd_t data[MAXOBS];
} obs_t;

#define SYS_NONE    0x00                /* navigation system: none */
#define SYS_GPS     0x01                /* navigation system: GPS */
#define SYS_SBS     0x02                /* navigation system: SBAS */
#define SYS_GLO     0x04                /* navigation system: GLONASS */
#define SYS_GAL     0x08                /* navigation system: Galileo */
#define SYS_QZS     0x10                /* navigation system: QZSS */
#define SYS_CMP     0x20                /* navigation system: BeiDou */
#define SYS_IRN     0x40                /* navigation system: IRNSS */
#define SYS_LEO     0x80                /* navigation system: LEO */
#define SYS_ALL     0xFF                /* navigation system: all */

#define MINPRNSBS   120                 /* min satellite PRN number of SBAS */
#define MAXPRNSBS   142                 /* max satellite PRN number of SBAS */
#define NSATSBS     (MAXPRNSBS-MINPRNSBS+1) /* number of SBAS satellites */

// #define MINPRNGPS   1                   /* min satellite PRN number of GPS */
// #define MAXPRNGPS   32                  /* max satellite PRN number of GPS */
// #define NSATGPS     (MAXPRNGPS-MINPRNGPS+1) /* number of GPS satellites */
// #define NSYSGPS     1

// #define MINPRNGLO   1                   /* min satellite slot number of GLONASS */
// #define MAXPRNGLO   24                  /* max satellite slot number of GLONASS */
// #define NSATGLO     (MAXPRNGLO-MINPRNGLO+1) /* number of GLONASS satellites */
// #define NSYSGLO     1

// #define MINPRNGAL   1                   /* min satellite PRN number of Galileo */
// #define MAXPRNGAL   30                  /* max satellite PRN number of Galileo */
// #define NSATGAL    (MAXPRNGAL-MINPRNGAL+1) /* number of Galileo satellites */
// #define NSYSGAL     1

// #define MINPRNQZS   193                 /* min satellite PRN number of QZSS */
// #define MAXPRNQZS   199                 /* max satellite PRN number of QZSS */
// #define MINPRNQZS_S 183                 /* min satellite PRN number of QZSS SAIF */
// #define MAXPRNQZS_S 189                 /* max satellite PRN number of QZSS SAIF */

// extern int satno(int sys, int prn)
// {
//     if (prn<=0) return 0;
//     switch (sys) {
//         case SYS_GPS:
//             if (prn<MINPRNGPS||MAXPRNGPS<prn) return 0;
//             return prn-MINPRNGPS+1;
//         case SYS_GLO:
//             if (prn<MINPRNGLO||MAXPRNGLO<prn) return 0;
//             return NSATGPS+prn-MINPRNGLO+1;
//         case SYS_GAL:
//             if (prn<MINPRNGAL||MAXPRNGAL<prn) return 0;
//             return NSATGPS+NSATGLO+prn-MINPRNGAL+1;
//         case SYS_QZS:
//             if (prn<MINPRNQZS||MAXPRNQZS<prn) return 0;
//             return NSATGPS+NSATGLO+NSATGAL+prn-MINPRNQZS+1;
//         case SYS_CMP:
//             if (prn<MINPRNCMP||MAXPRNCMP<prn) return 0;
//             return NSATGPS+NSATGLO+NSATGAL+NSATQZS+prn-MINPRNCMP+1;
//         case SYS_LEO:
//             if (prn<MINPRNLEO||MAXPRNLEO<prn) return 0;
//             return NSATGPS+NSATGLO+NSATGAL+NSATQZS+NSATCMP+prn-MINPRNLEO+1;
//         case SYS_SBS:
//             if (prn<MINPRNSBS||MAXPRNSBS<prn) return 0;
//             return NSATGPS+NSATGLO+NSATGAL+NSATQZS+NSATCMP+NSATLEO+prn-MINPRNSBS+1;
//     }
//     return 0;
// }

#include <cmath>

#define U1(p) (*((uint8_t *)(p)))
#define I1(p) (*((int8_t *)(p)))
#define U2(p) (*((uint16_t *)(p)))
#define I2(p) (*((int16_t *)(p)))
#define U4(p) (*((uint32_t *)(p)))
#define I4(p) (*((int32_t *)(p)))
#define U8(p) (*((uint64_t *)(p)))
#define I8(p) (*((int64_t *)(p)))

#define ROUND(x)    (int)floor((x)+0.5)

#define CLIGHT      299792458.0         /* speed of light (m/s) */
#define P2_32       2.328306436538696E-10 /* 2^-32 */
#define P2_10       0.0009765625 /* 2^-10 */

#define CODE_NONE   0                   /* obs code: none or unknown */
#define CODE_L1I    47                  /* obs code: B1I        (BDS) */
#define CODE_L1C    1                   /* obs code: L1C/A,G1C/A,E1C (GPS,GLO,GAL,QZS,SBS) */

// double difftime(time_t t1, time_t t2) {
//     return (t1 - t2);
// }

// double timediff(gtime_t t1, gtime_t t2)
// {
//     return difftime(t1.time,t2.time)+t1.sec-t2.sec;
// }

// gtime_t gpst2utc(gtime_t t)
// {
//     gtime_t tu;
//     int i;
    
//     for (i=0;leaps[i][0]>0;i++) {
//         tu=timeadd(t,leaps[i][6]);
//         if (timediff(tu,epoch2time(leaps[i]))>=0.0) return tu;
//     }
//     return t;
// }

static uint8_t ubx_sys(uint8_t ind)
{
    switch (ind) {
        case 0: return SYS_GPS;
        case 1: return SYS_SBS;
        case 2: return SYS_GAL;
        case 3: return SYS_CMP;
        case 5: return SYS_QZS;
        case 6: return SYS_GLO;
    }
    return 0;
}

static int decode_trkd5(obs_t *obs, const uint8_t *raw, uint8_t raw_len)
{
    gtime_t time;
    double ts,tr=-1.0,t,tau,adr,snr,utc_gpst = 0;
    float dop;
    int i,j,n=0,type,off,len,sys,prn,sat,qi,flag,week;
    //int frq;
    const uint8_t *p=raw+6;
        
    //utc_gpst=timediff(gpst2utc(raw->time),raw->time);
    
    switch ((type=U1(p))) {
        case 3 : off=86; len=56; break;
        case 6 : off=86; len=64; break; /* u-blox 7 */
        default: off=78; len=56; break;
    }
    // for (i=0, p=raw+off; p-raw < raw_len-2; i++, p+=len) {
    //     if (U1(p+41)<4) continue;
    //     t=I8(p)*P2_32/1000.0;
    //     if (ubx_sys(U1(p+56))==SYS_GLO) t-=10800.0+utc_gpst;
    //     if (t>tr) tr=t;
    // }
    // if (tr<0.0) return 0;
    
    // tr=ROUND((tr+0.08)/0.1)*0.1;
    
    /* adjust week handover */
    // t=time2gpst(raw->time,&week);
    // if      (tr<t-302400.0) week++;
    // else if (tr>t+302400.0) week--;
    // time=gpst2time(week,tr);
        
    for (i=0, p=raw+off; p-raw < raw_len-2; i++, p+=len) {
        
        /* quality indicator */
        qi =U1(p+41)&7;
        if (qi<4||7<qi) continue;
        
        if (type==6) {
            if (!(sys=ubx_sys(U1(p+56)))) {
                continue;
            }
            prn = U1(p+57)+(sys==SYS_QZS?192:0);
            // frq=U1(p+59)-7;
        }
        else {
            prn = U1(p+34);
            sys = (prn<MINPRNSBS) ? SYS_GPS : SYS_SBS;
        }
        // if (!(sat=satno(sys,prn))) {
        //     trace(2,"ubx trkd5 sat number error: sys=%2d prn=%2d\n",sys,prn);
        //     continue;
        // }
        /* transmission time */
        ts=I8(p)*P2_32/1000.0;
        if (sys==SYS_GLO) ts-=10800.0+utc_gpst; /* glot -> gpst */
        
        /* signal travel time */
        tau=tr-ts;
        if      (tau<-302400.0) tau+=604800.0;
        else if (tau> 302400.0) tau-=604800.0;
        
        flag=U1(p+54);   /* tracking status */
        adr=qi<6?0.0:I8(p+8)*P2_32+(flag&0x01?0.5:0.0);
        dop=I4(p+16)*P2_10/4.0;
        snr=U2(p+32)/256.0;
        
        // if (snr<=10.0) raw->lockt[sat-1][1]=1.0;
                
        /* check phase lock */
        if (!(flag&0x08)) continue;
        
        obs->data[n].time=time;
        obs->data[n].sys=sys;
        obs->data[n].prn=prn;
        obs->data[n].P[0]=tau*CLIGHT;
        obs->data[n].L[0]=-adr;
        obs->data[n].D[0]=(float)dop;
        obs->data[n].SNR[0]=(uint8_t)(snr*4.0);
        obs->data[n].code[0]=sys==SYS_CMP?CODE_L1I:CODE_L1C;
        //obs->data[n].LLI[0]=raw->lockt[sat-1][1]>0.0?1:0;
        //lockt[sat-1][1]=0.0;
        
        for (j=1;j<NFREQ+NEXOBS;j++) {
            obs->data[n].L[j]=obs->data[n].P[j]=0.0;
            obs->data[n].D[j]=0.0;
            obs->data[n].SNR[j]=0;
            //obs->data[n].LLI[j]=0;
            obs->data[n].code[j]=CODE_NONE;
        }
        n++;
    }
    if (n<=0) return 0;
    // raw->time=time;
    obs->n=n;
    return 1;
}