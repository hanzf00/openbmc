#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>
#include <openbmc/libgpio.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/obmc-sensors.h>
#include "pal.h"
#include "pal_calibration.h"

#define PAL_FAN_CNT 4
#define PAL_NIC_CNT 8
#define GPIO_BATTERY_DETECT "BATTERY_DETECT"
#define GPIO_NIC_PRSNT "OCP_V3_%d_PRSNTB_R_N"

size_t pal_pwm_cnt = 4;
size_t pal_tach_cnt = 8;
const char pal_pwm_list[] = "0..3";
const char pal_tach_list[] = "0..7";

static int read_inlet_sensor(uint8_t snr_id, float *value);
static int read_fan_volt(uint8_t fan_id, float *value);
static int read_fan_curr(uint8_t fan_id, float *value);
static int read_fan_pwr(uint8_t fan_id, float *value);
static int read_adc_value(uint8_t adc_id, float *value);
static int read_bat_value(uint8_t adc_id, float *value);
static int read_hsc_iout(uint8_t hsc_id, float *value);
static int read_hsc_vin(uint8_t hsc_id, float *value);
static int read_hsc_pin(uint8_t hsc_id, float *value);
static void hsc_value_adjust(struct calibration_table *table, float *value);
static int read_pax_therm(uint8_t, float*);
static int read_nvme_temp(uint8_t sensor_num, float *value);
static int read_nvme_volt(uint8_t nvme_id, float *value);
static int read_nvme_curr(uint8_t nvme_id, float *value);
static int read_nvme_pwr(uint8_t nvme_id, float *value);
static int read_bay_temp(uint8_t sensor_num, float *value);
static int read_hsc_temp(uint8_t hsc_id, float *value);
static int read_nic_temp(uint8_t nic_id, float *value);
static int read_nic_volt(uint8_t nic_id, float *value);
static int read_nic_curr(uint8_t nic_id, float *value);
static int read_nic_pwr(uint8_t nic_id, float *value);
static int sensors_read_vr(uint8_t sensor_num, float *value);

static float fan_volt[PAL_FAN_CNT];
static float fan_curr[PAL_FAN_CNT];
static float nic_volt[PAL_NIC_CNT];
static float nic_curr[PAL_NIC_CNT];
static float nvme_volt[2];
static float nvme_curr[2];
static float vr_vdd_volt[vr_NUM];
static float vr_vdd_curr[vr_NUM];
static float vr_avd_volt[vr_NUM];
static float vr_avd_curr[vr_NUM];

const uint8_t mb_sensor_list[] = {
  MB_NIC_0_TEMP,
  MB_NIC_0_VOLT,
  MB_NIC_0_CURR,
  MB_NIC_0_POWER,
  MB_NIC_1_TEMP,
  MB_NIC_1_VOLT,
  MB_NIC_1_CURR,
  MB_NIC_1_POWER,
  MB_NIC_2_TEMP,
  MB_NIC_2_VOLT,
  MB_NIC_2_CURR,
  MB_NIC_2_POWER,
  MB_NIC_3_TEMP,
  MB_NIC_3_VOLT,
  MB_NIC_3_CURR,
  MB_NIC_3_POWER,
  MB_NIC_4_TEMP,
  MB_NIC_4_VOLT,
  MB_NIC_4_CURR,
  MB_NIC_4_POWER,
  MB_NIC_5_TEMP,
  MB_NIC_5_VOLT,
  MB_NIC_5_CURR,
  MB_NIC_5_POWER,
  MB_NIC_6_TEMP,
  MB_NIC_6_VOLT,
  MB_NIC_6_CURR,
  MB_NIC_6_POWER,
  MB_NIC_7_TEMP,
  MB_NIC_7_VOLT,
  MB_NIC_7_CURR,
  MB_NIC_7_POWER,
  MB_VR_P0V8_VDD0_TEMP,
  MB_VR_P0V8_VDD0_VOUT,
  MB_VR_P0V8_VDD0_CURR,
  MB_VR_P0V8_VDD0_POWER,
  MB_VR_P1V0_AVD0_TEMP,
  MB_VR_P1V0_AVD0_VOUT,
  MB_VR_P1V0_AVD0_CURR,
  MB_VR_P1V0_AVD0_POWER,
  MB_VR_P0V8_VDD1_TEMP,
  MB_VR_P0V8_VDD1_VOUT,
  MB_VR_P0V8_VDD1_CURR,
  MB_VR_P0V8_VDD1_POWER,
  MB_VR_P1V0_AVD1_TEMP,
  MB_VR_P1V0_AVD1_VOUT,
  MB_VR_P1V0_AVD1_CURR,
  MB_VR_P1V0_AVD1_POWER,
  MB_VR_P0V8_VDD2_TEMP,
  MB_VR_P0V8_VDD2_VOUT,
  MB_VR_P0V8_VDD2_CURR,
  MB_VR_P0V8_VDD2_POWER,
  MB_VR_P1V0_AVD2_TEMP,

  MB_VR_P1V0_AVD2_VOUT,
  MB_VR_P1V0_AVD2_CURR,
  MB_VR_P1V0_AVD2_POWER,
  MB_VR_P0V8_VDD3_TEMP,
  MB_VR_P0V8_VDD3_VOUT,
  MB_VR_P0V8_VDD3_CURR,
  MB_VR_P0V8_VDD3_POWER,
  MB_VR_P1V0_AVD3_TEMP,
  MB_VR_P1V0_AVD3_VOUT,
  MB_VR_P1V0_AVD3_CURR,
  MB_VR_P1V0_AVD3_POWER,
  MB_P12V_AUX,
  MB_P3V3_STBY,
  MB_P5V_STBY,
  MB_P3V3,
  MB_P3V3_PAX,
  MB_P3V_BAT,
  MB_P2V5_AUX,
  MB_P1V2_AUX,
  MB_P1V15_AUX,
  MB_PAX_0_TEMP,
  MB_PAX_1_TEMP,
  MB_PAX_2_TEMP,
  MB_PAX_3_TEMP,
  SYSTEM_INLET_TEMP,
  SYSTEM_INLET_REMOTE_TEMP,
};

const uint8_t pdb_sensor_list[] = {
  PDB_HSC_TEMP,
  PDB_HSC_VIN,
  PDB_HSC_IOUT,
  PDB_HSC_PIN,
  PDB_FAN0_VOLT,
  PDB_FAN0_CURR,
  PDB_FAN0_PWR,
  PDB_FAN1_VOLT,
  PDB_FAN1_CURR,
  PDB_FAN1_PWR,
  PDB_FAN2_VOLT,
  PDB_FAN2_CURR,
  PDB_FAN2_PWR,
  PDB_FAN3_VOLT,
  PDB_FAN3_CURR,
  PDB_FAN3_PWR,
  PDB_INLET_TEMP_L,
  PDB_INLET_REMOTE_TEMP_L,
  PDB_INLET_TEMP_R,
  PDB_INLET_REMOTE_TEMP_R,
};

