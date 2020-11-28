#ifndef __PAL_SENSORS_H__
#define __PAL_SENSORS_H__

#include <openbmc/pmbus.h>
#include <openbmc/obmc_pal_sensors.h>

#define MAX_SENSOR_NUM         (0xFF)
#define MAX_DEVICE_NAME_SIZE   (128)
#define MB_TEMP_DEVICE  "/sys/class/i2c-dev/i2c-%d/device/%d-00%x/hwmon/hwmon*/temp1_input"
#define MB_ADC_VOLTAGE_DEVICE "/sys/devices/platform/iio-hwmon/hwmon/hwmon*/in%d_input"

//PECI CMD INFO
#define PECI_RETRY_TIMES                        (10)
#define PECI_CMD_RD_PKG_CONFIG                  (0xA1)
#define PECI_INDEX_ACCUMULATED_ENERGY_STATUS    (3)
#define PECI_INDEX_THERMAL_MARGIN               (10)
#define PECI_INDEX_DIMM_THERMAL_RW              (14)
#define PECI_INDEX_TEMP_TARGET                  (16)
#define PECI_INDEX_TOTAL_TIME                   (31)
#define PECI_INDEX_PKG_IDENTIFIER               (0)
#define PECI_THERMAL_DIMM0_BYTE                 (1)
#define PECI_THERMAL_DIMM1_BYTE                 (2)
#define INTERNAL_CPU_ERROR_MASK                 (0x1C)
#define EXTERNAL_CPU_ERROR_MASK                 (0xE0)
#define BOTH_CPU_ERROR_MASK                     (0xFC)

//NM DEVICE INFO
#define NM_IPMB_BUS_ID   (5)
#define NM_SLAVE_ADDR    (0x2C)

//CPU INFO
#define PECI_CPU0_ADDR    (0x30)
#define PECI_CPU1_ADDR    (0x31)

//AMD1278 CMD INFO
#define PMBUS_PMON_CONFIG  (0xD4)
#define ADM1278_SLAVE_ADDR (0x22)
#define ADM1278_RSENSE     (0.15)

//INA260 CMD INFO
#define INA260_CURRENT   (0x01)
#define INA260_VOLTAGE   (0x02)
#define INA260_POWER     (0x03)

//ADC128 INFO
#define ADC128_FAN_SCALED_R1 (5110) //unit: ohm
#define ADC128_FAN_SCALED_R2 (1020) //unit: ohm
#define ADC128_GIMON         (243)  //unit: (uA/A)
#define ADC128_RIMON         (1820) //unit: ohm

//AVAII/NVMe INFO
#define RISER1_BUS                (2)
#define RISER2_BUS                (6)
#define RISER1_PCA9846_BUS_BASE   (24) // Riser1 – CH 0 to 3: I2C bus 24 to 27
#define RISER2_PCA9846_BUS_BASE   (28) // Riser2 – CH 0 to 3: I2C bus 28 to 31
#define RISER_PCA9846_CH_MAX_NUM  (4)
#define NVMe_SMBUS_ADDR           (0xD4)
#define AVA_FRUID_ADDR            (0xA0)
#define NVMe_GET_STATUS_CMD       (0x00)
#define NVMe_GET_STATUS_LEN       (8)
#define NVMe_TEMP_REG             (0x03)

//Sensor Table
enum {
  MB_SNR_PCH_TEMP = 0x08,
//NIC
  NIC_MEZZ0_SNR_TEMP = 0x10,
  NIC_MEZZ1_SNR_TEMP = 0x11,

  MB_SNR_FAN0_TACH = 0x2A,
  MB_SNR_FAN1_TACH = 0x2B,

