# Keysight 34465A IOC

EPICS IOC for the Keysight 34465A Truevolt 6½-digit digital multimeter,
using StreamDevice over the instrument's raw SCPI socket (TCP 5025).

## Features

- All 12 measurement functions: DCV, ACV, DCI, ACI, 2W/4W Ohm, frequency,
  period, continuity, diode, temperature, capacitance
- Range / autorange, NPLC, autozero, null (relative) and null value
- DCV input impedance, current input jack (3 A / 10 A), temperature transducer
- Trigger source / slope / delay, trigger count and samples per trigger
- `READ?` acquisition into a waveform, with last value, mean, std dev,
  min, max, valid count and an overload flag (`±9.9E+37`)
- Engineering units follow the active function
- User conversion to a derived quantity (e.g. pC): `CONV_*` outputs =
  reading × `CONV_FACTOR` + `CONV_OFFSET`, in unit `CONV_EGU`
- Settings that the active function does not support are disabled
  (no invalid SCPI is sent), and the OPI greys them out
- SCPI error queue: live poll plus a latched last error cleared by `*CLS`
- Front/rear terminals switch, internal temperature, line frequency,
  display on/off/text, beep, `*RST`, abort
- Phoebus OPI: `opi/keysight34465A.bob`

## Layout

- `keysight34465AApp/Db/` — `keysight34465A.db`, `keysight34465A.proto`
- `keysight34465AApp/src/` — IOC main and the `keysightStats` aSub routine
- `iocBoot/ioc34465A/st.cmd` — startup script
- `opi/` — Phoebus display
- `testScript/scpi_probe.py` — talk SCPI directly to the instrument

## Build and run

Build inside the `ghcr.io/infn-epics/infn-epics-ioc-base` dev container,
which provides EPICS base, asyn and StreamDevice under `/epics`
(see `configure/RELEASE`; override in `configure/RELEASE.local`).

```bash
cd /app/IOC-DEVEL/multimeter-keysight
make -j4
cd iocBoot/ioc34465A
./st.cmd
```

The instrument address and PV prefix are set at the top of `st.cmd`:

| Variable   | Default          | Notes                                  |
|------------|------------------|----------------------------------------|
| `DMM_IP`   | `192.168.190.18` | Instrument web UI: `http://<ip>/Index.asp` |
| `DMM_PORT` | `5025`           | SCPI raw socket                        |
| `P`, `R`   | `KEYSIGHT`, `DMM1` | Records are `$(P):$(R):<NAME>`       |
| `NELM`     | `1000`           | Max readings per `READ?`               |

`dbLoadRecords` also takes `SCAN` (acquisition rate, `1 second`),
`RB_SCAN` (settings readback, `5 second`), `ERR_SCAN` (`1 second`) and
`ACQ_EN` (`1`), and the conversion defaults `CONV_FACTOR` (`1`),
`CONV_OFFSET` (`0`), `CONV_EGU` (empty) and `CONV_PREC` (`4`).

### Conversion

`CONV_VAL/MEAN/MIN/MAX = x × CONV_FACTOR + CONV_OFFSET` and
`CONV_STD = STD × |CONV_FACTOR|`; a negative factor swaps min and max.
They are recomputed after every acquisition and whenever the factor or
offset changes. Example: DCV across a 1 nF capacitor read as charge
(Q = C·V) → `CONV_FACTOR=1000` (1e-9 F × 1e12 pC/C), `CONV_EGU=pC`.

The parameters are not saved across IOC restarts (no autosave); put the
values you need in the `dbLoadRecords` macros in `st.cmd`.

## EPIK8S / generic IOC deployment

- ibek support: `ibek-support-infn/keysight34465A` (entity `keysight34465A.dmm`);
  the generic IOC links `libkeysight34465ASupport` and `keysightStats.dbd`.
- ibek template: `ibek-templates/templates/daq/keysight34465A`
  (test config `ibek-templates/tests/daq-keysight34465A-test.yaml`).
- OPI: `epik8s-opi/keysight/keysight34465A.bob` (copy of `opi/keysight34465A.bob`).

## Main PVs

