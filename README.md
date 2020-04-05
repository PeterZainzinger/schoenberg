# schoenberg

A is a low level input_event mapper for virtual layers 
inspired by ergodox. It's ought  to be used in 
combination with 
[interception tools](https://gitlab.com/interception/linux/tools/tree/master).

## Features

* map an arbitrary key to another key
* switch to other layer with different key mapping


Example configuration:

```
mapping:
  ESC: CAPSLOCK
  CAPSLOCK: ESC

layers:
  - name: right hand
    prefix: F
    keys:
      J: DOWN
      K: UP
      H: LEFT
      L: RIGHT
```

## Getting Started 

* install [interception tools] and its dependencies and add the 
binaries to the path, for example `udevmon`.
* build an `schoenberg_run` executable with `cmake . && make`.
In the nix shell started with `nix-shell --pure` this should work out of the box.
* create a `config.yaml` file, see `tst/test.yaml` for an example. The name of the keys 
correspond with the event names in [include/uapi/linux/input-event-codes.h](https://github.com/torvalds/linux/blob/master/include/uapi/linux/input-event-codes.h).
* create a `udevmon.yaml`. Here `schoenberg_run` and `config.yaml` have to adapted.

```
- JOB: "intercept -g $DEVNODE | ./src/schoenberg_run ./tst/test.yaml | uinput -d $DEVNODE"
  DEVICE:
    EVENTS:
      EV_KEY: [KEY_S]
```
**NOTE** the value of `EV_KEY` can be anything as long as its not empty.
 
* start the udevmon daemon.

```
sudo udevmon -c udevmon.yaml 
```