  MB_SNR_CPU0_TJMAX = 0x30,
  MB_SNR_CPU1_TJMAX = 0x31,
  MB_SNR_CPU0_PKG_POWER = 0x32,
  MB_SNR_CPU1_PKG_POWER = 0x33,
  MB_SNR_CPU0_THERM_MARGIN = 0x34,
  MB_SNR_CPU1_THERM_MARGIN = 0x35,

//ADC128 VOLT/CURR/POWER
  FCB_FAN0_VOLT  = 0x12,
  FCB_FAN0_CURR  = 0x13,
  FCB_FAN0_PWR = 0x14,
  FCB_FAN1_VOLT  = 0x15,
  FCB_FAN1_CURR  = 0x16,
  FCB_FAN1_PWR = 0x17,
  FCB_FAN2_VOLT  = 0x18,
  FCB_FAN2_CURR  = 0x19,
  FCB_FAN2_PWR = 0x1A,
  FCB_FAN3_VOLT  = 0x1B,
  FCB_FAN3_CURR  = 0x1C,
  FCB_FAN3_PWR = 0x1D,
  FCB_FAN4_VOLT  = 0x1E,
  FCB_FAN4_CURR  = 0x1F,
  FCB_FAN4_PWR = 0x20,
  FCB_FAN5_VOLT = 0x21,
  FCB_FAN5_CURR = 0x22,
  FCB_FAN5_PWR = 0x23,
  FCB_FAN6_VOLT = 0x24,
  FCB_FAN6_CURR = 0x25,
  FCB_FAN6_PWR = 0x26,
  FCB_FAN7_VOLT = 0x27,
  FCB_FAN7_CURR = 0x28,
  FCB_FAN7_PWR = 0x29,

//HSC
  MB_SNR_HSC_VIN = 0x40,
  MB_SNR_HSC_IOUT = 0x41,
  MB_SNR_HSC_PIN = 0x42,
  MB_SNR_HSC_TEMP = 0x43,
//DIMM TEMP
  MB_SNR_CPU0_DIMM_GRPA_TEMP = 0x50,
  MB_SNR_CPU0_DIMM_GRPB_TEMP = 0x51,
  MB_SNR_CPU0_DIMM_GRPC_TEMP = 0x52,
  MB_SNR_CPU0_DIMM_GRPD_TEMP = 0x53,
  MB_SNR_CPU0_DIMM_GRPE_TEMP = 0x54,
  MB_SNR_CPU0_DIMM_GRPF_TEMP = 0x55,
  MB_SNR_CPU1_DIMM_GRPA_TEMP = 0x56,
  MB_SNR_CPU1_DIMM_GRPB_TEMP = 0x57,
  MB_SNR_CPU1_DIMM_GRPC_TEMP = 0x58,
  MB_SNR_CPU1_DIMM_GRPD_TEMP = 0x59,
  MB_SNR_CPU1_DIMM_GRPE_TEMP = 0x5A,
  MB_SNR_CPU1_DIMM_GRPF_TEMP = 0x5B,

//SYS FAN RPM
  MB_SNR_FAN0_TACH_IN  = 0x60,
  MB_SNR_FAN0_TACH_OUT = 0x61,
  MB_SNR_FAN1_TACH_IN  = 0x62,
  MB_SNR_FAN1_TACH_OUT = 0x63,
  MB_SNR_FAN2_TACH_IN  = 0x64,
  MB_SNR_FAN2_TACH_OUT = 0x65,
  MB_SNR_FAN3_TACH_IN  = 0x66,
  MB_SNR_FAN3_TACH_OUT = 0x67,
  MB_SNR_FAN4_TACH_IN  = 0x68,
  MB_SNR_FAN4_TACH_OUT = 0x69,
  MB_SNR_FAN5_TACH_IN  = 0x6A,
  MB_SNR_FAN5_TACH_OUT = 0x6B,
  MB_SNR_FAN6_TACH_IN  = 0x6C,
  MB_SNR_FAN6_TACH_OUT = 0x6D,
  MB_SNR_FAN7_TACH_IN  = 0x6E,
  MB_SNR_FAN7_TACH_OUT = 0x6F,

//HARD DISK TEMP
  MB_SNR_BOOT_DRIVER_TEMP = 0x70,
  MB_SNR_DATA0_DRIVER_TEMP = 0x71,
  MB_SNR_DATA1_DRIVER_TEMP = 0x72,

//AVAII CARD TEMP
  RISER1_SNR_AVAII_FTEMP     = 0x80,
  RISER1_SNR_AVAII_RTEMP     = 0x81,
  RISER2_SNR_AVAII_FTEMP     = 0x82,
  RISER2_SNR_AVAII_RTEMP     = 0x83,

//NVMe TEMP
  RISER1_SNR_SLOT0_NVME_TEMP = 0x84,
  RISER1_SNR_SLOT1_NVME_TEMP = 0x85,
  RISER1_SNR_SLOT2_NVME_TEMP = 0x86,
  RISER1_SNR_SLOT3_NVME_TEMP = 0x87,
  RISER2_SNR_SLOT0_NVME_TEMP = 0x88,
  RISER2_SNR_SLOT1_NVME_TEMP = 0x89,
  RISER2_SNR_SLOT2_NVME_TEMP = 0x8A,
  RISER2_SNR_SLOT3_NVME_TEMP = 0x8B,

//INA260
  MB_SNR_P3V3_STBY_INA260_VOL = 0x90,
  MB_SNR_P3V3_STBY_INA260_CURR = 0x91,
  MB_SNR_P3V3_STBY_INA260_POWER = 0x92,
  MB_SNR_P3V3_M2_1_INA260_VOL = 0x93,
  MB_SNR_P3V3_M2_1_INA260_CURR = 0x94,
  MB_SNR_P3V3_M2_1_INA260_POWER = 0x95,
  MB_SNR_P3V3_M2_2_INA260_VOL = 0x96,
  MB_SNR_P3V3_M2_2_INA260_CURR = 0x97,
  MB_SNR_P3V3_M2_2_INA260_POWER = 0x98,
  MB_SNR_P3V3_M2_3_INA260_VOL = 0x99,
  MB_SNR_P3V3_M2_3_INA260_CURR = 0x9A,
  MB_SNR_P3V3_M2_3_INA260_POWER = 0x9B,