| PV | Type | Description |
|----|------|-------------|
| `VAL` | ai | Last reading (EGU follows the function) |
| `MEAN`, `STD`, `MIN`, `MAX`, `NVALID` | ai/longin | Statistics over the last `READ?` |
| `OVLD` | bi | Overload in the last `READ?` (MAJOR alarm) |
| `READ_ARR` | waveform | All readings of the last `READ?` |
| `CONV_FACTOR`, `CONV_OFFSET`, `CONV_EGU` | ao/ao/stringout | Conversion parameters |
| `CONV_VAL`, `CONV_MEAN`, `CONV_STD`, `CONV_MIN`, `CONV_MAX` | calc | Converted value and statistics |
| `ACQ_EN` | bo | Enable/disable periodic acquisition |
| `TRIGGER` | bo | Single acquisition (use with `READ_ARR.SCAN = Passive`) |
| `FUNC` / `FUNC_RB` | mbbo/mbbi | Measurement function |
| `RANGE`, `AUTO`, `NPLC`, `AZ`, `NULL`, `NULL_VAL` | — | Function settings (`*_RB` readbacks) |
| `IMP_AUTO`, `TERM`, `TEMP_TYPE` | — | DCV input Z, current jack, temp sensor |
| `TRIG_SRC`, `TRIG_SLOPE`, `TRIG_DEL`, `TRIG_DEL_AUTO`, `TRIG_COUNT`, `SAMP_COUNT` | — | Trigger / sampling |
| `HAS_RANGE`, `HAS_NPLC`, `HAS_AZ`, `HAS_NULL`, `HAS_IMP`, `HAS_TERM`, `HAS_TEMP` | calc | Capabilities of the active function |
| `LAST_ERR_CODE`, `LAST_ERR_MSG` | longin/lsi | Last non-zero SCPI error, latched |
| `ERR_CODE`, `ERR_MSG` | longin/lsi | Live `SYST:ERR?` poll |
| `CLEAR`, `RESET`, `ABORT`, `BEEP` | bo | `*CLS`, `*RST;*CLS`, `ABOR`, beep |
| `STATE_FILE`, `STATE_RESTORE`, `STATE_CAT` | stringout/bo/lsi | Restore a saved `.sta` state; list files on the DMM |
| `INPUT_TERM` | bi | Front/rear terminals switch |
| `ASYN` | asyn | Port diagnostics / connection |

## How it works

- Function-dependent commands are built from `FUNC_PFX`, an mbbi whose
  state strings are the SCPI subsystems (`VOLT:DC`, `CURR:AC`, `RES`, ...),
  through StreamDevice redirection: `out "%($1FUNC_PFX)s:RANG %g"`.
- Readback chain, run every `RB_SCAN` and after every setpoint:
  `REFRESH → FUNC_RB → FUNC_PFX → FUNC_UNIT → HAS_* → EGU_* → RB_FAN`.
- Function-dependent records use `SDIS = HAS_*`, `DISV = 0`.

## Notes

- `STATE_RESTORE` loads `STATE_FILE` (default `STATE_1.sta`) from the DMM's
  current folder. A failed restore does not alarm the record: check
  `LAST_ERR_*`. Readbacks refresh afterwards, but setpoint records keep their
  last written value. `.sta` files are saved from the instrument itself.
- `STATE_CAT`, `ERR_MSG` and `LAST_ERR_MSG` are `lsi` long strings: read them
  as `<PV>.VAL$` (`caget -S`), since the plain PV truncates at 40 characters.
- `READ?` holds the port until all readings are taken (timeout 60 s).
  Keep `TRIG_COUNT × SAMP_COUNT × integration time` well below that and
  `TRIG_COUNT × SAMP_COUNT ≤ NELM`. With an external trigger that never
  arrives, other records wait up to 60 s.
- The 10 A current jack exists only on the front panel; with the rear
  terminals selected the instrument forces 3 A (error -221).
- Other SCPI clients (web UI, `testScript/scpi_probe.py`) share the
  instrument state and error queue with the IOC: errors they cause show
  up in `LAST_ERR_*`, and the IOC's polling can consume their errors.
- NPLC 0.001/0.002/0.006 need the DIG option and are not offered.
