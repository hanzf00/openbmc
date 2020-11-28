#!/usr/bin/env python3

import re
import subprocess
from typing import Dict

from rest_utils import DEFAULT_TIMEOUT_SEC


# Handler for sys/firmware_info_scm resource endpoint
def get_firmware_info() -> Dict:
    return {"Information": get_fpga_ver_data(), "Actions": [], "Resources": []}


def _parse_fpga_ver_data(data) -> Dict:
    result = {}
    for sdata in data.splitlines():
        if re.match(r"^PIM", sdata):
            pim_id = int(sdata.replace("PIM ", "").replace(":", ""))
            result[pim_id] = {"status": "missing"}
        matches = re.match(r"(?P<type>[^ ]+) DOMFPGA: (?P<version>[^ ]+)", sdata)
        if matches:
            result[pim_id]["status"] = "present"
            result[pim_id]["version"] = matches.group("version")
            result[pim_id]["type"] = matches.group("type")
    return result


def get_fpga_ver_data() -> Dict:
    cmd = ["/usr/local/bin/fpga_ver.sh"]
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    data, _ = proc.communicate(timeout=DEFAULT_TIMEOUT_SEC)
    data = data.decode(errors="ignore")
    return _parse_fpga_ver_data(data)
