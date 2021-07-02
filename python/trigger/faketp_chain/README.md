# faketp_chain

This is an example application that runs a full dataselection chain using 
`TriggerPrimitiveMaker` to read TPs from a file, and `FakeDataFlow` to emulate
the response of dataflow. In the middle, the following trigger modules are used:

* `TriggerActivityMaker` which loads a `triggeralgs` plugin to convert `TPSet` 
  from `TriggerPrimativeMaker` into `TASet` consisting of the `TriggerActivity`
  produced by the algorithm being called on `TriggerPrimitive` in the `TPSet`.
  
* `TriggerCandidateMaker` which loads a `triggeralgs` plugin to convert `TASet` 
  from `TriggerActivityMaker` into `TriggerCandidates` produced by the algorithm
  being called on `TriggerActivity` in the `TASet`.
  
* `ModuleLevelTrigger` which produces `TriggerDecision` from the input
  `TriggerCandidate` from `TriggerCandidateMaker`, and exchanges these with 
  tokens with `FakeDataFlow`

A plugin is a `trigger` DAQ module with the sole purpose of loading a 
`triggeralgs` algorithm. The algorithms are thus in the `triggeralgs` package
and are subclasses of `triggeralgs::Trigger*Maker`. The plugin is loaded by a
`trigger::Trigger*Maker`, which handles the input and output queues, and calling
the `operator()` of the algorithm.

## Generating the app

The `trigger.faketp_chain` app generator will create the metadata for `nanorc` 
to execute.

```
Usage: python -m trigger.faketp_chain [OPTIONS] JSON_DIR

  JSON_DIR: Json file output folder

Options:
  -s, --slowdown-factor FLOAT  [default: 1.0]
  -f, --input-file PATH
  -c, --candidate-plugin TEXT  [default: TriggerCandidateMakerPrescalePlugin]
  -C, --candidate-config TEXT  [default: dict(prescale=1000)]
  -a, --activity-plugin TEXT   [default: TriggerActivityMakerPrescalePlugin]
  -A, --activity-config TEXT   [default: dict(prescale=1000)]
  -h, --help                   Show this message and exit.  [default: False]
```

By default, the prescale algorithms are used, managed by the prescale plugins. 
These can be changed with the `-a` and `-c` options. The algorithms have a 
`configure` method which currently accepts a JSON object. These objects may be
given as a string containing a python dictionary with the `-A` and `-C` options. 

As a concrete example, to change the prescale of the activity, and read TPs from
a file called `tps_link_11.txt`:
```
python -m trigger.faketp_chain -A "dict(prescale=100)" -f tps_link_11.txt config_dir
```

## Running the app

The generated JSON_DIR (`config_dir` in the above example) can be run with the
`nanorc` run control. Using a recent version of `nanorc` this looks something
like:

```
python -m nanorc config_dir boot init conf start 1 resume wait 10 stop
```
which runs though the `boot`, `init`, `conf` commands, starts run 1, resumes
the `ModuleLevelTrigger`, waits 10 seconds, and stops.

This will produce a log file `log_faketp_chain_3333.txt` with output from the 
run, and a similarly named info file.

## Data files

As of 23-Jun, there are a couple of TP files available from [cernbox](https://cernbox.cern.ch/index.php/s/75PDf54a9DWXWOJ).

To download them, you can use the following commands:

* `curl -o tps_link_05.txt -O https://cernbox.cern.ch/index.php/s/75PDf54a9DWXWOJ/download?files=tps_link_05.txt`
* `curl -o tps_link_11.txt -O https://cernbox.cern.ch/index.php/s/75PDf54a9DWXWOJ/download?files=tps_link_11.txt`
