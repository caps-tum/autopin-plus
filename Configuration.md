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

## clustsafe

The ```clustsafe``` performance monitor can read the current energy levels from the ClustSafe devices made by MEGWARE.

See also: http://www.megware.com/en/produkte_leistungen/eigenentwicklungen/clustsafe-71-8.aspx

The following options are available:

  - ```<name>.host = <string>``` (no default)

    The host name or IP address of the ClustSafe device.

  - ```<name>.port = <integer>``` (defaults to ```2010```)

    The port on which the ClustSafe device listens.

  - ```<name>.signature = <string>``` (defaults to ```MEGware```)

    The signature string used to address a specific kind of ClustSafe device.

  - ```<name>.password = <string>``` (defaults to ```""```)

    The password used when accessing the ClustSafe device.

  - ```<name>.outlets = <integer> [<integer>] [...]``` (no default)

    The list of outlets whose values will be added to form the resulting value.

  - ```<name>.timeout = <integer>``` (defaults to ```1000```)

    The amount of milliseconds before a connection attempt or data read will time out.

  - ```<name>.ttl = <integer>``` (defaults to ```10```)

    To avoid unnecessary queries, the ClustSafe device will only be queried if the cached value is too old. This is the amount of milliseconds after which the cached value will be refreshed.

## gperf

The ```gperf``` monitor is also based on Linux' perf subsystem. Compared to the ```perf``` monitor it is more generic and allows to monitor (almost) all events available on the system.

The following options are available:

  - ```<name>.sensor = <string>``` (no default)

    This controls which sensor you want to use. For a list of sensors available on your system run

    ```
    ls /sys/bus/event_source/devices/*/events/* | grep -E -v '\.(scale|unit)$'
    ```

    Depending on your hardware and kernel configuration, the output will look **roughly** like this:

      - ```/sys/bus/event_source/devices/cpu/events/branch-instructions```
      - ```/sys/bus/event_source/devices/cpu/events/branch-misses```
      - ```/sys/bus/event_source/devices/cpu/events/bus-cycles```
      - ```/sys/bus/event_source/devices/cpu/events/cache-misses```
      - ```/sys/bus/event_source/devices/cpu/events/cache-references```
      - ```/sys/bus/event_source/devices/cpu/events/cpu-cycles```
      - ```/sys/bus/event_source/devices/cpu/events/instructions```
      - ```/sys/bus/event_source/devices/cpu/events/mem-loads```
      - ```/sys/bus/event_source/devices/cpu/events/mem-stores```
      - ```/sys/bus/event_source/devices/cpu/events/stalled-cycles-frontend```
      - ```/sys/bus/event_source/devices/power/events/energy-cores```
      - ```/sys/bus/event_source/devices/power/events/energy-gpu```
      - ```/sys/bus/event_source/devices/power/events/energy-pkg```
      - ```/sys/bus/event_source/devices/uncore_cbox_0/events/clockticks```
      - ```/sys/bus/event_source/devices/uncore_cbox_1/events/clockticks```

    Any of these files can be used as a sensor. Be aware, that you need to pass the full, absolute path to the file. Relative paths and symlinks **will not work**, as the program will try to read additional information from files located in the vicinity.

    Additionally, the kernel provides abstraced names for some hardware-based sensors. These names are guaranteed to be the identical on all systems. However, there is **no guarantee** that a suiting sensor actually exists on your system! The following hardware sensors are available:

      - ```hardware/cpu-cycles```
      - ```hardware/instructions```
      - ```hardware/cache-references```
      - ```hardware/cache-misses```
      - ```hardware/branch-instructions```
      - ```hardware/branch-misses```
      - ```hardware/bus-cycles```
      - ```hardware/ref-cpu-cycles```
      - ```hardware/stalled-cycles-frontend```
      - ```hardware/stalled-cycles-backend```
      - ```cache/l1d-read-access```
      - ```cache/l1d-read-miss```
      - ```cache/l1d-write-access```
      - ```cache/l1d-write-miss```
      - ```cache/l1d-prefetch-access```
      - ```cache/l1d-prefetch-miss```
      - ```cache/l1i-read-access```
      - ```cache/l1i-read-miss```
      - ```cache/l1i-write-access```
      - ```cache/l1i-write-miss```
      - ```cache/l1i-prefetch-access```
      - ```cache/l1i-prefetch-miss```
      - ```cache/ll-read-access```
      - ```cache/ll-read-miss```
      - ```cache/ll-write-access```
      - ```cache/ll-write-miss```
      - ```cache/ll-prefetch-access```
      - ```cache/ll-prefetch-miss```
      - ```cache/dtlb-read-access```
      - ```cache/dtlb-read-miss```
      - ```cache/dtlb-write-access```
      - ```cache/dtlb-write-miss```
      - ```cache/dtlb-prefetch-access```
      - ```cache/dtlb-prefetch-miss```
      - ```cache/itlb-read-access```
      - ```cache/itlb-read-miss```
      - ```cache/itlb-write-access```
      - ```cache/itlb-write-miss```
      - ```cache/itlb-prefetch-access```
      - ```cache/itlb-prefetch-miss```
      - ```cache/bpu-read-access```
      - ```cache/bpu-read-miss```
      - ```cache/bpu-write-access```
      - ```cache/bpu-write-miss```
      - ```cache/bpu-prefetch-access```
      - ```cache/bpu-prefetch-miss```
      - ```cache/node-read-access```
      - ```cache/node-read-miss```
      - ```cache/node-write-access```
      - ```cache/node-write-miss```
      - ```cache/node-prefetch-access```
      - ```cache/node-prefetch-miss```

    The kernel also provides some sensors which are done entirely in software and should therefore be available on all machines. The following software sensors are available:

      - ```software/cpu-clock```
      - ```software/task-clock```
      - ```software/page-faults```
      - ```software/context-switches```
      - ```software/cpu-migrations```
      - ```software/page-faults-min```
      - ```software/page-faults-maj```
      - ```software/alignment-faults```
      - ```software/emulation-faults```
      - ```software/dummy```

    Finally, we also allow configuring the value used in the ```attr``` parameter of the ```perf_event_open(2)``` syscall manually. The format for this is:

      - ```perf_event_attr/key1=value1,key2=value2,...```

    The available keys are the fields in the ```perf_event_attr struct``` defined in the ```/usr/include/linux/perf_event.h``` file. The valid values are numeric values in the C language convention (i.e. with either no prefix or ```0``` or ```0x``` as a prefix).
    Be aware that the values **will not be checked** in any way, not even for an overflow. Omitted values default to ```0```, except for the ```size``` field (which will be set to the correct value) and the ```disabled``` field (which will be set to ```1```).

  - ```<name>.processors = <integer> [<integer>] [...]``` (no default)

    This list controls which processors to monitor. If omitted, the program will try to automatically determine a sensible list from the value of the ```<name>.sensor``` option. If that fails, the program will try to monitor all available processors. Not all sensors support that!

  - ```<name>.valtype = <string>``` (defaults to ```MIN```)

    This can be one of ```MIN```, ```MAX``` or ```UNKNOWN```. If set to ```MIN```, smaller values will be considered ```better```. If set to ```MAX```, bigger values will be be preferred. If set to ```UNKNOWN``` no preference is selected.

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

