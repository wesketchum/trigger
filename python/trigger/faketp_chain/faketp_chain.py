# Set moo schema search path
from dunedaq.env import get_moo_model_path
import moo.io
moo.io.default_load_path = get_moo_model_path()

from pprint import pprint
pprint(moo.io.default_load_path)
# Load configuration types
import moo.otypes
moo.otypes.load_types('rcif/cmd.jsonnet')
moo.otypes.load_types('appfwk/cmd.jsonnet')
moo.otypes.load_types('appfwk/app.jsonnet')

moo.otypes.load_types('trigger/triggerprimitivemaker.jsonnet')
moo.otypes.load_types('trigger/triggeractivitymaker.jsonnet')
moo.otypes.load_types('trigger/triggercandidatemaker.jsonnet')
moo.otypes.load_types('trigger/moduleleveltrigger.jsonnet')
moo.otypes.load_types('trigger/fakedataflow.jsonnet')
moo.otypes.load_types('trigger/triggerzipper.jsonnet')
moo.otypes.load_types('trigger/faketpcreatorheartbeatmaker.jsonnet')

# Import new types
import dunedaq.cmdlib.cmd as basecmd # AddressedCmd, 
import dunedaq.rcif.cmd as rccmd # AddressedCmd, 
import dunedaq.appfwk.cmd as cmd # AddressedCmd, 
import dunedaq.appfwk.app as app # AddressedCmd,
import dunedaq.trigger.triggerprimitivemaker as tpm
import dunedaq.trigger.triggeractivitymaker as tam
import dunedaq.trigger.triggercandidatemaker as tcm
import dunedaq.trigger.moduleleveltrigger as mlt
import dunedaq.trigger.fakedataflow as fdf
import dunedaq.trigger.triggerzipper as tzip
import dunedaq.trigger.faketpcreatorheartbeatmaker as ftpchm

from appfwk.utils import mcmd, mrccmd, mspec

import json
import math
from pprint import pprint


def acmd(mods: list) -> cmd.CmdObj:
    ''' 
    Helper function to create appfwk's Commands addressed to modules.
        
    :param      cmdid:  The coommand id
    :type       cmdid:  str
    :param      mods:   List of module name/data structures 
    :type       mods:   list
    
    :returns:   A constructed Command object
    :rtype:     dunedaq.appfwk.cmd.Command
    '''
    return cmd.CmdObj(
        modules=cmd.AddressedCmds(
            cmd.AddressedCmd(match=m, data=o)
            for m,o in mods
        )
    )

#FIXME maybe one day, triggeralgs will define schemas... for now allow a dictionary of 4byte int, 4byte floats, and strings
moo.otypes.make_type(schema='number', dtype='i4', name='temp_integer', path='temptypes')
moo.otypes.make_type(schema='number', dtype='f4', name='temp_float', path='temptypes')
moo.otypes.make_type(schema='string', name='temp_string', path='temptypes')
def make_moo_record(conf_dict,name,path='temptypes'):
    fields = []
    for pname,pvalue in conf_dict.items():
        typename = None
        if type(pvalue) == int:
            typename = 'temptypes.temp_integer'
        elif type(pvalue) == float:
            typename = 'temptypes.temp_float'
        elif type(pvalue) == str:
            typename = 'temptypes.temp_string'
        else:
            raise Exception(f'Invalid config argument type: {type(value)}')
        fields.append(dict(name=pname,item=typename))
    moo.otypes.make_type(schema='record', fields=fields, name=name, path=path)

