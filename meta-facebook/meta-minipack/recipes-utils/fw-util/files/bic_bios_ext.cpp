#include "fw-util.h"
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include "bic_bios.h"
#include <openbmc/pal.h>
#ifdef BIC_SUPPORT
#include <facebook/bic.h>

using namespace std;

class BiosExtComponent : public BiosComponent {
  uint8_t slot_id = 0;
  Server server;
  public:
    BiosExtComponent(string fru, string comp, uint8_t _slot_id)
      : BiosComponent(fru, comp, _slot_id), slot_id(_slot_id), server(_slot_id, fru) {}
    int update(string image);
};

int BiosExtComponent::update(string image) {
  int ret;
  uint8_t status;
  int retry_count = 60;

  try {
    server.ready();
    pal_set_server_power(slot_id, SERVER_GRACEFUL_SHUTDOWN);

    /* Checking Server Power Status to make sure Server is really Off */
    while (retry_count > 0) {
      ret = pal_get_server_power(slot_id, &status);
      if ((ret == 0) && (status == SERVER_POWER_OFF)) {
        break;
      }
      if ((--retry_count) > 0)
        sleep(1);
    }
    if (retry_count <= 0) {
      cerr << "Failed to Power Off Server " << (int)slot_id
           << ". Stopping the update!" << endl;
      return -1;
    }

    me_recovery(slot_id, RECOVERY_MODE);
    sleep(1);
    ret = bic_update_fw(slot_id, UPDATE_BIOS, (char *)image.c_str());
    sleep(3);
    pal_set_server_power(slot_id, SERVER_POWER_CYCLE);
    sleep(5);
    me_recovery(slot_id, RESTORE_FACTORY_DEFAULT);
  } catch(string err) {
    return FW_STATUS_NOT_SUPPORTED;
  }
  return ret;
}

BiosExtComponent bios("scm", "bios", 0);

#endif
