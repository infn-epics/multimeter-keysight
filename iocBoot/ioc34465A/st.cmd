#!../../bin/linux-x86_64/keysight34465A
# ================================================================
# Keysight 34465A Truevolt DMM — IOC startup
# ================================================================

< envPaths

## Instrument and PV naming (override with epicsEnvSet before running)
epicsEnvSet("DMM_IP",   "192.168.190.18")
epicsEnvSet("DMM_PORT", "5025")
epicsEnvSet("P",        "KEYSIGHT")
epicsEnvSet("R",        "DMM1")
## Max readings per acquisition (TRIG:COUN x SAMP:COUN)
epicsEnvSet("NELM",     "1000")

epicsEnvSet("STREAM_PROTOCOL_PATH", "$(TOP)/db")
epicsEnvSet("EPICS_CA_MAX_ARRAY_BYTES", "100000")

cd "${TOP}"

## Register all support components
dbLoadDatabase "dbd/keysight34465A.dbd"
keysight34465A_registerRecordDeviceDriver pdbbase

## Raw SCPI socket. portName, hostInfo, priority, noAutoConnect, noProcessEos
drvAsynIPPortConfigure("DMM", "$(DMM_IP):$(DMM_PORT)", 0, 0, 0)

## Uncomment for protocol debug
#asynSetTraceMask("DMM", 0, 0x9)
#asynSetTraceIOMask("DMM", 0, 0x2)
#var streamDebug 1

## CONV_*: derived output = reading * CONV_FACTOR + CONV_OFFSET, in CONV_EGU
dbLoadRecords("db/keysight34465A.db", "P=$(P),R=$(R),PORT=DMM,NELM=$(NELM),SCAN=1 second,RB_SCAN=5 second,CONV_FACTOR=1,CONV_OFFSET=0,CONV_EGU=,CONV_PREC=4")

cd "${TOP}/iocBoot/${IOC}"
iocInit

dbl > "${TOP}/iocBoot/${IOC}/pvlist.txt"
