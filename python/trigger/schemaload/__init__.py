# Configure facade module
import sys
from ._schemaload_facade import Facade
sys.modules[__name__] = Facade(sys.modules[__name__])
