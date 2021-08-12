# Set moo schema search path
from dunedaq.env import get_moo_model_path
import moo.io
moo.io.default_load_path = get_moo_model_path()

import moo.otypes

from importlib import import_module

import types
import sys

class Facade(types.ModuleType):
    """Facade class for my module"""

    def __init__(self, module, parent_path=None):
        types.ModuleType.__init__(self, module.__name__)

        self.module = module
        # Importing all will be customised later
        self.module.__all__ = []

        self.__doc__  = module.__doc__
        self.__name__ = module.__name__
        self.__file__ = module.__file__

        self.__class__.__getattr__ = self._fallback_getattr

        if parent_path is None:
            self._path=module.__name__
        else:
            self._path=f"{parent_path}.{module.__name__}"

    def _guess_filename(self, classname):
        parts=classname.split(".")
        # Strip off dunedaq from the front
        if parts[0]=="dunedaq":
            del parts[0]
        if len(parts)<2:
            return None
        else:
            return f"{parts[0]}/{parts[1]}.jsonnet"

    def _do_import(self, classname):
        parts=classname.split(".")
        pprint(moo.otypes.load_types(self._guess_filename(classname)))
        mod=import_module(".".join(parts[0:3]))
        # c=do_import("dunedaq.trigger.triggerprimitivemaker.ConfParams")
        return getattr(mod, parts[3])()
    
    def _fallback_getattr(self, name):
        new_path=f"{self._path}.{name}"
        trial_path=new_path.replace("trigger.schemaload.", "")
        trial_path_parts=trial_path.split(".")
        print(f"Trying to guess filename for {trial_path}")
        filename=self._guess_filename(trial_path)

        if filename is not None:
            try:
                moo.otypes.load_types(filename)
                print(trial_path_parts)
                mod=import_module(".".join(trial_path_parts[0:3]))
                return getattr(mod, trial_path_parts[3])()
            except Exception as e:
                print(e)

        mod = types.ModuleType(name)
        mod.__doc__ = name
        mod.__file__ = self.__file__
        return Facade(mod, self._path)