def generate(
        INPUT_FILES: str,
        SLOWDOWN_FACTOR: float,
        
        ACTIVITY_PLUGIN: str = 'TriggerActivityMakerPrescalePlugin',
        ACTIVITY_CONFIG: dict = dict(prescale=1000),
        
        CANDIDATE_PLUGIN: str = 'TriggerCandidateMakerPrescalePlugin',
        CANDIDATE_CONFIG: int = dict(prescale=1000),
        
        TOKEN_COUNT: int = 10,
        
        FORGET_DECISION_PROB: float = 0.0,
        HOLD_DECISION_PROB: float = 0.0,
        HOLD_MAX_SIZE: int = 0,
        HOLD_MIN_SIZE: int = 0,
        HOLD_MIN_MS: int = 0,
        RELEASE_RANDOMLY_PROB: float = 0.0,
        
        CLOCK_SPEED_HZ: int = 50000000
):
    cmd_data = {}

    # Derived parameters
    CLOCK_FREQUENCY_HZ = CLOCK_SPEED_HZ / SLOWDOWN_FACTOR

    # Define modules and queues
    queue_specs = [
        app.QueueSpec(inst=f"tpset_q{i}", kind='FollySPSCQueue', capacity=1000)
        for i in range(len(INPUT_FILES))
    ] +  [
        app.QueueSpec(inst="tpset_plus_hb_q", kind='FollyMPMCQueue', capacity=1000),
        app.QueueSpec(inst='zipped_tpset_q', kind='FollyMPMCQueue', capacity=1000),
        app.QueueSpec(inst='taset_q', kind='FollySPSCQueue', capacity=1000),
        app.QueueSpec(inst='trigger_candidate_q', kind='FollyMPMCQueue', capacity=1000),
        app.QueueSpec(inst='trigger_decision_q', kind='FollySPSCQueue', capacity=1000),
        app.QueueSpec(inst='token_q', kind='FollySPSCQueue', capacity=1000),
    ]

    mod_specs = [
        mspec(f'tpm{i}', 'TriggerPrimitiveMaker', [ # File -> TPSet
            app.QueueInfo(name='tpset_sink', inst=f'tpset_q{i}', dir='output'),
        ])
        for i in range(len(INPUT_FILES))
    ] + [

        mspec(f"ftpchm{i}", "FakeTPCreatorHeartbeatMaker", [
            app.QueueInfo(name="tpset_source", inst=f"tpset_q{i}", dir="input"),
            app.QueueInfo(name="tpset_sink", inst="tpset_plus_hb_q", dir="output"),
        ]) for i in range(len(INPUT_FILES))

    ] +  [

        mspec("zip", "TPZipper", [
            app.QueueInfo(name="input", inst="tpset_plus_hb_q", dir="input"),
            app.QueueInfo(name="output", inst="zipped_tpset_q", dir="output"),
        ]),

        mspec('tam', 'TriggerActivityMaker', [ # TPSet -> TASet
            app.QueueInfo(name='input', inst='zipped_tpset_q', dir='input'),
            app.QueueInfo(name='output', inst='taset_q', dir='output'),
        ]),
        
        mspec('tcm', 'TriggerCandidateMaker', [ # TASet -> TC
            app.QueueInfo(name='input', inst='taset_q', dir='input'),
            app.QueueInfo(name='output', inst='trigger_candidate_q', dir='output'),
        ]),
        
        mspec('mlt', 'ModuleLevelTrigger', [ # TC -> TD (with sufficient tokens)
            app.QueueInfo(name='trigger_candidate_source', inst='trigger_candidate_q', dir='input'),
            app.QueueInfo(name='trigger_decision_sink', inst='trigger_decision_q', dir='output'),
            app.QueueInfo(name='token_source', inst='token_q', dir='input'),
        ]),
        
        mspec('fdf', 'FakeDataFlow', [ # TD -> Token
            app.QueueInfo(name='trigger_decision_source', inst='trigger_decision_q', dir='input'),
            app.QueueInfo(name='trigger_complete_sink', inst='token_q', dir='output'),
        ]),
    ]

    cmd_data['init'] = app.Init(queues=queue_specs, modules=mod_specs)

    make_moo_record(ACTIVITY_CONFIG,'ActivityConf','temptypes')
    make_moo_record(CANDIDATE_CONFIG,'CandidateConf','temptypes')
    import temptypes

    cmd_data['conf'] = acmd([
        (f'tpm{i}', tpm.ConfParams(
            filename=input_file,
            number_of_loops=-1, # Infinite
            tpset_time_offset=0,
            tpset_time_width=10000,
            clock_frequency_hz=CLOCK_FREQUENCY_HZ,
            maximum_wait_time_us=1000
        )) for i,input_file in enumerate(INPUT_FILES)
    ] + [
        (f"ftpchm{i}", ftpchm.Conf(
          heartbeat_interval = 100000
        )) for i in range(len(INPUT_FILES))
    ] + [
        ("zip", tzip.ConfParams(
            cardinality=len(INPUT_FILES),
            max_latency_ms=1000,
            region_id=0, # Fake placeholder
            element_id=0 # Fake placeholder
        )),
        ('tam', tam.Conf(
            activity_maker=ACTIVITY_PLUGIN,
            geoid_region=0, # Fake placeholder
            geoid_element=0, # Fake placeholder
            buffer_time=625000, # 10ms in 62.5 MHz ticks
            activity_maker_config=temptypes.ActivityConf(**ACTIVITY_CONFIG)
        )),
        ('tcm', tcm.Conf(
            candidate_maker=CANDIDATE_PLUGIN,
            candidate_maker_config=temptypes.CandidateConf(**CANDIDATE_CONFIG)
        )),
        ('mlt', mlt.ConfParams(
            links=[],
            initial_token_count=TOKEN_COUNT                    
        )),
        ('fdf', fdf.ConfParams(
          hold_max_size = HOLD_MAX_SIZE,
          hold_min_size = HOLD_MIN_SIZE,
          hold_min_ms = HOLD_MIN_MS,
          release_randomly_prob = RELEASE_RANDOMLY_PROB,
          forget_decision_prob = FORGET_DECISION_PROB,
          hold_decision_prob = HOLD_DECISION_PROB
        )),
    ])

    startpars = rccmd.StartParams(run=1)
    cmd_data['start'] = acmd(
        [
            ('fdf', startpars),
            ('mlt', startpars),
            ('tcm', startpars),
            ('tam', startpars),
            ('zip', startpars),
        ] +
        [ (f'ftpchm{i}', startpars) for i in range(len(INPUT_FILES)) ] +
        [ (f'tpm{i}', startpars) for i in range(len(INPUT_FILES)) ]
    )

    cmd_data['pause'] = acmd([
        ('mlt', None)
    ])
    
    resumepars = rccmd.ResumeParams(trigger_interval_ticks=50000000)
    cmd_data['resume'] = acmd([
        ('mlt', resumepars)
    ])
    
    cmd_data['stop'] = acmd(
        [ (f'tpm{i}', None) for i in range(len(INPUT_FILES)) ] +
        [ (f'ftpchm{i}', None) for i in range(len(INPUT_FILES)) ] +
        [
            ('zip', None),
            ('tam', None),
            ('tcm', None),
            ('mlt', None),
            ('fdf', None)
        ]
    )

    cmd_data['scrap'] = acmd(
        [ (f'tpm{i}', None) for i in range(len(INPUT_FILES)) ] +
        [ (f'ftpchm{i}', None) for i in range(len(INPUT_FILES)) ] +
        [
            ('zip', None),
            ('tam', None),
            ('tcm', None),
            ('mlt', None),
            ('fdf', None)
        ]
    )

    return cmd_data