const uint8_t ava1_sensor_list[] = {
  BAY_0_FTEMP,
  BAY_0_RTEMP,
  BAY_0_0_NVME_CTEMP,
  BAY_0_1_NVME_CTEMP,
  BAY_0_2_NVME_CTEMP,
  BAY_0_3_NVME_CTEMP,
  BAY_0_4_NVME_CTEMP,
  BAY_0_5_NVME_CTEMP,
  BAY_0_6_NVME_CTEMP,
  BAY_0_7_NVME_CTEMP,
  BAY_0_VOL,
  BAY_0_IOUT,
  BAY_0_POUT,
};

const uint8_t ava2_sensor_list[] = {
  BAY_1_FTEMP,
  BAY_1_RTEMP,
  BAY_1_0_NVME_CTEMP,
  BAY_1_1_NVME_CTEMP,
  BAY_1_2_NVME_CTEMP,
  BAY_1_3_NVME_CTEMP,
  BAY_1_4_NVME_CTEMP,
  BAY_1_5_NVME_CTEMP,
  BAY_1_6_NVME_CTEMP,
  BAY_1_7_NVME_CTEMP,
  BAY_1_VOL,
  BAY_1_IOUT,
  BAY_1_POUT,
};

PAL_I2C_BUS_INFO nic_info_list[] = {
  {MEZZ0, I2C_BUS_1, 0x3E},
  {MEZZ1, I2C_BUS_9, 0x3E},
  {MEZZ2, I2C_BUS_2, 0x3E},
  {MEZZ3, I2C_BUS_10, 0x3E},
  {MEZZ4, I2C_BUS_4, 0x3E},
  {MEZZ5, I2C_BUS_11, 0x3E},
  {MEZZ6, I2C_BUS_7, 0x3E},
  {MEZZ7, I2C_BUS_13, 0x3E},
};

//ADM1278
PAL_ADM1278_INFO adm1278_info_list[] = {
  {ADM1278_VOLTAGE, 19599, 0, 100},
  {ADM1278_CURRENT, 842 * ADM1278_RSENSE, 20475, 10},
  {ADM1278_POWER, 6442 * ADM1278_RSENSE, 0, 100},
  {ADM1278_TEMP, 42, 31880, 10},
};

PAL_HSC_INFO hsc_info_list[] = {
  {HSC_ID0, ADM1278_SLAVE_ADDR, adm1278_info_list },
};

