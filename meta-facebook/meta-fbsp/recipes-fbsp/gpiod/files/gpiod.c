/*
 * sensord
 *
 * Copyright 2015-present Facebook. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/file.h>
#include <pthread.h>
#include <openbmc/libgpio.h>
#include <openbmc/pal.h>
#include <openbmc/nm.h>
#include <openbmc/kv.h>

#define POLL_TIMEOUT -1
#define POWER_ON_STR        "on"
#define POWER_OFF_STR       "off"
#define SERVER_POWER_CHECK(power_on_time)  \
do {                                       \
  uint8_t status = 0;                      \
  pal_get_server_power(FRU_MB, &status);   \
  if (status != SERVER_POWER_ON) {         \
    return;                                \
  }                                        \
  if (g_power_on_sec < power_on_time) {    \
    return;                                \
  }                                        \
}while(0)
#define TOUCH(path) \
{\
  int fd = creat(path, 0644);\
  if (fd) close(fd);\
}

static uint8_t g_caterr_irq = 0;
static uint8_t g_msmi_irq = 0;
static bool g_mcerr_ierr_assert = false;
static int g_uart_switch_count = 0;
static long int g_reset_sec = 0;
static long int g_power_on_sec = 0;
static pthread_mutex_t timer_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t caterr_mutex = PTHREAD_MUTEX_INITIALIZER;

// For monitoring GPIOs on IO expender
struct gpioexppoll_config {
  char shadow[32];
  char desc[64];
  char pwr_st;
  gpio_value_t last;
  gpio_value_t curr;
  gpio_edge_t edge;
//  void (*handler)(char shadow, char desc, gpio_edge_t edge, gpio_value_t last, gpio_value_t curr);
  void (*init_gpio)(char shadow, char desc);
};

enum {
  STBY,
  PS_ON,
  PS_ON_3S,
};

//PCA9539 Address 0XEE
struct gpioexppoll_config ioex0_gpios[] = {
  {"IRQ_PVCCIN_CPU0_VRHOT_LVC3_N", "ExIO_0", PS_ON, GPIO_VALUE_HIGH, GPIO_VALUE_INVALID, GPIO_EDGE_FALLING},
  {"IRQ_PVCCIN_CPU1_VRHOT_LVC3_N", "ExIO_1", PS_ON, GPIO_VALUE_HIGH, GPIO_VALUE_INVALID, GPIO_EDGE_FALLING},
  {"IRQ_PVDDQ_ABC_CPU0_VRHOT_LVC3_N", "ExIO_2", PS_ON, GPIO_VALUE_HIGH, GPIO_VALUE_INVALID, GPIO_EDGE_FALLING},
  {"IRQ_PVDDQ_DEF_CPU0_VRHOT_LVC3_N", "ExIO_3", PS_ON, GPIO_VALUE_HIGH, GPIO_VALUE_INVALID, GPIO_EDGE_FALLING},
  {"IRQ_PVDDQ_ABC_CPU1_VRHOT_LVC3_N", "ExIO_4", PS_ON, GPIO_VALUE_HIGH, GPIO_VALUE_INVALID, GPIO_EDGE_FALLING},
  {"IRQ_PVDDQ_DEF_CPU1_VRHOT_LVC3_N", "ExIO_5", PS_ON, GPIO_VALUE_HIGH, GPIO_VALUE_INVALID, GPIO_EDGE_FALLING},
  {"FM_CPU0_SKTOCC_LVT3_PLD_N", "ExIO_6", STBY, GPIO_VALUE_LOW, GPIO_VALUE_INVALID, GPIO_EDGE_RISING},
  {"FM_CPU1_SKTOCC_LVT3_PLD_N", "ExIO_7", STBY, GPIO_VALUE_LOW, GPIO_VALUE_INVALID, GPIO_EDGE_RISING},
};

static gpio_value_t gpio_get(const char *shadow)
{
  gpio_value_t value = GPIO_VALUE_INVALID;
  gpio_desc_t *desc = gpio_open_by_shadow(shadow);
  if (!desc) {
    syslog(LOG_CRIT, "Open failed for GPIO: %s\n", shadow);
    return GPIO_VALUE_INVALID;
  }
  if (gpio_get_value(desc, &value)) {
    syslog(LOG_CRIT, "Get failed for GPIO: %s\n", shadow);
    value = GPIO_VALUE_INVALID;
  }
  gpio_close(desc);
  return value;
}

static void* ioex0_monitor()
{
  int i;
  uint8_t status;
  bool assert = false;

  //delay for system power on
  sleep(5);

  while (1) {

    for (i = 0; i < 8; i++) {
      if(ioex0_gpios[i].pwr_st != STBY) {
        if (pal_get_server_power(FRU_MB, &status) < 0 || status == SERVER_POWER_OFF) {
          continue;
        }
      }

      ioex0_gpios[i].curr = gpio_get(ioex0_gpios[i].shadow);

      if (ioex0_gpios[i].last == ioex0_gpios[i].curr) {
        ioex0_gpios[i].last = ioex0_gpios[i].curr;
        continue;
      }

      if (ioex0_gpios[i].edge == GPIO_EDGE_FALLING) {
        if (ioex0_gpios[i].last == GPIO_VALUE_HIGH && ioex0_gpios[i].curr == GPIO_VALUE_LOW) {
          assert = true;
        }
        else if (ioex0_gpios[i].last == GPIO_VALUE_LOW && ioex0_gpios[i].curr == GPIO_VALUE_HIGH ) {
          assert = false;
        }
      }
      else if (ioex0_gpios[i].edge == GPIO_EDGE_RISING) {
        if (ioex0_gpios[i].last == GPIO_VALUE_LOW && ioex0_gpios[i].curr == GPIO_VALUE_HIGH) {
          assert = true;
        }
        else if (ioex0_gpios[i].last == GPIO_VALUE_HIGH && ioex0_gpios[i].curr == GPIO_VALUE_LOW ) {
          assert = false;
        }
      }

      syslog(LOG_CRIT, "%s: %s - %s\n", assert? "ASSERT": "DEASSERT", ioex0_gpios[i].desc, ioex0_gpios[i].shadow);
      ioex0_gpios[i].last = ioex0_gpios[i].curr;
    }

    sleep(1);
  }

  return NULL;
}

static void
log_gpio_change(gpiopoll_pin_t *desc, gpio_value_t value, useconds_t log_delay) {
  const struct gpiopoll_config *cfg = gpio_poll_get_config(desc);
  assert(cfg);
  syslog(LOG_CRIT, "%s: %s - %s\n", value ? "DEASSERT": "ASSERT", cfg->description, cfg->shadow);
}

//PCH Thermtrip Handler
static void pch_thermtrip_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr)
{
  uint8_t status = 0;
  gpio_value_t value;

  value = gpio_get("RST_PLTRST_BMC_N");
  if(value < 0 || value == GPIO_VALUE_LOW) {
    return;
  }

  pal_get_server_power(FRU_MB, &status);
  if (status != SERVER_POWER_ON)
    return;
  // Filter false positives during reset.
  if (g_reset_sec < 10)
    return;
  log_gpio_change(desc, curr, 0);
}

//PCH Thermtrip Handler
static void fivr_fault_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr)
{
  uint8_t status = 0;
  gpio_value_t value;

  value = gpio_get("RST_PLTRST_BMC_N");
  if(value < 0 || value == GPIO_VALUE_LOW) {
    return;
  }

  pal_get_server_power(FRU_MB, &status);
  if (status != SERVER_POWER_ON)
    return;
  log_gpio_change(desc, curr, 0);
}


static void irq_uv_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr)
{
  log_gpio_change(desc, curr, 20*1000);
}

//Generic Event Handler for GPIO changes
static void gpio_event_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr)
{
  log_gpio_change(desc, curr, 0);
}

// Generic Event Handler for GPIO changes, but only logs event when MB is ON
static void gpio_event_pson_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr)
{
  SERVER_POWER_CHECK(0);
  log_gpio_change(desc, curr, 0);
}

// Generic Event Handler for GPIO changes, but only logs event when MB is ON 3S
static void gpio_event_pson_3s_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr)
{
  SERVER_POWER_CHECK(3);
  log_gpio_change(desc, curr, 0);
}

static void
set_smi_trigger(void) {
  static bool triggered = false;
  static gpio_desc_t *gpio = NULL;
  if (!gpio) {
    gpio = gpio_open_by_shadow("BMC_PPIN"); // GPIOB5
    if (!gpio)
      return;
  }

  if(!triggered) {
    gpio_set_value(gpio, GPIO_VALUE_LOW);
    usleep(1000);
    gpio_set_value(gpio, GPIO_VALUE_HIGH);
    triggered = true;
  }
}

static inline long int
reset_timer(long int *val) {
  pthread_mutex_lock(&timer_mutex);
  *val = 0;
  pthread_mutex_unlock(&timer_mutex);
  return *val;
}

static inline long int
increase_timer(long int *val) {
  pthread_mutex_lock(&timer_mutex);
  (*val)++;
  pthread_mutex_unlock(&timer_mutex);
  return *val;
}

static inline long int
decrease_timer(long int *val) {
  pthread_mutex_lock(&timer_mutex);
  (*val)--;
  pthread_mutex_unlock(&timer_mutex);
  return *val;
}

//Reset Button Event Handler
static void
pwr_reset_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  log_gpio_change(desc, curr, 0);
}

//Power Button Event Handler
static void
pwr_button_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  struct timespec ts;
  char value[MAX_VALUE_LEN];

  if (!curr) {
    clock_gettime(CLOCK_MONOTONIC, &ts);
    ts.tv_sec += 5;
    sprintf(value, "%ld", ts.tv_sec);
    kv_set("pwr_btn_press_time", value, 0, 0);
  }
  log_gpio_change(desc, curr, 0);
}

//System Power Ok Event Handler
static void
pwr_sysok_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  if(curr == GPIO_VALUE_HIGH){
    pal_set_post_complete(false);
  }

  reset_timer(&g_power_on_sec);
  log_gpio_change(desc, curr, 0);
}

//IERR and MCERR Event Handler
static void
init_caterr(gpiopoll_pin_t *desc, gpio_value_t value) {
  uint8_t status = 0;
  pal_get_server_power(FRU_MB, &status);
  if (status && value == GPIO_VALUE_LOW) {
    g_caterr_irq++;
  }
}

static void
init_msmi(gpiopoll_pin_t *desc, gpio_value_t value) {
  uint8_t status = 0;
  pal_get_server_power(FRU_MB, &status);
  if (status && value == GPIO_VALUE_LOW) {
    g_msmi_irq++;
  }
}

static void cpu0_thermtrip(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr)
{
  SERVER_POWER_CHECK(3);
  if (curr == GPIO_VALUE_HIGH) {
    pal_add_cri_sel("CPU0 thermtrip DEASSERT");
  } else {
    pal_add_cri_sel("CPU0 thermtrip ASSERT");
  }
  log_gpio_change(desc, curr, 0);
}

static void cpu1_thermtrip(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr)
{
  SERVER_POWER_CHECK(3);
  if (curr == GPIO_VALUE_HIGH) {
    pal_add_cri_sel("CPU1 thermtrip DEASSERT");
  } else {
    pal_add_cri_sel("CPU1 thermtrip ASSERT");
  }
  log_gpio_change(desc, curr, 0);
}

static void cpu0_mem_thermtrip(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr)
{
  if (curr == GPIO_VALUE_HIGH) {
    pal_add_cri_sel("CPU0 MEM thermtrip DEASSERT");
  } else {
    pal_add_cri_sel("CPU0 MEM thermtrip ASSERT");
  }
  log_gpio_change(desc, curr, 0);
}

static void cpu1_mem_thermtrip(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr)
{
  if (curr == GPIO_VALUE_HIGH) {
    pal_add_cri_sel("CPU1 MEM thermtrip DEASSERT");
  } else {
    pal_add_cri_sel("CPU1 MEM thermtrip ASSERT");
  }
  log_gpio_change(desc, curr, 0);
}

static void
err_caterr_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  SERVER_POWER_CHECK(3);
  pthread_mutex_lock(&caterr_mutex);
  g_caterr_irq++;
  pthread_mutex_unlock(&caterr_mutex);
}

static void
err_msmi_handler(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
  SERVER_POWER_CHECK(3);
  pthread_mutex_lock(&caterr_mutex);
  g_msmi_irq++;
  pthread_mutex_unlock(&caterr_mutex);
}

static int
ierr_mcerr_event_log(bool is_caterr, const char *err_type) {
  char temp_log[128] = {0};
  char temp_syslog[128] = {0};
  char cpu_str[32] = "";
  int num=0;
  int ret=0;

  ret = cmd_peci_get_cpu_err_num(&num, is_caterr);
  if(ret != 0) {
    syslog(LOG_ERR, "Can't Read MCA Log\n");
  }

  if (num == 2)
    strcpy(cpu_str, "0/1");
  else if (num != -1)
    sprintf(cpu_str, "%d", num);

  sprintf(temp_syslog, "ASSERT: CPU%s %s\n", cpu_str, err_type);
  syslog(LOG_CRIT, "%s", temp_syslog);

  sprintf(temp_log, "CPU%s %s", cpu_str, err_type);
  pal_add_cri_sel(temp_log);

  return ret;
}

static int trigger_hpr(void) {

  char value[MAX_VALUE_LEN] = {0};

  if (!kv_get("mb_trigger_hpr", value, NULL, KV_FPERSIST) && !strcmp(value, "off")) {
    return 0;
  }

  return 1;
}

static void *
ierr_mcerr_event_handler() {
  uint8_t caterr_cnt = 0;
  uint8_t msmi_cnt = 0;
  gpio_value_t value;
  gpio_desc_t *caterr = gpio_open_by_shadow("FM_CPU_CATERR_LVT3_N");
  gpio_desc_t *msmi = gpio_open_by_shadow("FM_CPU_MSMI_LVT3_N");

  if (!caterr) {
    gpio_close(caterr);
    return NULL;
  }
  if (!msmi) {
    gpio_close(msmi);
    return NULL;
  }

  while (1) {
    if (g_caterr_irq > 0) {
      caterr_cnt++;
      if (caterr_cnt == 2) {
        if (g_caterr_irq == 1) {
          g_mcerr_ierr_assert = true;

          if (gpio_get_value(caterr, &value)) {
            syslog(LOG_CRIT, "Getting CATERR GPIO failed");
            break;
          }

          if (value == GPIO_VALUE_LOW) {
            ierr_mcerr_event_log(true, "IERR/CATERR");
          } else {
            ierr_mcerr_event_log(true, "MCERR/CATERR");
          }

          pthread_mutex_lock(&caterr_mutex);
          g_caterr_irq--;
          pthread_mutex_unlock(&caterr_mutex);
          caterr_cnt = 0;
          pal_set_fault_led(FRU_MB, FAULT_LED_ON);
          if (system("/usr/local/bin/autodump.sh")) {
            syslog(LOG_WARNING, "Failed to start crashdump\n");
          }

          if (value == GPIO_VALUE_LOW && trigger_hpr()) {
            if (system("/usr/local/bin/power-util mb reset") != 0)
              syslog(LOG_WARNING, "Failed to power reset\n");
          }

        } else if (g_caterr_irq > 1) {
          while (g_caterr_irq > 1) {
            ierr_mcerr_event_log(true, "MCERR/CATERR");
            pthread_mutex_lock(&caterr_mutex);
            g_caterr_irq--;
            pthread_mutex_unlock(&caterr_mutex);
          }
          caterr_cnt = 1;

        }
      }
    }

    if (g_msmi_irq > 0) {
      msmi_cnt++;
      if (msmi_cnt == 2) {
        if (g_msmi_irq == 1) {
          g_mcerr_ierr_assert = true;

          if (gpio_get_value(msmi, &value)) {
            syslog(LOG_CRIT, "Getting MSMI GPIO failed");
            break;
          }

          if (value == GPIO_VALUE_LOW) {
            ierr_mcerr_event_log(false, "IERR/MSMI");
          } else {
            ierr_mcerr_event_log(false, "MCERR/MSMI");
          }

          pthread_mutex_lock(&caterr_mutex);
          g_msmi_irq--;
          pthread_mutex_unlock(&caterr_mutex);
          msmi_cnt = 0;
          pal_set_fault_led(FRU_MB, FAULT_LED_ON);
          if (system("/usr/local/bin/autodump.sh")) {
            syslog(LOG_WARNING, "Failed to start crashdump\n");
          }

          if (value == GPIO_VALUE_LOW && trigger_hpr()) {
            if (system("/usr/local/bin/power-util mb reset") != 0)
              syslog(LOG_WARNING, "Failed to power reset\n");
          }

        } else if (g_msmi_irq > 1) {
          while (g_msmi_irq > 1) {
            ierr_mcerr_event_log(false, "MCERR/MSMI");
            pthread_mutex_lock(&caterr_mutex);
            g_msmi_irq -= 1;
            pthread_mutex_unlock(&caterr_mutex);
          }
          msmi_cnt = 1;
        }
      }
    }
    usleep(25000); //25ms
  }

  gpio_close(caterr);
  gpio_close(msmi);
  return NULL;
}

//Uart Select on DEBUG Card Event Handler
static void
uart_select_handle(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr) {
   g_uart_switch_count = 2;
   log_gpio_change(desc, curr, 0);
   pal_uart_select_led_set();
}

// Event Handler for GPIOF6 platform reset changes
static void platform_reset_event_handle(gpiopoll_pin_t *desc, gpio_value_t last, gpio_value_t curr)
{
  // Use GPIOF6 to filter some gpio logging
  reset_timer(&g_reset_sec);
  TOUCH("/tmp/rst_touch");
  log_gpio_change(desc, curr, 0);
  if (g_mcerr_ierr_assert) {
    pal_second_crashdump_chk();
    g_mcerr_ierr_assert = false;
  }
}

// Thread for gpio timer
static void
*gpio_timer() {
  uint8_t status = 0;
  uint8_t fru = 1;
  long int pot;
  char str[MAX_VALUE_LEN] = {0};
  int tread_time = 0;
  struct timespec ts;

  while (1) {
    sleep(1);

    pal_get_server_power(fru, &status);
    if (status == SERVER_POWER_ON) {
      increase_timer(&g_reset_sec);
      pot = increase_timer(&g_power_on_sec);
    } else {
      pot = decrease_timer(&g_power_on_sec);
    }

    // Wait power-on.sh finished, then update pwr_server_last_state
    if (tread_time < 20) {
      tread_time++;
    } else {
      if (pal_get_last_pwr_state(fru, str) < 0)
        str[0] = '\0';
      if (status == SERVER_POWER_ON) {
        if (strncmp(str, POWER_ON_STR, strlen(POWER_ON_STR)) != 0) {
          pal_set_last_pwr_state(fru, POWER_ON_STR);
          syslog(LOG_INFO, "last pwr state updated to on\n");
          syslog(LOG_CRIT, "FRU: %d, System powered ON", fru);
        }

        memset(str, 0, sizeof(str));
        if (!kv_get("pwr_btn_press_time", str, NULL, 0)) {
          clock_gettime(CLOCK_MONOTONIC, &ts);
          if (strtoul(str, NULL, 10) > ts.tv_sec) {
            pal_set_restart_cause(FRU_MB, RESTART_CAUSE_PWR_ON_PUSH_BUTTON);
          }
        }
      } else {
        // wait until PowerOnTime < -2 to make sure it's not AC lost
        // Handle corner case during sled-cycle due to BMC residual electricity (1.2sec)
        if (pot < -2 && strncmp(str, POWER_OFF_STR, strlen(POWER_OFF_STR)) != 0) {
          pal_set_last_pwr_state(fru, POWER_OFF_STR);
          syslog(LOG_INFO, "last pwr state updated to off\n");
          syslog(LOG_CRIT, "FRU: %d, System powered OFF", fru);
        }
      }
      set_smi_trigger();
    }

    //Show Uart Debug Select Number 2sec
    if (g_uart_switch_count > 0) {
      if (--g_uart_switch_count == 0)
        pal_uart_select(AST_GPIO_BASE, UARTSW_OFFSET, UARTSW_BY_DEBUG, 0);
    }
  }

  return NULL;
}

// GPIO table to be monitored
static struct gpiopoll_config g_gpios[] = {
  // shadow, description, edge, handler, oneshot
  {"FP_BMC_RST_BTN_N", "GPIOE0", GPIO_EDGE_BOTH, pwr_reset_handler, NULL},
  {"FM_BMC_PWR_BTN_R_N", "GPIOE2", GPIO_EDGE_BOTH, pwr_button_handler, NULL},
  {"PWRGD_SYS_PWROK", "GPIOY2", GPIO_EDGE_BOTH, pwr_sysok_handler, NULL},
  {"FM_UARTSW_LSB_N", "GPIOL0", GPIO_EDGE_BOTH, uart_select_handle, NULL},
  {"FM_UARTSW_MSB_N", "GPIOL1", GPIO_EDGE_BOTH, uart_select_handle, NULL},
  {"FM_CPU0_THERMTRIP_LVT3_PLD_N", "GPIOA1", GPIO_EDGE_BOTH, cpu0_thermtrip, NULL},
  {"FM_CPU1_THERMTRIP_LVT3_PLD_N", "GPIOD0", GPIO_EDGE_BOTH, cpu1_thermtrip, NULL},
  {"FM_PCH_BMC_THERMTRIP_N", "GPIOG2", GPIO_EDGE_BOTH, pch_thermtrip_handler, NULL},
  {"RST_PLTRST_BMC_N", "GPIOF6", GPIO_EDGE_BOTH, platform_reset_event_handle, NULL},
  {"FM_SLPS3_N", "GPIOY0", GPIO_EDGE_BOTH, gpio_event_pson_handler, NULL},
  {"FM_SLPS4_N", "GPIOY1", GPIO_EDGE_BOTH, gpio_event_pson_handler, NULL},
  {"IRQ_OC_DETECT_N", "GPIOM0", GPIO_EDGE_BOTH, gpio_event_handler, NULL},
  {"IRQ_UV_DETECT_N", "GPIOM1", GPIO_EDGE_BOTH, irq_uv_handler, NULL},
  {"IRQ_HSC_FAULT_N", "GPIOL2", GPIO_EDGE_BOTH, gpio_event_handler, NULL},
  {"IRQ_SML1_PMBUS_BMC_ALERT_N", "GPIOAA1", GPIO_EDGE_BOTH, gpio_event_handler, NULL},
  {"FM_CPU_CATERR_LVT3_N", "GPIOZ0", GPIO_EDGE_FALLING, err_caterr_handler, init_caterr},
  {"FM_CPU_MSMI_LVT3_N", "GPIOZ2", GPIO_EDGE_FALLING, err_msmi_handler, init_msmi},
  {"FM_CPU0_MEMHOT_OUT_N", "GPIOU5", GPIO_EDGE_BOTH, gpio_event_pson_3s_handler, NULL},
  {"FM_CPU1_MEMHOT_OUT_N", "GPIOL3", GPIO_EDGE_BOTH, gpio_event_pson_3s_handler, NULL},
  {"FM_CPU0_FIVR_FAULT_LVT3_PLD", "GPIOB2", GPIO_EDGE_BOTH, fivr_fault_handler, NULL},
  {"FM_CPU1_FIVR_FAULT_LVT3_PLD", "GPIOB3", GPIO_EDGE_BOTH, fivr_fault_handler, NULL},
  {"FM_MEM_THERM_EVENT_CPU0_LVT3_N", "GPIOB0", GPIO_EDGE_BOTH, cpu0_mem_thermtrip, NULL},
  {"FM_MEM_THERM_EVENT_CPU1_LVT3_N", "GPIOB1", GPIO_EDGE_BOTH, cpu1_mem_thermtrip, NULL},
};

int main(int argc, char **argv)
{
  int rc, pid_file;
  gpiopoll_desc_t *polldesc;
  pthread_t tid_ierr_mcerr_event;
  pthread_t tid_gpio_timer;
  pthread_t tid_ioex0_monitor;
  char value[MAX_VALUE_LEN] = {0};

  pid_file = open("/var/run/gpiod.pid", O_CREAT | O_RDWR, 0666);
  rc = flock(pid_file, LOCK_EX | LOCK_NB);
  if (rc) {
    if (EWOULDBLOCK == errno) {
      syslog(LOG_ERR, "Another gpiod instance is running...\n");
      exit(-1);
    }
  } else {
    openlog("gpiod", LOG_CONS, LOG_DAEMON);
    syslog(LOG_INFO, "gpiod: daemon started");

    if (kv_get("mb_trigger_hpr", value, NULL, KV_FPERSIST) < 0) {
#ifdef DEBUG
    syslog(LOG_WARNING, "mb_trigger_hpr did not exist!\n");
#endif
      kv_set("mb_trigger_hpr", "on", 0, KV_FPERSIST);
    }

    //Create thread for IERR/MCERR event detect
    if (pthread_create(&tid_ierr_mcerr_event, NULL, ierr_mcerr_event_handler, NULL) < 0) {
      syslog(LOG_WARNING, "pthread_create for ierr_mcerr_event_handler\n");
      exit(1);
    }

    //Create thread for platform reset event filter check
    if (pthread_create(&tid_gpio_timer, NULL, gpio_timer, NULL) < 0) {
      syslog(LOG_WARNING, "pthread_create for platform_reset_filter_handler\n");
      exit(1);
    }

    if (pthread_create(&tid_ioex0_monitor, NULL, ioex0_monitor, NULL) < 0) {
      syslog(LOG_CRIT, "pthread_create for fan monitor error");
      exit(1);
    }


    polldesc = gpio_poll_open(g_gpios, sizeof(g_gpios)/sizeof(g_gpios[0]));
    if (!polldesc) {
      syslog(LOG_CRIT, "Cannot start poll operation on GPIOs");
    } else {
      if (gpio_poll(polldesc, POLL_TIMEOUT)) {
        syslog(LOG_CRIT, "Poll returned error");
      }
      gpio_poll_close(polldesc);
    }
  }

  return 0;
}
