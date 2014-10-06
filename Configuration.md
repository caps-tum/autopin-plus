# Configuring autopin+

This documents describes how to configure ```autopin+```. The first two sections are specific to the configuration format implemented by the class ```StandardConfiguration```, which is used by default.

# Configuration format
Configuration options for autopin+ are pairs of option names and value lists:

```opt = val1 val2 val3```

The above statement assigns the values ```val1```, ```val2``` and ```val3``` to the configuration option opt. Additional values can be added using the operator ```+=```. The ```-=``` operator can be used to remove values from the list:

```
opt = val1 val2 val3
opt += val4 --> opt contains val1, val2, val3 and val4
opt -= val1 --> opt contains val2, val3 and val4
```

When using a configuration file, single lines can be disabled by adding ```#```:

```
#opt = val1 val2 val3
```

Boolean values can be expressed using the keywords ```true```, ```True```, ```TRUE``` and ```false```, ```False```, ```FALSE```.

# Configuration sources

On startup, ```autopin+``` accepts configuration options from three different sources:

  - Default configuration

    A default configuration file with standard options can be loaded by setting the environment variable ```AUTOPIN_DEFAULT_CONFIG``` to the path of the desired configuration file.

  - User configuration

    By starting ```autopin+``` with the argument ```-c <user_config>``` where ```<user_config>``` is the path to a configuration file, one can provide more specific configuration options.

  - Command line

    Configuration options can also be specified directly on the command line. In this case the prefix ```--``` must be added to the corresponding option:

    ```
    autopin+ -c user_config --opta+=val4 --optb=val8
    ````

The three configuration sources are loaded in the order shown above. This means that the user configuration can overwrite settings specified in the default configuration. Command line options can overwrite both the user configuration and the default configuration.

# Basic configuration options

The following options are available:

  - ```Exec = <string>``` (no default)

    ```autopin+``` will start the observed process with the command specified in the argument.

  - ```Attach = <string>|<int>``` (no default)

    ```autopin+``` will attach to the process with the given name or the given pid.

  - ```Trace = <boolean>``` (defaults to ```false```)

    Enable or disable process tracing.

  - ```CommChan = <string>|<boolean>``` (no default).

    Enable the communication channel for notifications from the observed process. If the argument is a string it is interpreted as the address for the communication channel. If no address is specified but the argument is set to true ```autopin+``` will use a default address.

  - ```Logfile = <string>``` (no default)

    If this option is set, the output of ```autopin+``` will be redirected to the file specified in the argument.

  - ```PinningHistory.load = <string>``` (no default)

    Load a pinning history file from the specified path. The type of the history is determined by the suffix of the file (e. g. .xml).

  - ```PinningHistory.save = <string>``` (no default)

    Save a pinning history file to the specified path. The type of the history is determined by the suffix of the file. (e. g. .xml).

# Performance monitors

As ```autopin+``` supports the parallel usage of different performance monitors, every monitor must be assigned a unique name. This name has to be added to the configuration option ```PerformanceMonitors```:

```
PerformanceMonitors = <foo> [<bar>] [<baz>] [...]
```

After defining the monitor the type can be selected:

```
<foo>.type = <type>
<bar>.type = <type>
<baz>.type = <type>
```

The following performance monitors are the available:

## random

The ```random``` performance monitor is intended for testing. It does not perform any measurements but just returns random values. The monitor provides the following configuration options (<name> has to be replaced with the name of the monitor):


  - ```<name>.rand_min = <integer>``` (defaults to ```0```)

    Lower bound for the generated random numbers.

  - ```<name>.rand_max = <integer>``` (defaults to ```1```)

    Upper bound for the generated random numbers.

  - ```<name>.valtype = <string>``` (defaults to ```MAX```)

    Type of the reported values (```MAX```, ```MIN``` or ```UNKNOWN```). This information can be used by control strategies to find out if bigger or smaller results are "better".

## perfmon

This monitor is based on the perfmon monitoring interface and thus requires the perfmon kernel patch. As perfmon uses ptrace, the process tracing feature of ```autopin+``` is not available when this monitor is used.

The following options are available:

  - ```<name>.event_type = <string>``` (no default)

    A valid perfmon event type.

The supported events depend on the hardware platform and can be determined with the perfmon utility.

## perf

The ```perf``` monitor is based on the Linux Performance Counter subsystem which is part of newer kernel versions and does not require any kernel patches.

The following options are available:

  - ```<name>.event_type = <string>``` (no default)

    A valid perf event type.

    The following event types are available:

      - ```PERF_COUNT_HW_CPU_CYCLES```
      - ```PERF_COUNT_HW_INSTRUCTIONS```
      - ```PERF_COUNT_HW_CACHE_REFERENCES```
      - ```PERF_COUNT_HW_CACHE_MISSES```
      - ```PERF_COUNT_HW_BRANCH_INSTRUCTIONS```
      - ```PERF_COUNT_HW_BRANCH_MISSES```
      - ```PERF_COUNT_HW_BUS_CYCLES```
      - ```PERF_COUNT_HW_STALLED_CYCLES_FRONTEND```
      - ```PERF_COUNT_HW_STALLED_CYCLES_BACKEND```

# Control strategies

The control strategy which will be used by ```autopin+``` must be specified with the option ```ControlStrategy```, for example:

```
ControlStrategy = autopin1
```

The following control strategies are the available:

## autopin1

The ```autopin1``` control strategy is an extended version of the control strategy of the original ```autopin```. It supports the ```autopin+``` communication channel and is able to store results in the pinning history.

```autopin+``` introduces a new format for pinnings:

```
autopin1.schedule = <cpu_a>:<cpu_b><cpu_c>:<cpu_d>
```

The cores are separated by a ```:```. This makes it possible to specify pinnings on systems with more than 10 cores.

The following options are available:

  - ```autopin1.schedule = <pinning> [<pinning>] [...]``` (no default)

    The pinnings which will be tested by ```autopin+```.

  - ```autopin1.init_time = <integer>``` (defaults to ```15```)

    The init time (like in the original autopin).

  - ```autopin1.warmup_time = <integer>``` (defaults to ```15```)

    The warmup time (like in the original autopin).

  - ```autopin1.measure_time = <integer>``` (defaults to ```30```)

    The measure time (like in the original autopin).

  - ```autopin.openmp_icc = <boolean>``` (defaults to ```false```)

    If this option is true the second thread will not be pinned. This is useful if the application uses OpenMP and has been compiled with the Intel compiler.

  - ```autopin1.skip = <integer> [<integer>] [...]``` (no default)

    This option can be used to specifiy threads which will not be pinned.

  - ```autopin1.notification_interval = <integer>``` (defaults to ```0```)

    If the communication channel is used the value of this option specifies the minimum interval between two phase change notifications.
