#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <openbmc/pal.h>
#include <syslog.h>
#include "usbdbg.h"
#include "nic_ext.h"
#include "vr_fw.h"
#include "bios.h"

class palBiosComponent : public BiosComponent {
  public:
    palBiosComponent(std::string fru, std::string comp, std::string mtd, 
                     std::string devpath, std::string dev, std::string shadow,
                     bool level, std::string verp) :
      BiosComponent(fru, comp, mtd, devpath, dev, shadow, level, verp) {}
    int reboot(uint8_t fruid) override;
    int update_finish(void) override;
};

int palBiosComponent::reboot(uint8_t fruid) {
  
  // Let mb will be ON after AC cycle
  if (sys.runcmd("/usr/bin/killall gpiod > /dev/null") != 0)
    syslog(LOG_DEBUG, "killall gpiod failed");
  if (sys.runcmd("/usr/local/bin/cfg-util pwr_server_last_state on > /dev/null") != 0)
    syslog(LOG_DEBUG, "Set pwr_server_last_state on failed");

  return 0;
}

int palBiosComponent::update_finish(void) {
  int ret = 0;
  if(pal_get_config_is_master()){
    ret = pal_sled_cycle();
  }
  return ret;
}

NicExtComponent nic0("nic", "nic0", "nic0_fw_ver", FRU_NIC0, 0, 0x00);  // fru_name, component, kv, fru_id, eth_index, ch_id
NicExtComponent nic1("nic", "nic1", "nic1_fw_ver", FRU_NIC1, 1, 0x20);

UsbDbgComponent usbdbg("ocpdbg", "mcu", "F0C", 0, 0x60, false);
UsbDbgBlComponent usbdbgbl("ocpdbg", "mcubl", 0, 0x60, 0x02);  // target ID of bootloader = 0x02

VrComponent vr_pch_pvnn("vr", "pch_pvnn", "VR_PCH_PVNN/P1V05");
VrComponent vr_cpu0_vccin("vr", "cpu0_vccin", "VR_CPU0_VCCIN/VCCSA");
VrComponent vr_cpu0_vccio("vr", "cpu0_vccio", "VR_CPU0_VCCIO");
VrComponent vr_cpu0_vddq_abc("vr", "cpu0_vddq_abc", "VR_CPU0_VDDQ_ABC");
VrComponent vr_cpu0_vddq_def("vr", "cpu0_vddq_def", "VR_CPU0_VDDQ_DEF");
VrComponent vr_cpu1_vccin("vr", "cpu1_vccin", "VR_CPU1_VCCIN/VCCSA");
VrComponent vr_cpu1_vccio("vr", "cpu1_vccio", "VR_CPU1_VCCIO");
VrComponent vr_cpu1_vddq_abc("vr", "cpu1_vddq_abc", "VR_CPU1_VDDQ_ABC");
VrComponent vr_cpu1_vddq_def("vr", "cpu1_vddq_def", "VR_CPU1_VDDQ_DEF");

palBiosComponent bios("mb", "bios", "pnor", "/sys/bus/platform/drivers/aspeed-smc", "1e630000.spi", "FM_BIOS_SPI_BMC_CTRL", true, "(F0C_)(.*)");