## noop

The ```noop``` control strategy does nothing besides starting the configured performance monitors. It's useful if you want to measure the performance of an application without doing any kind of thread pinning.

The following options are available:

  - ```interval = <integer>``` (defaults to ```100```)

    If process tracing is disabled (see the ```Trace``` option) we have to periodically query the OS for the current list of threads. This option configures the amount of milliseconds between two such queries.

# Data Loggers

You can specify one or more data loggers via the ```DataLoggers``` option:

```DataLoggers = <foo> [<bar>] [<baz>]```

 Data loggers have access to the configured performance monitors and can show or save their values at a fixed time or periodically. Please note that data loggers will never create processes or start/stop performance monitors themselves, so they can only be used in combination with a suitable control strategy. Additionally every data logger can only be specified once.

The following data loggers are available:

## external

The ```external``` data logger spawns a configurable program and periodically sends it the current performance data for further processing.

The command will be spawned with ```stdin```, ```stdout``` and ```stderr``` connected as follows:

  - stdin

    This channel will be connected to the logger, which will periodically write data points containg performance data to it.

    The performance data is formatted as plain text, with each data point on a separate line. A data point contains the following data (in this order) separated by tab stops:

      - monitor (```string```)

        Name of the performance monitor as configured in the configuration file.

      - tid (```integer```)

        Thread id (TID) of the thread being monitored.

      - time (```float```)

        The time between the start of the logger and the creation of the data point, in seconds.

      - value (```float```)

        The raw value of the specified performance monitor for the specified thread at the specified time.

      - unit (```string```)

        The unit of the value, or ```none``` if the performance monitor doesn't have a unit.

  - stdout, stderr

    These channel will be connected to the logger which will read the data sent there and print it (prefixed by ```[stdout]``` or ```[stderr]```) on screen using ```autopin+```'s logging system.

When the data logger terminates, it will close all those communication channels. However, the program itself will not be terminated. Your program should detect the ```EOF``` and react appropriately.

The following options are available:

  - ```external.command = <command> [<argument>] [<argument>] [...]``` (defaults to ```cat```)

    The external command as described above. The command will be invoked directly (without using the system shell), but ```$PATH``` lookup is supported.

  - ```external.interval = <integer>``` (defaults to ```100```)

    The time in milliseconds between two data points. Please note that the actual data points may be further apart as gathering the necessary data also takes time.

  - ```external.systemwide = <boolean>``` (defaults to ```false```)

    If set to true, the logger will only emit data points for the first of the monitored threads instead of for all of them (it will still emit data points for all performance monitors though). This is useful if your performance monitors are measuring system-wide values which are identical for all threads.