//{SensorName, ID, FUNCTION, PWR_STATUS, {UCR, UNR, UNC, LCR, LNR, LNC, Pos, Neg}
PAL_SENSOR_MAP sensor_map[] = {
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x00
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x01
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x02
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x03
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x04
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x05
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x06
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x07
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x08
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x09
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x0A
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x0B
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x0C
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x0D
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x0E
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x0F

  {"NIC_0_TEMP", NIC0, read_nic_temp, 0, {95, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x10
  {"NIC_0_VOLT", NIC0, read_nic_volt, 0, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT}, //0x11
  {"NIC_0_IOUT", NIC0, read_nic_curr, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x12
  {"NIC_0_POUT", NIC0, read_nic_pwr , 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x13
  {"NIC_1_TEMP", NIC1, read_nic_temp, 0, {95, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x14
  {"NIC_1_VOLT", NIC1, read_nic_volt, 0, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT}, //0x15
  {"NIC_1_IOUT", NIC1, read_nic_curr, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x16
  {"NIC_1_POUT", NIC1, read_nic_pwr , 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x17
  {"NIC_2_TEMP", NIC2, read_nic_temp, 0, {95, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x18
  {"NIC_2_VOLT", NIC2, read_nic_volt, 0, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT}, //0x19
  {"NIC_2_IOUT", NIC2, read_nic_curr, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x1A
  {"NIC_2_POUT", NIC2, read_nic_pwr , 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x1B
  {"NIC_3_TEMP", NIC3, read_nic_temp, 0, {95, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x1C
  {"NIC_3_VOLT", NIC3, read_nic_volt, 0, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT}, //0x1D
  {"NIC_3_IOUT", NIC3, read_nic_curr, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x1E
  {"NIC_3_POUT", NIC3, read_nic_pwr , 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x1F

  {"NIC_4_TEMP", NIC4, read_nic_temp, 0, {95, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x20
  {"NIC_4_VOLT", NIC4, read_nic_volt, 0, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT}, //0x21
  {"NIC_4_IOUT", NIC4, read_nic_curr, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x22
  {"NIC_4_POUT", NIC4, read_nic_pwr , 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x23
  {"NIC_5_TEMP", NIC5, read_nic_temp, 0, {95, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x24
  {"NIC_5_VOLT", NIC5, read_nic_volt, 0, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT}, //0x25
  {"NIC_5_IOUT", NIC5, read_nic_curr, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x26
  {"NIC_5_POUT", NIC5, read_nic_pwr , 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x27
  {"NIC_6_TEMP", NIC6, read_nic_temp, 0, {95, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x28
  {"NIC_6_VOLT", NIC6, read_nic_volt, 0, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT}, //0x29
  {"NIC_6_IOUT", NIC6, read_nic_curr, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x2A
  {"NIC_6_POUT", NIC6, read_nic_pwr , 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x2B
  {"NIC_7_TEMP", NIC7, read_nic_temp, 0, {95, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x2C
  {"NIC_7_VOLT", NIC7, read_nic_volt, 0, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT}, //0x2D
  {"NIC_7_IOUT", NIC7, read_nic_curr, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x2E
  {"NIC_7_POUT", NIC7, read_nic_pwr , 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x2F

  {"SSD_BAY_0_FTEMP", BAY0_FTEMP_ID, read_bay_temp, 0, {50, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x30
  {"SSD_BAY_0_RTEMP", BAY0_RTEMP_ID, read_bay_temp, 0, {70, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x31
  {"SSD_BAY_0_NVME_CTEMP", 0, NULL, 0, {60, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x32
  {"SSD_BAY_0_0_NVME_CTEMP", BAY_0_0_NVME_CTEMP, read_nvme_temp, 0, {60, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x33
  {"SSD_BAY_0_1_NVME_CTEMP", BAY_0_1_NVME_CTEMP, read_nvme_temp, 0, {60, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x34
  {"SSD_BAY_0_2_NVME_CTEMP", BAY_0_2_NVME_CTEMP, read_nvme_temp, 0, {60, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x35
  {"SSD_BAY_0_3_NVME_CTEMP", BAY_0_3_NVME_CTEMP, read_nvme_temp, 0, {60, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x36
  {"SSD_BAY_0_4_NVME_CTEMP", BAY_0_4_NVME_CTEMP, read_nvme_temp, 0, {60, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x37
  {"SSD_BAY_0_5_NVME_CTEMP", BAY_0_5_NVME_CTEMP, read_nvme_temp, 0, {60, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x38
  {"SSD_BAY_0_6_NVME_CTEMP", BAY_0_6_NVME_CTEMP, read_nvme_temp, 0, {60, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x39
  {"SSD_BAY_0_7_NVME_CTEMP", BAY_0_7_NVME_CTEMP, read_nvme_temp, 0, {60, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x3A
  {"SSD_BAY_0_VOL" , BAY_ID0, read_nvme_volt, 0, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT}, //0x3B
  {"SSD_BAY_0_IOUT", BAY_ID0, read_nvme_curr, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x3C
  {"SSD_BAY_0_POUT", BAY_ID0, read_nvme_pwr, 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x3D
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x3E
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x3F

  {"SSD_BAY_1_FTEMP", BAY1_FTEMP_ID, read_bay_temp, 0, {50, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x40
  {"SSD_BAY_1_RTEMP", BAY1_RTEMP_ID, read_bay_temp, 0, {70, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x41
  {"SSD_BAY_1_NVME_CTEMP", 0, NULL, 0, {60, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x42
  {"SSD_BAY_1_0_NVME_CTEMP", BAY_1_0_NVME_CTEMP, read_nvme_temp, 0, {60, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x43
  {"SSD_BAY_1_1_NVME_CTEMP", BAY_1_1_NVME_CTEMP, read_nvme_temp, 0, {60, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x44
  {"SSD_BAY_1_2_NVME_CTEMP", BAY_1_2_NVME_CTEMP, read_nvme_temp, 0, {60, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x45
  {"SSD_BAY_1_3_NVME_CTEMP", BAY_1_3_NVME_CTEMP, read_nvme_temp, 0, {60, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x46
  {"SSD_BAY_1_4_NVME_CTEMP", BAY_1_4_NVME_CTEMP, read_nvme_temp, 0, {60, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x47
  {"SSD_BAY_1_5_NVME_CTEMP", BAY_1_5_NVME_CTEMP, read_nvme_temp, 0, {60, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x48
  {"SSD_BAY_1_6_NVME_CTEMP", BAY_1_6_NVME_CTEMP, read_nvme_temp, 0, {60, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x49
  {"SSD_BAY_1_7_NVME_CTEMP", BAY_1_7_NVME_CTEMP, read_nvme_temp, 0, {60, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x4A
  {"SSD_BAY_1_VOL" , BAY_ID1, read_nvme_volt, 0, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT}, //0x4B
  {"SSD_BAY_1_IOUT", BAY_ID1, read_nvme_curr, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x4C
  {"SSD_BAY_1_POUT", BAY_ID1, read_nvme_pwr, 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x4D
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x4E
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x4F

  {"PDB_HSC_TEMP", HSC_ID0, read_hsc_temp, 0, {0, 0, 0, 0, 0, 0, 0, 0}, TEMP}, //0x50
  {"PDB_HSC_VIN" , HSC_ID0, read_hsc_vin , 0, {0, 0, 0, 0, 0, 0, 0, 0}, VOLT}, //0x51
  {"PDB_HSC_IOUT", HSC_ID0, read_hsc_iout, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x52
  {"PDB_HSC_PIN" , HSC_ID0, read_hsc_pin , 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x53
  {"BB_P12V_PUX" , ADC0, read_adc_value, true, {13.2, 0, 0, 10.8, 0, 0, 0, 0}    , VOLT}, //0x54
  {"BB_P3V3_STBY", ADC1, read_adc_value, true, {3.465, 0, 0, 3.135, 0, 0, 0, 0}  , VOLT}, //0x55
  {"BB_P5V_STBY" , ADC2, read_adc_value, true, {5.25, 0, 0, 4.75, 0, 0, 0, 0}    , VOLT}, //0x56
  {"BB_P3V3"     , ADC3, read_adc_value, true, {3.465, 0, 0, 3.135, 0, 0, 0, 0}  , VOLT}, //0x57
  {"BB_P3V3_PAX" , ADC4, read_adc_value, true, {3.465, 0, 0, 3.135, 0, 0, 0, 0}  , VOLT}, //0x58
  {"BB_P3V_BAT"  , ADC5, read_bat_value, true, {3.3, 0, 0, 2.85, 0, 0, 0, 0}    , VOLT}, //0x59
  {"BB_P2V5_AUX" , ADC6, read_adc_value, true, {2.625, 0, 0, 2.375, 0, 0, 0, 0}  , VOLT}, //0x5A
  {"BB_P1V2_AUX" , ADC7, read_adc_value, true, {1.26, 0, 0, 1.14, 0, 0, 0, 0}    , VOLT}, //0x5B
  {"BB_P1V15_AUX", ADC8, read_adc_value, true, {1.2075, 0, 0, 1.0925, 0, 0, 0, 0}, VOLT}, //0x5C
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x5D
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x5E
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x5F

  {"P0V8_VDD0_TEMP", MB_VR_P0V8_VDD0_TEMP , sensors_read_vr, 0, {115, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x60
  {"P0V8_VDD0_VOLT" , MB_VR_P0V8_VDD0_VOUT , sensors_read_vr, 0, {0.86, 0, 0, 0.82, 0, 0, 0}, VOLT}, //0x61
  {"P0V8_VDD0_IOUT", MB_VR_P0V8_VDD0_CURR , sensors_read_vr, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x62
  {"P0V8_VDD0_POUT", MB_VR_P0V8_VDD0_POWER, sensors_read_vr, 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x63
  {"P0V8_AVD_PCIE0_TEMP", MB_VR_P1V0_AVD0_TEMP , sensors_read_vr, 0, {115, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x64
  {"P0V8_AVD_PCIE0_VOLT", MB_VR_P1V0_AVD0_VOUT , sensors_read_vr, 0, {0.86, 0, 0, 0.82, 0, 0, 0, 0}, VOLT}, //0x65
  {"P0V8_AVD_PCIE0_IOUT", MB_VR_P1V0_AVD0_CURR , sensors_read_vr, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x66
  {"P0V8_AVD_PCIE0_POUT", MB_VR_P1V0_AVD0_POWER, sensors_read_vr, 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x67
  {"P0V8_VDD1_TEMP", MB_VR_P0V8_VDD1_TEMP , sensors_read_vr, 0, {115, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x68
  {"P0V8_VDD1_VOLT", MB_VR_P0V8_VDD1_VOUT , sensors_read_vr, 0, {0.86, 0, 0, 0.82, 0, 0, 0, 0}, VOLT}, //0x69
  {"P0V8_VDD1_IOUT", MB_VR_P0V8_VDD1_CURR , sensors_read_vr, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x6A
  {"P0V8_VDD1_POUT", MB_VR_P0V8_VDD1_POWER, sensors_read_vr, 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x6B
  {"P0V8_AVD_PCIE1_TEMP", MB_VR_P1V0_AVD1_TEMP , sensors_read_vr, 0, {115, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x6C
  {"P0V8_AVD_PCIE1_VOL" , MB_VR_P1V0_AVD1_VOUT , sensors_read_vr, 0, {0.86, 0, 0, 0.82, 0, 0, 0, 0}, VOLT}, //0x6D
  {"P0V8_AVD_PCIE1_IOUT", MB_VR_P1V0_AVD1_CURR , sensors_read_vr, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x6E
  {"P0V8_AVD_PCIE1_POUT", MB_VR_P1V0_AVD1_POWER, sensors_read_vr, 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x6F

  {"P0V8_VDD2_TEMP", MB_VR_P0V8_VDD2_TEMP, sensors_read_vr, 0, {115, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x70
  {"P0V8_VDD2_VOL" , MB_VR_P0V8_VDD2_VOUT, sensors_read_vr, 0, {0.86, 0, 0, 0.82, 0, 0, 0, 0}, VOLT}, //0x71
  {"P0V8_VDD2_IOUT", MB_VR_P0V8_VDD2_CURR, sensors_read_vr, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x72
  {"P0V8_VDD2_POUT", MB_VR_P0V8_VDD2_POWER, sensors_read_vr, 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x73
  {"P0V8_AVD_PCIE2_TEMP", MB_VR_P1V0_AVD2_TEMP, sensors_read_vr, 0, {115, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x74
  {"P0V8_AVD_PCIE2_VOLT", MB_VR_P1V0_AVD2_VOUT, sensors_read_vr, 0, {0.86, 0, 0, 0.82, 0, 0, 0, 0}, VOLT}, //0x75
  {"P0V8_AVD_PCIE2_IOUT", MB_VR_P1V0_AVD2_CURR, sensors_read_vr, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x76
  {"P0V8_AVD_PCIE2_POUT", MB_VR_P1V0_AVD2_POWER, sensors_read_vr, 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x77
  {"P0V8_VDD3_TEMP", MB_VR_P0V8_VDD3_TEMP, sensors_read_vr, 0, {115, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x78
  {"P0V8_VDD3_VOLT", MB_VR_P0V8_VDD3_VOUT, sensors_read_vr, 0, {0.86, 0, 0, 0.82, 0, 0, 0, 0}, VOLT}, //0x79
  {"P0V8_VDD3_IOUT", MB_VR_P0V8_VDD3_CURR, sensors_read_vr, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x7A
  {"P0V8_VDD3_POUT", MB_VR_P0V8_VDD3_POWER, sensors_read_vr, 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x7B
  {"P0V8_AVD_PCIE3_TEMP", MB_VR_P1V0_AVD3_TEMP, sensors_read_vr, 0, {115, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x7C
  {"P0V8_AVD_PCIE3_VOLT", MB_VR_P1V0_AVD3_VOUT, sensors_read_vr, 0, {0.86, 0, 0, 0.82, 0, 0, 0, 0}, VOLT}, //0x7D
  {"P0V8_AVD_PCIE3_IOUT", MB_VR_P1V0_AVD3_CURR, sensors_read_vr, 0, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x7E
  {"P0V8_AVD_PCIE3_POUT", MB_VR_P1V0_AVD3_POWER, sensors_read_vr, 0, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x7F

  {"BB_PAX_0_TEMP", PAX_ID0, read_pax_therm, 0, {95, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x80
  {"BB_PAX_1_TEMP", PAX_ID1, read_pax_therm, 0, {95, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x81
  {"BB_PAX_2_TEMP", PAX_ID2, read_pax_therm, 0, {95, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x82
  {"BB_PAX_3_TEMP", PAX_ID3, read_pax_therm, 0, {95, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x83
  {"FAN_0_VOL" , FAN_ID0, read_fan_volt, true, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT}, //0x84
  {"FAN_0_CURR", FAN_ID0, read_fan_curr, true, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x85
  {"FAN_0_PWR" , FAN_ID0, read_fan_pwr , true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x86
  {"FAN_1_VOL" , FAN_ID1, read_fan_volt, true, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT}, //0x87
  {"FAN_1_CURR", FAN_ID1, read_fan_curr, true, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x88
  {"FAN_1_PWR" , FAN_ID1, read_fan_pwr , true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x89
  {"FAN_2_VOL" , FAN_ID2, read_fan_volt, true, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT}, //0x8A
  {"FAN_2_CURR", FAN_ID2, read_fan_curr, true, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x8B
  {"FAN_2_PWR" , FAN_ID2, read_fan_pwr , true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x8C
  {"FAN_3_VOL" , FAN_ID3, read_fan_volt, true, {13.2, 0, 0, 10.8, 0, 0, 0, 0}, VOLT}, //0x8D
  {"FAN_3_CURR", FAN_ID3, read_fan_curr, true, {0, 0, 0, 0, 0, 0, 0, 0}, CURR}, //0x8E
  {"FAN_3_PWR" , FAN_ID3, read_fan_pwr , true, {0, 0, 0, 0, 0, 0, 0, 0}, POWER}, //0x8F

  {"SYSTEM_INLET_TEMP"       , SYS_TEMP       , read_inlet_sensor, 0, {50, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x90
  {"SYSTEM_INLET_REMOTE_TEMP", SYS_REMOTE_TEMP, read_inlet_sensor, 0, {50, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x91
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x92
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x93
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x94
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x95
  {"PDB_INLET_TEMP_L"       , INLET_TEMP_L       , read_inlet_sensor, 0, {65, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x96
  {"PDB_INLET_REMOTE_TEMP_L", INLET_REMOTE_TEMP_L, read_inlet_sensor, 0, {65, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x97
  {"PDB_INLET_TEMP_R"       , INLET_TEMP_R       , read_inlet_sensor, 0, {65, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x98
  {"PDB_INLET_REMOTE_TEMP_R", INLET_REMOTE_TEMP_R, read_inlet_sensor, 0, {65, 0, 0, 10, 0, 0, 0, 0}, TEMP}, //0x99
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x9A
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x9B
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x9C
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x9D
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x9E
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0x9F

  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xA0
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xA1
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xA2
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xA3
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xA4
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xA5
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xA6
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xA7
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xA8
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xA9
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xAA
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xAB
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xAC
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xAD
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xAE
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xAF

  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xB0
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xB1
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xB2
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xB3
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xB4
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xB5
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xB6
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xB7
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xB8
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xB9
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xBA
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xBB
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xBC
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xBD
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xBE
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xBF

  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xC0
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xC1
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xC2
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xC3
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xC4
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xC5
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xC6
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xC7
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xC8
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xC9
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xCA
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xCB
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xCC
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xCD
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xCE
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xCF

  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xD0
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xD1
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xD2
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xD3
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xD4
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xD5
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xD6
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xD7
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xD8
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xD9
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xDA
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xDB
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xDC
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xDD
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xDE
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xDF

  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xE0
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xE1
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xE2
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xE3
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xE4
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xE5
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xE6
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xE7
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xE8
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xE9
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xEA
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xEB
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xEC
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xED
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xEE
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xEF

  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xF0
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xF1
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xF2
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xF3
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xF4
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xF5
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xF6
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xF7
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xF8
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xF9
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xFA
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xFB
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xFC
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xFD
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xFE
  {NULL, 0, NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}, 0}, //0xFF
};

size_t mb_sensor_cnt = sizeof(mb_sensor_list)/sizeof(uint8_t);
size_t pdb_sensor_cnt = sizeof(pdb_sensor_list)/sizeof(uint8_t);
size_t ava1_sensor_cnt = sizeof(ava1_sensor_list)/sizeof(uint8_t);
size_t ava2_sensor_cnt = sizeof(ava2_sensor_list)/sizeof(uint8_t);

int pal_sensor_read_raw(uint8_t fru, uint8_t sensor_num, void *value)
{
  char key[MAX_KEY_LEN] = {0};
  char str[MAX_VALUE_LEN] = {0};
  char fru_name[32];
  int ret=0;
  uint8_t id=0;

  pal_get_fru_name(fru, fru_name);
  sprintf(key, "%s_sensor%d", fru_name, sensor_num);
  id = sensor_map[sensor_num].id;

  switch(fru) {
    case FRU_MB:
    case FRU_PDB:
    case FRU_AVA1:
    case FRU_AVA2:
      ret = sensor_map[sensor_num].read_sensor(id, (float*) value);
      break;
    default:
      return -1;
  }

  if (ret) {
    if (ret == READING_NA || ret == -1) {
      strcpy(str, "NA");
    } else {
      return ret;
    }
  } else {
    switch(sensor_num){
      case PDB_HSC_IOUT:
        hsc_value_adjust(current_table, value);
       break;
      case PDB_HSC_PIN:
        hsc_value_adjust(power_table, value);
      break;
    }
    sprintf(str, "%.2f",*((float*)value));
  }

  if(kv_set(key, str, 0, 0) < 0) {
    syslog(LOG_WARNING, "pal_sensor_read_raw: cache_set key = %s, str = %s failed.", key, str);
    return -1;
  } else {
    return ret;
  }

  return 0;
}

int
pal_get_sensor_name(uint8_t fru, uint8_t sensor_num, char *name) {
  switch(fru) {
  case FRU_MB:
  case FRU_PDB:
  case FRU_AVA1:
  case FRU_AVA2:
    sprintf(name, "%s", sensor_map[sensor_num].snr_name);
    break;
  default:
    return -1;
  }
  return 0;
}

int
pal_get_sensor_threshold(uint8_t fru, uint8_t sensor_num, uint8_t thresh, void *value) {
  float *val = (float*) value;
  switch(fru) {
  case FRU_MB:
  case FRU_PDB:
  case FRU_AVA1:
  case FRU_AVA2:
    switch(thresh) {
    case UCR_THRESH:
      *val = sensor_map[sensor_num].snr_thresh.ucr_thresh;
      break;
    case UNC_THRESH:
      *val = sensor_map[sensor_num].snr_thresh.unc_thresh;
      break;
    case UNR_THRESH:
      *val = sensor_map[sensor_num].snr_thresh.unr_thresh;
      break;
    case LCR_THRESH:
      *val = sensor_map[sensor_num].snr_thresh.lcr_thresh;
      break;
    case LNC_THRESH:
      *val = sensor_map[sensor_num].snr_thresh.lnc_thresh;
      break;
    case LNR_THRESH:
      *val = sensor_map[sensor_num].snr_thresh.lnr_thresh;
      break;
    case POS_HYST:
      *val = sensor_map[sensor_num].snr_thresh.pos_hyst;
      break;
    case NEG_HYST:
      *val = sensor_map[sensor_num].snr_thresh.neg_hyst;
      break;
    default:
      syslog(LOG_WARNING, "Threshold type error value=%d\n", thresh);
      return -1;
    }
    break;
  default:
    return -1;
  }
  return 0;
}

int
pal_get_sensor_units(uint8_t fru, uint8_t sensor_num, char *units) {
  uint8_t scale = sensor_map[sensor_num].units;

  switch(fru) {
    case FRU_MB:
    case FRU_PDB:
    case FRU_AVA1:
    case FRU_AVA2:
      switch(scale) {
        case TEMP:
          sprintf(units, "C");
          break;
        case FAN:
          sprintf(units, "RPM");
          break;
        case VOLT:
          sprintf(units, "Volts");
          break;
        case CURR:
          sprintf(units, "Amps");
          break;
        case POWER:
          sprintf(units, "Watts");
          break;
        default:
          return -1;
      }
      break;
    default:
      return -1;
  }
  return 0;
}

int pal_get_fru_sensor_list(uint8_t fru, uint8_t **sensor_list, int *cnt)
{
  if (fru == FRU_MB) {
    *sensor_list = (uint8_t *) mb_sensor_list;
    *cnt = mb_sensor_cnt;
  } else if (fru == FRU_PDB) {
    *sensor_list = (uint8_t *) pdb_sensor_list;
    *cnt = pdb_sensor_cnt;
  } else if (fru == FRU_AVA1) {
    *sensor_list = (uint8_t *) ava1_sensor_list;
    *cnt = ava1_sensor_cnt;
  } else if (fru == FRU_AVA2) {
    *sensor_list = (uint8_t *) ava2_sensor_list;
    *cnt = ava2_sensor_cnt;
  } else if (fru == FRU_BSM) {
    *sensor_list = NULL;
    *cnt = 0;
  } else {
    *sensor_list = NULL;
    *cnt = 0;
    return -1;
  }

  return 0;
}

int pal_set_fan_speed(uint8_t fan, uint8_t pwm)
{
  char label[32] = {0};

  if (fan >= pal_pwm_cnt ||
      snprintf(label, sizeof(label), "pwm%d", fan + 1) > sizeof(label)) {
    return -1;
  }
  return sensors_write_fan(label, (float)pwm);
}

int pal_get_fan_speed(uint8_t fan, int *rpm)
{
  char label[32] = {0};
  float value;
  int ret;

  if (fan >= pal_tach_cnt ||
      snprintf(label, sizeof(label), "fan%d", fan + 1) > sizeof(label)) {
    syslog(LOG_WARNING, "%s: invalid fan#:%d", __func__, fan);
    return -1;
  }
  ret = sensors_read_fan(label, &value);
  *rpm = (int)value;
  return ret;
}

int pal_get_fan_name(uint8_t num, char *name)
{
  if (num >= pal_tach_cnt) {
    syslog(LOG_WARNING, "%s: invalid fan#:%d", __func__, num);
    return -1;
  }

  sprintf(name, "Fan %d %s", num/2, num%2==0? "In":"Out");

  return 0;
}

int pal_get_pwm_value(uint8_t fan, uint8_t *pwm)
{
  char label[32] = {0};
  float value;
  int ret;

  if (fan >= pal_tach_cnt ||
      snprintf(label, sizeof(label), "pwm%d", fan/2 + 1) > sizeof(label)) {
    syslog(LOG_WARNING, "%s: invalid fan#:%d", __func__, fan);
    return -1;
  }
  ret = sensors_read_fan(label, &value);
  if (ret == 0)
    *pwm = (int)value;
  return ret;
}

static int
read_inlet_sensor(uint8_t id, float *value) {
  struct {
    const char *chip;
    const char *label;
  } devs[] = {
    {"tmp421-i2c-6-4c", "INLET_TEMP_L"},
    {"tmp421-i2c-6-4c", "INLET_REMOTE_TEMP_L"},
    {"tmp421-i2c-6-4f", "INLET_TEMP_R"},
    {"tmp421-i2c-6-4f", "INLET_REMOTE_TEMP_R"},
    {"tmp421-i2c-8-4f", "SYSTEM_INLET_TEMP"},
    {"tmp421-i2c-8-4f", "SYSTEM_INLET_REMOTE_TEMP"},
  };
  if (id >= ARRAY_SIZE(devs)) {
    return -1;
  }

  return sensors_read(devs[id].chip, devs[id].label, value);
}

static bool
is_fan_present(uint8_t fan_id) {
  int fd = 0, ret = -1;
  char fn[32];
  bool value = false;
  uint8_t retry = 3, tlen, rlen, addr, bus;
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};

  bus = 5;
  addr = 0xEE;

  snprintf(fn, sizeof(fn), "/dev/i2c-%d", bus);
  fd = open(fn, O_RDWR);
  if (fd < 0) {
    goto err_exit;
  }

  tbuf[0] = 0x01;
  tlen = 1;
  rlen = 1;

  while (ret < 0 && retry-- > 0 ) {
    ret = i2c_rdwr_msg_transfer(fd, addr, tbuf, tlen, rbuf, rlen);
  }

  if (ret < 0) {
    goto err_exit;
  }

  value = rbuf[0] & (0x1 << fan_id);

  err_exit:
  if (fd > 0) {
    close(fd);
  }
  return value;
}

static int
read_fan_volt(uint8_t fan_id, float *value) {
  struct {
    const char *chip;
    const char *label;
  } devs[] = {
    {"adc128d818-i2c-5-1d", "FAN0_VOLT"},
    {"adc128d818-i2c-5-1d", "FAN1_VOLT"},
    {"adc128d818-i2c-5-1d", "FAN2_VOLT"},
    {"adc128d818-i2c-5-1d", "FAN3_VOLT"},
  };
  int ret = 0;

  if (fan_id >= ARRAY_SIZE(devs)) {
    return -1;
  }
  if (is_fan_present(fan_id) == true) {
    return READING_NA;
  }
  ret = sensors_read(devs[fan_id].chip, devs[fan_id].label, value);

  fan_volt[fan_id] = *value;
  return ret;
}

static int
read_fan_curr(uint8_t fan_id, float *value) {
  struct {
    const char *chip;
    const char *label;
  } devs[] = {
    {"adc128d818-i2c-5-1d", "FAN0_CURR"},
    {"adc128d818-i2c-5-1d", "FAN1_CURR"},
    {"adc128d818-i2c-5-1d", "FAN2_CURR"},
    {"adc128d818-i2c-5-1d", "FAN3_CURR"},
  };
  int ret = 0;

  if (fan_id >= ARRAY_SIZE(devs)) {
    return -1;
  }
  if (is_fan_present(fan_id) == true) {
    return READING_NA;
  }
  ret = sensors_read(devs[fan_id].chip, devs[fan_id].label, value);

  fan_curr[fan_id] = *value;
  return ret;
}

static int
read_fan_pwr(uint8_t fan_id, float *value) {
  if (is_fan_present(fan_id) == true) {
    return READING_NA;
  }
  *value = fan_volt[fan_id] * fan_curr[fan_id];
  return 0;
}

static int
read_adc_value(uint8_t adc_id, float *value) {
  const char *adc_label[] = {
    "MB_P12V_AUX",
    "MB_P3V3_STBY",
    "MB_P5V_STBY",
    "MB_P3V3",
    "MB_P3V3_PAX",
    "MB_P3V_BAT",
    "MB_P2V5_AUX",
    "MB_P1V2_AUX",
    "MB_P1V15_AUX",
  };
  if (adc_id >= ARRAY_SIZE(adc_label)) {
    return -1;
  }

  return sensors_read_adc(adc_label[adc_id], value);
}

static int
read_bat_value(uint8_t adc_id, float *value) {
  int ret = -1;
  gpio_desc_t *gp_batt = gpio_open_by_shadow(GPIO_BATTERY_DETECT);
  if (!gp_batt) {
    return -1;
  }
  if (gpio_set_value(gp_batt, GPIO_VALUE_HIGH)) {
    goto exit;
  }

#ifdef DEBUG
  syslog(LOG_DEBUG, "%s %s\n", __func__, path);
#endif
  msleep(10);

  ret = read_adc_value(adc_id, value);
  if (gpio_set_value(gp_batt, GPIO_VALUE_LOW)) {
    goto exit;
  }

exit:
  gpio_close(gp_batt);
  return ret;
}

static bool
is_nic_present(uint8_t nic_id) {
  gpio_desc_t *gdesc = NULL;
  gpio_value_t val;
  char gpio_name[32];

  sprintf(gpio_name, GPIO_NIC_PRSNT, nic_id);

  if ((gdesc = gpio_open_by_shadow(gpio_name))) {
    if (!gpio_get_value(gdesc, &val)) {
      gpio_close(gdesc);
      return val;
    }
    gpio_close(gdesc);
  }

  return 0;
}

static int
read_nic_temp(uint8_t nic_id, float *value) {
  int fd = 0, ret = -1;
  char fn[32];
  static uint8_t retry=0;
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};
  uint8_t tlen, rlen, addr, bus;

  if (is_nic_present(nic_id) == true) {
    return READING_NA;
  }

  bus = nic_info_list[nic_id].bus;
  addr = nic_info_list[nic_id].slv_addr;

  snprintf(fn, sizeof(fn), "/dev/i2c-%d", bus);
  fd = open(fn, O_RDWR);
  if (fd < 0) {
    goto err_exit;
  }

  //Temp Register
  tbuf[0] = 0x01;
  tlen = 1;
  rlen = 1;

  ret = i2c_rdwr_msg_transfer(fd, addr, tbuf, tlen, rbuf, rlen);
  if( ret < 0 || (rbuf[0] == 0x80) ) {
    retry++;
    if (retry < 3) {
      ret = READING_SKIP;
    } else {
      ret = READING_NA;
    }
    goto err_exit;
  } else {
    retry=0;
  }

#ifdef DEBUG
  syslog(LOG_DEBUG, "%s Temp[%d]=%x bus=%x slavaddr=%x\n", __func__, nic_id, rbuf[0], bus, addr);
#endif

  *value = (float)rbuf[0];

err_exit:
  if (fd > 0) {
    close(fd);
  }
  return ret;
}

static int
get_hsc_reading(uint8_t hsc_id, uint8_t type, uint8_t cmd, float *value, uint8_t *raw_data) {
  const uint8_t adm1278_bus = 5;
  uint8_t addr = hsc_info_list[hsc_id].slv_addr;
  static int fd = -1;

  if ( fd < 0 ) {
    fd = i2c_cdev_slave_open(adm1278_bus, addr >> 1, I2C_SLAVE_FORCE_CLAIM);
    if ( fd < 0 ) {
      syslog(LOG_WARNING, "Failed to open bus %d", adm1278_bus);
      return READING_NA;
    }
  }

  uint8_t rbuf[2] = {0x00};
  uint8_t rlen = 2;
  int retry = MAX_SNR_READ_RETRY;
  int ret = ERR_NOT_READY;
  while ( ret < 0 && retry-- > 0 ) {
    ret = i2c_rdwr_msg_transfer(fd, addr, &cmd, 1, rbuf, rlen);
  }

  if ( ret < 0 ) {
    if ( fd >= 0 ) {
      close(fd);
      fd = -1;
    }
    return READING_NA;
  }

  float m = hsc_info_list[hsc_id].info[type].m;
  float b = hsc_info_list[hsc_id].info[type].b;
  float r = hsc_info_list[hsc_id].info[type].r;
  *value = ((float)(rbuf[1] << 8 | rbuf[0]) * r - b) / m;

#ifdef DEBUG
  syslog(LOG_WARNING, "cmd %d, rbuf[0] = %x, rbuf[1] %x, value = %f", cmd, rbuf[0], rbuf[1], *value);
#endif

  return PAL_EOK;
}

static int
read_hsc_temp(uint8_t hsc_id, float *value) {
  if ( get_hsc_reading(hsc_id, HSC_TEMP, PMBUS_READ_TEMP1, value, NULL) < 0 ) return READING_NA;
  return PAL_EOK;
}

static int
read_hsc_pin(uint8_t hsc_id, float *value) {
  if ( get_hsc_reading(hsc_id, HSC_POWER, PMBUS_READ_PIN, value, NULL) < 0 ) return READING_NA;
  *value *= 0.99; //improve the accuracy of PIN to +-2%
  return PAL_EOK;
}

static int
read_hsc_iout(uint8_t hsc_id, float *value) {
  if ( get_hsc_reading(hsc_id, HSC_CURRENT, PMBUS_READ_IOUT, value, NULL) < 0 ) return READING_NA;
  *value *= 0.99; //improve the accuracy of IOUT to +-2%
  return PAL_EOK;
}

static int
read_hsc_vin(uint8_t hsc_id, float *value) {
  if ( get_hsc_reading(hsc_id, HSC_VOLTAGE, PMBUS_READ_VIN, value, NULL) < 0 ) return READING_NA;
  return PAL_EOK;
}

static void
hsc_value_adjust(struct calibration_table *table, float *value) {
  float x0, x1, y0, y1, x;
  int i;
  x = *value;
  x0 = table[0].ein;
  y0 = table[0].coeff;
  if (x0 > *value) {
    *value = x * y0;
    return;
  }

  for (i = 0; table[i].ein > 0.0; i++) {
    if (*value < table[i].ein)
      break;
    x0 = table[i].ein;
    y0 = table[i].coeff;
  }

  if (table[i].ein <= 0.0) {
    *value = x * y0;
    return;
  }

  //if value is bwtween x0 and x1, use linear interpolation method.
  x1 = table[i].ein;
  y1 = table[i].coeff;
  *value = (y0 + (((y1 - y0)/(x1 - x0)) * (x - x0))) * x;

  return;
}

static int
read_pax_therm(uint8_t pax_id, float *value) {
  int ret = 0;
  struct {
    const char *chip;
    const char *label;
  } devs[] = {
    {"tmp422-i2c-6-4d", "PAX0_THERM_REMOTE"},
    {"tmp422-i2c-6-4d", "PAX1_THERM_REMOTE"},
    {"tmp422-i2c-6-4e", "PAX2_THERM_REMOTE"},
    {"tmp422-i2c-6-4e", "PAX3_THERM_REMOTE"},
  };

  ret = sensors_read(devs[pax_id].chip, devs[pax_id].label, value);

  return ret;
}

static int
read_bay_temp(uint8_t bay_id, float *value) {
  int ret = 0;
  struct {
    const char *chip;
    const char *label;
  } devs[] = {
    {"tmp75-i2c-21-48", "BAY0_FTEMP"},
    {"tmp75-i2c-21-49", "BAY0_RTEMP"},
    {"tmp75-i2c-22-48", "BAY1_FTEMP"},
    {"tmp75-i2c-22-49", "BAY1_RTEMP"},
  };
  ret = sensors_read(devs[bay_id].chip, devs[bay_id].label, value);

  return ret;
}

static int
read_nvme_temp(uint8_t sensor_num, float *value) {
  int ret = 0;
  int bus_id = 0, nvme_id = 0;
  int fd = 0;
  char fn[32];
  uint8_t tcount = 0, rcount = 0;
  uint8_t tbuf[16] = {0};
  uint8_t rbuf[16] = {0};

  if (sensor_num < BAY_1_FTEMP) {
    nvme_id = sensor_num - BAY_0_0_NVME_CTEMP;
    bus_id = I2C_BUS_21;
  } else {
    nvme_id = sensor_num - BAY_1_0_NVME_CTEMP;
    bus_id = I2C_BUS_22;
  }

  pal_control_mux_to_target_ch(1 << nvme_id, bus_id, 0xE6);

  snprintf(fn, sizeof(fn), "/dev/i2c-%d", bus_id);
  fd = open(fn, O_RDWR);
  if (fd < 0) {
    return READING_NA;
  }

  ret = 0;
  tbuf[0] = NVMe_GET_STATUS_CMD;
  tcount = 1;
  rcount = NVMe_GET_STATUS_LEN;
  ret = i2c_rdwr_msg_transfer(fd, NVMe_SMBUS_ADDR, tbuf, tcount, rbuf, rcount);
  if (ret < 0) {
    ret = READING_NA;
    goto error_exit;
  }
  *value = (float)(signed char)rbuf[NVMe_TEMP_REG];

error_exit:
  close(fd);

  return PAL_EOK;
}

static int sensors_read_vr(uint8_t sensor_num, float *value)
{
  int ret = 0;

  switch (sensor_num) {
    case MB_VR_P0V8_VDD0_TEMP:
      ret = sensors_read("mpq8645p-i2c-5-30", "VR_P0V8_VDD0_TEMP", value);
      break;
    case MB_VR_P0V8_VDD0_VOUT:
      ret = sensors_read("mpq8645p-i2c-5-30", "VR_P0V8_VDD0_VOUT", value);
      vr_vdd_volt[VR_ID0] = *value;
      break;
    case MB_VR_P0V8_VDD0_CURR:
      ret = sensors_read("mpq8645p-i2c-5-30", "VR_P0V8_VDD0_CURR", value);
      vr_vdd_curr[VR_ID0] = *value;
      break;
    case MB_VR_P0V8_VDD0_POWER:
      *value = vr_vdd_volt[VR_ID0] * vr_vdd_curr[VR_ID0];
      break;
    case MB_VR_P0V8_VDD1_TEMP:
      ret = sensors_read("mpq8645p-i2c-5-31", "VR_P0V8_VDD1_TEMP", value);
      break;
    case MB_VR_P0V8_VDD1_VOUT:
      ret = sensors_read("mpq8645p-i2c-5-31", "VR_P0V8_VDD1_VOUT", value);
      vr_vdd_volt[VR_ID1] = *value;
      break;
    case MB_VR_P0V8_VDD1_CURR:
      ret = sensors_read("mpq8645p-i2c-5-31", "VR_P0V8_VDD1_CURR", value);
      vr_vdd_curr[VR_ID1] = *value;
      break;
    case MB_VR_P0V8_VDD1_POWER:
      *value = vr_vdd_volt[VR_ID1] * vr_vdd_curr[VR_ID1];
      break;
    case MB_VR_P0V8_VDD2_TEMP:
      ret = sensors_read("mpq8645p-i2c-5-32", "VR_P0V8_VDD2_TEMP", value);
      break;
    case MB_VR_P0V8_VDD2_VOUT:
      ret = sensors_read("mpq8645p-i2c-5-32", "VR_P0V8_VDD2_VOUT", value);
      vr_vdd_volt[VR_ID2] = *value;
      break;
    case MB_VR_P0V8_VDD2_CURR:
      ret = sensors_read("mpq8645p-i2c-5-32", "VR_P0V8_VDD2_CURR", value);
      vr_vdd_curr[VR_ID2] = *value;
      break;
    case MB_VR_P0V8_VDD2_POWER:
      *value = vr_vdd_volt[VR_ID2] * vr_vdd_curr[VR_ID2];
      break;
    case MB_VR_P0V8_VDD3_TEMP:
      ret = sensors_read("mpq8645p-i2c-5-33", "VR_P0V8_VDD3_TEMP", value);
      break;
    case MB_VR_P0V8_VDD3_VOUT:
      ret = sensors_read("mpq8645p-i2c-5-33", "VR_P0V8_VDD3_VOUT", value);
      vr_vdd_volt[VR_ID3] = *value;
      break;
    case MB_VR_P0V8_VDD3_CURR:
      ret = sensors_read("mpq8645p-i2c-5-33", "VR_P0V8_VDD3_CURR", value);
      vr_vdd_curr[VR_ID3] = *value;
      break;
    case MB_VR_P0V8_VDD3_POWER:
      *value = vr_vdd_volt[VR_ID3] * vr_vdd_curr[VR_ID3];
      break;
    case MB_VR_P1V0_AVD0_TEMP:
      ret = sensors_read("mpq8645p-i2c-5-34", "VR_P1V0_AVD0_TEMP", value);
      break;
    case MB_VR_P1V0_AVD0_VOUT:
      ret = sensors_read("mpq8645p-i2c-5-34", "VR_P1V0_AVD0_VOUT", value);
      vr_avd_volt[VR_ID0] = *value;
      break;
    case MB_VR_P1V0_AVD0_CURR:
      ret = sensors_read("mpq8645p-i2c-5-34", "VR_P1V0_AVD0_CURR", value);
      vr_avd_curr[VR_ID0] = *value;
      break;
    case MB_VR_P1V0_AVD0_POWER:
      *value = vr_avd_volt[VR_ID0] * vr_avd_curr[VR_ID0];
      break;
    case MB_VR_P1V0_AVD1_TEMP:
      ret = sensors_read("mpq8645p-i2c-5-35", "VR_P1V0_AVD1_TEMP", value);
      break;
    case MB_VR_P1V0_AVD1_VOUT:
      ret = sensors_read("mpq8645p-i2c-5-35", "VR_P1V0_AVD1_VOUT", value);
      vr_avd_volt[VR_ID1] = *value;
      break;
    case MB_VR_P1V0_AVD1_CURR:
      ret = sensors_read("mpq8645p-i2c-5-35", "VR_P1V0_AVD1_CURR", value);
      vr_avd_curr[VR_ID1] = *value;
      break;
    case MB_VR_P1V0_AVD1_POWER:
      *value = vr_avd_volt[VR_ID1] * vr_avd_curr[VR_ID1];
      break;
    case MB_VR_P1V0_AVD2_TEMP:
      ret = sensors_read("mpq8645p-i2c-5-36", "VR_P1V0_AVD2_TEMP", value);
      break;
    case MB_VR_P1V0_AVD2_VOUT:
      ret = sensors_read("mpq8645p-i2c-5-36", "VR_P1V0_AVD2_VOUT", value);
      vr_avd_volt[VR_ID2] = *value;
      break;
    case MB_VR_P1V0_AVD2_CURR:
      ret = sensors_read("mpq8645p-i2c-5-36", "VR_P1V0_AVD2_CURR", value);
      vr_avd_curr[VR_ID2] = *value;
      break;
    case MB_VR_P1V0_AVD2_POWER:
      *value = vr_avd_volt[VR_ID2] * vr_avd_curr[VR_ID2];
      break;
    case MB_VR_P1V0_AVD3_TEMP:
      ret = sensors_read("mpq8645p-i2c-5-3b", "VR_P1V0_AVD3_TEMP", value);
      break;
    case MB_VR_P1V0_AVD3_VOUT:
      ret = sensors_read("mpq8645p-i2c-5-3b", "VR_P1V0_AVD3_VOUT", value);
      vr_avd_volt[VR_ID3] = *value;
      break;
    case MB_VR_P1V0_AVD3_CURR:
      ret = sensors_read("mpq8645p-i2c-5-3b", "VR_P1V0_AVD3_CURR", value);
      vr_avd_curr[VR_ID3] = *value;
      break;
    case MB_VR_P1V0_AVD3_POWER:
      *value = vr_avd_volt[VR_ID3] * vr_avd_curr[VR_ID3];
      break;

    default:
      ret = ERR_SENSOR_NA;
  }

  return ret < 0? ERR_SENSOR_NA: 0;
}

static int
read_nic_volt(uint8_t nic_id, float *value) {
  struct {
    const char *chip;
    const char *label;
  } devs[] = {
    {"adc128d818-i2c-23-1d", "NIC0_VOLT"},
    {"adc128d818-i2c-23-1d", "NIC1_VOLT"},
    {"adc128d818-i2c-23-1d", "NIC2_VOLT"},
    {"adc128d818-i2c-23-1d", "NIC3_VOLT"},
    {"adc128d818-i2c-23-1f", "NIC4_VOLT"},
    {"adc128d818-i2c-23-1f", "NIC5_VOLT"},
    {"adc128d818-i2c-23-1f", "NIC6_VOLT"},
    {"adc128d818-i2c-23-1f", "NIC7_VOLT"},
  };
  int ret = 0;

  if (is_nic_present(nic_id) == true) {
    return READING_NA;
  }
  ret = sensors_read(devs[nic_id].chip, devs[nic_id].label, value);

  nic_volt[nic_id] = *value;
  return ret;
}

static int
read_nic_curr(uint8_t nic_id, float *value) {
  struct {
    const char *chip;
    const char *label;
  } devs[] = {
    {"adc128d818-i2c-23-1d", "NIC0_CURR"},
    {"adc128d818-i2c-23-1d", "NIC1_CURR"},
    {"adc128d818-i2c-23-1d", "NIC2_CURR"},
    {"adc128d818-i2c-23-1d", "NIC3_CURR"},
    {"adc128d818-i2c-23-1f", "NIC4_CURR"},
    {"adc128d818-i2c-23-1f", "NIC5_CURR"},
    {"adc128d818-i2c-23-1f", "NIC6_CURR"},
    {"adc128d818-i2c-23-1f", "NIC7_CURR"},
  };
  int ret = 0;

  if (is_nic_present(nic_id) == true) {
    return READING_NA;
  }
  ret = sensors_read(devs[nic_id].chip, devs[nic_id].label, value);

  nic_curr[nic_id] = *value;
  return ret;
}

static int
read_nic_pwr(uint8_t nic_id, float *value) {
  if (is_nic_present(nic_id) == true) {
    return READING_NA;
  }
  *value = nic_volt[nic_id] * nic_curr[nic_id];
  return 0;
}

static int
read_nvme_volt(uint8_t nvme_id, float *value) {
  struct {
    const char *chip;
    const char *label;
  } devs[] = {
    {"adc128d818-i2c-23-37", "NVME1_VOLT"},
    {"adc128d818-i2c-23-37", "NVME2_VOLT"},
  };
  int ret = 0;

  ret = sensors_read(devs[nvme_id].chip, devs[nvme_id].label, value);

  nvme_volt[nvme_id] = *value;
  return ret;
}

static int
read_nvme_curr(uint8_t nvme_id, float *value) {
  struct {
    const char *chip;
    const char *label;
  } devs[] = {
    {"adc128d818-i2c-23-37", "NVME1_CURR"},
    {"adc128d818-i2c-23-37", "NVME2_CURR"},
  };
  int ret = 0;

  ret = sensors_read(devs[nvme_id].chip, devs[nvme_id].label, value);

  nvme_curr[nvme_id] = *value;
  return ret;
}

static int
read_nvme_pwr(uint8_t nvme_id, float *value) {
  *value = nvme_volt[nvme_id] * nvme_curr[nvme_id];
  return 0;
}