  MB_SNR_POWER_FAIL = 0x9C,
  MB_SNR_MEMORY_LOOP_FAIL = 0x9D,
//Board TEMP  
  MB_SNR_INLET_TEMP = 0xA0,
  MB_SNR_OUTLET_TEMP_R = 0xA1,
  MB_SNR_OUTLET_TEMP_L = 0xA2,
  MB_SNR_INLET_REMOTE_TEMP = 0xA3,
  MB_SNR_OUTLET_REMOTE_TEMP_R = 0xA4,
  MB_SNR_OUTLET_REMOTE_TEMP_L = 0xA5,
//CPU TEMP  
  MB_SNR_CPU0_TEMP = 0xAA,
  MB_SNR_CPU1_TEMP = 0xAB,
//CPU VR
  MB_SNR_VR_CPU0_VCCIN_VOLT = 0xB0,
  MB_SNR_VR_CPU0_VCCIN_TEMP = 0xB1,
  MB_SNR_VR_CPU0_VCCIN_CURR = 0xB2,
  MB_SNR_VR_CPU0_VCCIN_POWER = 0xB3,
  MB_SNR_VR_CPU0_VSA_VOLT = 0xB4,
  MB_SNR_VR_CPU0_VSA_TEMP = 0xB5,
  MB_SNR_VR_CPU0_VSA_CURR = 0xB6,
  MB_SNR_VR_CPU0_VSA_POWER = 0xB7,
  MB_SNR_VR_CPU0_VCCIO_VOLT = 0xB8,
  MB_SNR_VR_CPU0_VCCIO_TEMP = 0xB9,
  MB_SNR_VR_CPU0_VCCIO_CURR = 0xBA,
  MB_SNR_VR_CPU0_VCCIO_POWER = 0xBB,
  MB_SNR_VR_CPU0_VDDQ_GRPABC_VOLT = 0xBC,
  MB_SNR_VR_CPU0_VDDQ_GRPABC_TEMP = 0xBD,
  MB_SNR_VR_CPU0_VDDQ_GRPABC_CURR = 0xBE,
  MB_SNR_VR_CPU0_VDDQ_GRPABC_POWER = 0xBF,
  MB_SNR_VR_CPU0_VDDQ_GRPDEF_VOLT = 0xC0,
  MB_SNR_VR_CPU0_VDDQ_GRPDEF_TEMP = 0xC1,
  MB_SNR_VR_CPU0_VDDQ_GRPDEF_CURR = 0xC2,
  MB_SNR_VR_CPU0_VDDQ_GRPDEF_POWER = 0xC3,
//PCH VR
  MB_SNR_VR_PCH_P1V05_VOLT = 0xC4,
  MB_SNR_VR_PCH_P1V05_TEMP = 0xC5,
  MB_SNR_VR_PCH_P1V05_CURR = 0xC6,
  MB_SNR_VR_PCH_P1V05_POWER = 0xC7,
  MB_SNR_VR_PCH_PVNN_VOLT = 0xC8,
  MB_SNR_VR_PCH_PVNN_TEMP = 0xC9,
  MB_SNR_VR_PCH_PVNN_CURR = 0xCA,
  MB_SNR_VR_PCH_PVNN_POWER = 0xCB, 
//ADC
  MB_SNR_P5V = 0xD0,  
  MB_SNR_P5V_STBY = 0xD1,
  MB_SNR_P3V3_STBY = 0xD2,
  MB_SNR_P3V3 = 0xD3,
  MB_SNR_P3V_BAT = 0xD4,
  MB_SNR_CPU_1V8 = 0xD5,
  MB_SNR_PCH_1V8 = 0xD6,
  MB_SNR_CPU0_PVPP_ABC= 0xD7,
  MB_SNR_CPU1_PVPP_ABC= 0xD8,
  MB_SNR_CPU0_PVPP_DEF= 0xD9,
  MB_SNR_CPU1_PVPP_DEF= 0xDA,
  MB_SNR_CPU0_PVTT_ABC= 0xDB,
  MB_SNR_CPU1_PVTT_ABC= 0xDC,
  MB_SNR_CPU0_PVTT_DEF= 0xDD,
  MB_SNR_CPU1_PVTT_DEF= 0xDE,
  MB_SNR_P12V_OCP_IMON = 0xDF,
//VR CPU1
  MB_SNR_VR_CPU1_VCCIN_VOLT = 0xE0,
  MB_SNR_VR_CPU1_VCCIN_TEMP = 0xE1,
  MB_SNR_VR_CPU1_VCCIN_CURR = 0xE2,
  MB_SNR_VR_CPU1_VCCIN_POWER = 0xE3,
  MB_SNR_VR_CPU1_VSA_VOLT = 0xE4,
  MB_SNR_VR_CPU1_VSA_TEMP = 0xE5,
  MB_SNR_VR_CPU1_VSA_CURR = 0xE6,
  MB_SNR_VR_CPU1_VSA_POWER = 0xE7,
  MB_SNR_VR_CPU1_VCCIO_VOLT = 0xE8,
  MB_SNR_VR_CPU1_VCCIO_TEMP = 0xE9,
  MB_SNR_VR_CPU1_VCCIO_CURR = 0xEA,
  MB_SNR_VR_CPU1_VCCIO_POWER = 0xEB,
  MB_SNR_VR_CPU1_VDDQ_GRPABC_VOLT = 0xEC,
  MB_SNR_VR_CPU1_VDDQ_GRPABC_TEMP = 0xED,
  MB_SNR_VR_CPU1_VDDQ_GRPABC_CURR = 0xEE,
  MB_SNR_VR_CPU1_VDDQ_GRPABC_POWER = 0xEF,
  MB_SNR_VR_CPU1_VDDQ_GRPDEF_VOLT = 0xF0,
  MB_SNR_VR_CPU1_VDDQ_GRPDEF_TEMP = 0xF1,
  MB_SNR_VR_CPU1_VDDQ_GRPDEF_CURR = 0XF2,
  MB_SNR_VR_CPU1_VDDQ_GRPDEF_POWER = 0XF3,

//Discrete sensor
  MB_SENSOR_PROCESSOR_FAIL = 0xFC,
};

