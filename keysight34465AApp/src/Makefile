TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------

# Support library (linked by the INFN generic IOC via ibek-support-infn)
LIBRARY_IOC += keysight34465ASupport
DBD += keysightStats.dbd
keysight34465ASupport_SRCS += keysightStats.c
keysight34465ASupport_LIBS += $(EPICS_BASE_IOC_LIBS)

# Standalone IOC
PROD_IOC = keysight34465A
DBD += keysight34465A.dbd

keysight34465A_DBD += base.dbd
keysight34465A_DBD += asyn.dbd
keysight34465A_DBD += drvAsynIPPort.dbd
keysight34465A_DBD += stream.dbd
keysight34465A_DBD += keysightStats.dbd

keysight34465A_SRCS += keysight34465A_registerRecordDeviceDriver.cpp
keysight34465A_SRCS_DEFAULT += keysight34465AMain.cpp

keysight34465A_LIBS += keysight34465ASupport stream asyn
keysight34465A_LIBS += $(EPICS_BASE_IOC_LIBS)

#----------------------------------------
include $(TOP)/configure/RULES
