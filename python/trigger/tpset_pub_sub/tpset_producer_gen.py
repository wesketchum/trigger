# Set moo schema search path
from dunedaq.env import get_moo_model_path
import moo.io
moo.io.default_load_path = get_moo_model_path()

import moo.otypes

from pprint import pprint
pprint(moo.otypes.load_types('trigger/triggerprimitivemaker.jsonnet'))

import dunedaq.trigger.triggerprimitivemaker as tpm

from ..util import module, modulegraph, direction

# ===============================================================================

def generate(
        INPUT_FILE: str,
        SLOWDOWN_FACTOR: float,
):
    CLOCK_FREQUENCY_HZ = 50000000 / SLOWDOWN_FACTOR

    modules = {
        "tpm": module(plugin="TriggerPrimitiveMaker",
                      connections={},
                      conf=tpm.ConfParams(filename=INPUT_FILE,
                                          number_of_loops=-1,  # Infinite
                                          tpset_time_offset=0,
                                          tpset_time_width=10000,  # 0.2 ms
                                          clock_frequency_hz=CLOCK_FREQUENCY_HZ),
                      )
    }

    mgraph = modulegraph(modules)
    mgraph.add_endpoint("tpsets_out", "tpm.tpset_sink", direction.OUT)
    
    return mgraph