typedef struct {
  float ucr_thresh;
  float unc_thresh;
  float unr_thresh;
  float lcr_thresh;
  float lnc_thresh;
  float lnr_thresh;
  float pos_hyst;
  float neg_hyst;
} PAL_SENSOR_THRESHOLD;

enum {
  TEMP = 1,
  CURR,
  VOLT,
  FAN,
  POWER,
};

enum {
  FCB_0,
  FCB_1,
};

typedef struct {
  char* snr_name;
  uint8_t id;
  int (*read_sensor) (uint8_t id, float *value);
  uint8_t stby_read;
  PAL_SENSOR_THRESHOLD snr_thresh;
  uint8_t units;
  int retry;
  float last_value;
} PAL_SENSOR_MAP;

//ADC INFO
enum {
  ADC0 = 0,
  ADC1,
  ADC2,
  ADC3,
  ADC4,
  ADC5,
  ADC6,
  ADC7,
  ADC8,
  ADC9,
  ADC10,
  ADC11, 
  ADC12, 
  ADC13, 
  ADC14,
  ADC15,
};

//INA260 INFO
enum {
  INA260_ID0,
  INA260_ID1,
  INA260_ID2,
  INA260_ID3,
};

//GENERIC I2C Sensors
enum {
  TEMP_INLET = 0,
  TEMP_OUTLET_R,
  TEMP_OUTLET_L,
  TEMP_REMOTE_INLET ,
  TEMP_REMOTE_OUTLET_R,
  TEMP_REMOTE_OUTLET_L,
};

