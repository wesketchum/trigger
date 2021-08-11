# Set moo schema search path
from dunedaq.env import get_moo_model_path
import moo.io
moo.io.default_load_path = get_moo_model_path()

# Load configuration types
import moo.otypes

from ..util import module

# ===============================================================================

def generate(
):
    modules = {
        "tps_sink": module(plugin="TPSetSink",
                           conf=None,
                           connections={})
    }
    return modules
