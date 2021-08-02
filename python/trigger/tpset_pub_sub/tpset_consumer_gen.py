# Set moo schema search path
from ..util import module
import moo.otypes
from dunedaq.env import get_moo_model_path
import moo.io
moo.io.default_load_path = get_moo_model_path()

# Load configuration types


# ===============================================================================

def generate(
):
    modules = {
        "tps_sink": module(plugin="TPSetSink",
                           conf=None,
                           connections={})
    }
    return modules