//NIC INFO
enum {
  MEZZ0 = 0,
  MEZZ1,
};

//HARD DISK INFO
enum {
  DISK_BOOT = 0,
  DISK_DATA0,
  DISK_DATA1,
};

//PECI CPU INFO
enum {
  CPU_ID0 = 0,
  CPU_ID1 = 1,
};

typedef struct {
  uint8_t cpu_id;
  uint8_t cpu_addr;
}PAL_CPU_INFO;

enum {
  DIMM_CRPA = 0,
  DIMM_CRPB = 1,
  DIMM_CRPC = 2,
  DIMM_CRPD = 3,
  DIMM_CRPE = 4,
  DIMM_CRPF = 5,
};

typedef struct {
  uint8_t cpu_addr;
  uint8_t index;
  uint8_t dev_info;
  uint8_t para_l;
  uint8_t para_h;
} PECI_RD_PKG_CONFIG_INFO;

//ADM1278 INFO
enum {
  ADM1278_VOLTAGE = 0,
  ADM1278_CURRENT,
  ADM1278_POWER,
  ADM1278_TEMP,
};

typedef struct {
  uint8_t type;
  float m;
  float b;
  float r;
} PAL_ADM1278_INFO;

//HSC INFO
enum {
  HSC_ID0,
};

typedef struct {
  uint8_t id;
  uint8_t slv_addr;
  PAL_ADM1278_INFO* info;
} PAL_HSC_INFO;

//NM ID
enum {
  NM_ID0,
};

//PCH SENSOR INFO
enum {
  NM_PCH_TEMP = 8,
};

typedef struct {
  uint8_t id;
  uint8_t bus;
  uint8_t slv_addr;
} PAL_I2C_BUS_INFO;

typedef struct {
  int integer :10;
  uint8_t fract :6;
} PAL_S10_6_FORMAT;

//VR TPS53688 INFO
enum {
  VR_ID0 = 0,
  VR_ID1,
  VR_ID2,
  VR_ID3,
  VR_ID4,
  VR_ID5,
  VR_ID6,
  VR_ID7,
  VR_ID8,
  VR_ID9,
  VR_ID_NUM
};

//PCH VR
enum {
  PCH_ID_NUM = 2,
  VR_ID10 = 10,
  VR_ID11 = 11,
};

// ADC128 INFO
enum {
  FAN_ID0 = 0,
  FAN_ID1,
  FAN_ID2,
  FAN_ID3,
  FAN_ID4,
  FAN_ID5,
  FAN_ID6,
  FAN_ID7,
};

int pal_set_fan_led_state(uint8_t fan_id, char* state);
int pal_get_fan_led_state(uint8_t fan_id, char* state);
int pal_fan_led_control(void);
int cmd_peci_get_cpu_err_num(int* num, uint8_t is_caterr);
bool pal_is_dimm_present(int cpu_id, uint8_t dimm_id);
bool pal_dimm_present_check(uint8_t snr_num);
bool pal_is_nic_prsnt(uint8_t snr_num);
void pal_second_crashdump_chk(void);
bool pal_set_post_complete(bool is_completed);
#endif
