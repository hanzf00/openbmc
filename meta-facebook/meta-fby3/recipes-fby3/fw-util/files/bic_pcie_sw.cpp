#include "fw-util.h"
#include <cstdio>
#include <cstring>
#include "server.h"
#include <openbmc/pal.h>
#include "bic_pcie_sw.h"
#ifdef BIC_SUPPORT
#include <facebook/bic.h>

using namespace std;

int PCIESWComponent::update(string image) {
  int ret = 0;
  try {
    server.ready();
    expansion.ready();
    ret = bic_update_fw(slot_id, fw_comp, (char *)image.c_str(), FORCE_UPDATE_UNSET);
  } catch (string& err) {
    cout << "Failed Reason: " << err << endl;
    return FW_STATUS_NOT_SUPPORTED;
  }
  return ret;
}

int PCIESWComponent::fupdate(string image) {
  return FW_STATUS_NOT_SUPPORTED;
}

int PCIESWComponent::get_ver_str(string& s, const uint8_t alt_fw_comp) {
  char ver[32] = {0};
  uint8_t rbuf[4] = {0};
  int ret = 0;
  ret = bic_get_fw_ver(slot_id, alt_fw_comp, rbuf);
  snprintf(ver, sizeof(ver), "%02X%02X%02X%02X", rbuf[0], rbuf[1], rbuf[2], rbuf[3]);
  s = string(ver);
  if ( alt_fw_comp != FW_2OU_PESW_CFG_VER && alt_fw_comp != FW_2OU_PESW_FW_VER ) {
    string tmp("(");
    if ( rbuf[0] == 0x00 ) tmp += "Inactive, ";
    else tmp += "Active, ";

    if ( rbuf[1] == 0x00 ) tmp += "Invalid)";
    else tmp += "Valid)";
    s += tmp;
  }
  return ret;
}

int PCIESWComponent::print_version() {
  map<uint8_t, string> list = {{FW_2OU_PESW_CFG_VER, "PCIE Switch Config"},
                               {FW_2OU_PESW_FW_VER,  "PCIE Switch Firmware"},
                               {FW_2OU_PESW_BL0_VER, "PCIE Bootloader0"},
                               {FW_2OU_PESW_BL1_VER, "PCIE Bootloader1"},
                               {FW_2OU_PESW_PART_MAP0_VER, "PCIE switch Partition0"},
                               {FW_2OU_PESW_PART_MAP1_VER, "PCIE switch Partition1"}};

  string ver("");
  string board_name = name;
  string err_msg("");

  transform(board_name.begin(), board_name.end(), board_name.begin(), ::toupper);
  try {
    server.ready();
    expansion.ready();
  } catch(string& err) {
    for ( auto& node:list ) {
      printf("%s %s Version: NA (%s)\n", board_name.c_str(), node.second.c_str(), err.c_str());
    }
    return FW_STATUS_SUCCESS;   
  }

  for ( auto& node:list ) {
    try {
      //Print PESW FWs
      if ( get_ver_str(ver, node.first) < 0 ) {
        throw "Error in getting the version of " + board_name;
      }
      cout << board_name << " " << node.second << " Version: " << ver << endl;
    } catch(string& err) {
      printf("%s %s Version: NA (%s)\n", board_name.c_str(), node.second.c_str(), err.c_str());
    }
  }
  return FW_STATUS_SUCCESS;
}

void PCIESWComponent::get_version(json& j) {
  return;
}
#endif

