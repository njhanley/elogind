/* SPDX-License-Identifier: LGPL-2.1-or-later */
#pragma once

#include "bus-error.h"

#if 0 /// only system command elogind knows are needed
#define BUS_ERROR_NO_SUCH_UNIT                 "org.freedesktop.systemd1.NoSuchUnit"
#define BUS_ERROR_NO_UNIT_FOR_PID              "org.freedesktop.systemd1.NoUnitForPID"
#define BUS_ERROR_NO_UNIT_FOR_INVOCATION_ID    "org.freedesktop.systemd1.NoUnitForInvocationID"
#define BUS_ERROR_UNIT_EXISTS                  "org.freedesktop.systemd1.UnitExists"
#define BUS_ERROR_LOAD_FAILED                  "org.freedesktop.systemd1.LoadFailed"
#define BUS_ERROR_BAD_UNIT_SETTING             "org.freedesktop.systemd1.BadUnitSetting"
#define BUS_ERROR_JOB_FAILED                   "org.freedesktop.systemd1.JobFailed"
#define BUS_ERROR_NO_SUCH_JOB                  "org.freedesktop.systemd1.NoSuchJob"
#define BUS_ERROR_NOT_SUBSCRIBED               "org.freedesktop.systemd1.NotSubscribed"
#define BUS_ERROR_ALREADY_SUBSCRIBED           "org.freedesktop.systemd1.AlreadySubscribed"
#define BUS_ERROR_ONLY_BY_DEPENDENCY           "org.freedesktop.systemd1.OnlyByDependency"
#define BUS_ERROR_TRANSACTION_JOBS_CONFLICTING "org.freedesktop.systemd1.TransactionJobsConflicting"
#endif // 0

#if 0 /// no machined in elogind
#endif // 0


#if 0 /// more services unsupported by elogind
#endif // 0

#define BUS_ERROR_TRANSACTION_ORDER_IS_CYCLIC  "org.freedesktop.elogind1.TransactionOrderIsCyclic"
#define BUS_ERROR_TRANSACTION_IS_DESTRUCTIVE   "org.freedesktop.elogind1.TransactionIsDestructive"
#define BUS_ERROR_UNIT_MASKED                  "org.freedesktop.elogind1.UnitMasked"
#define BUS_ERROR_UNIT_GENERATED               "org.freedesktop.elogind1.UnitGenerated"
#define BUS_ERROR_UNIT_LINKED                  "org.freedesktop.elogind1.UnitLinked"
#define BUS_ERROR_JOB_TYPE_NOT_APPLICABLE      "org.freedesktop.elogind1.JobTypeNotApplicable"
#define BUS_ERROR_NO_ISOLATION                 "org.freedesktop.elogind1.NoIsolation"
#define BUS_ERROR_SHUTTING_DOWN                "org.freedesktop.elogind1.ShuttingDown"
#define BUS_ERROR_SCOPE_NOT_RUNNING            "org.freedesktop.elogind1.ScopeNotRunning"
#define BUS_ERROR_NO_SUCH_DYNAMIC_USER         "org.freedesktop.elogind1.NoSuchDynamicUser"
#define BUS_ERROR_NOT_REFERENCED               "org.freedesktop.elogind1.NotReferenced"
#define BUS_ERROR_DISK_FULL                    "org.freedesktop.elogind1.DiskFull"
#define BUS_ERROR_NOTHING_TO_CLEAN             "org.freedesktop.elogind1.NothingToClean"
#define BUS_ERROR_UNIT_BUSY                    "org.freedesktop.elogind1.UnitBusy"
#define BUS_ERROR_UNIT_INACTIVE                "org.freedesktop.elogind1.UnitInactive"

#define BUS_ERROR_NO_SUCH_MACHINE              "org.freedesktop.machine1.NoSuchMachine"
#define BUS_ERROR_NO_SUCH_IMAGE                "org.freedesktop.machine1.NoSuchImage"
#define BUS_ERROR_NO_MACHINE_FOR_PID           "org.freedesktop.machine1.NoMachineForPID"
#define BUS_ERROR_MACHINE_EXISTS               "org.freedesktop.machine1.MachineExists"
#define BUS_ERROR_NO_PRIVATE_NETWORKING        "org.freedesktop.machine1.NoPrivateNetworking"
#define BUS_ERROR_NO_SUCH_USER_MAPPING         "org.freedesktop.machine1.NoSuchUserMapping"
#define BUS_ERROR_NO_SUCH_GROUP_MAPPING        "org.freedesktop.machine1.NoSuchGroupMapping"

#define BUS_ERROR_NO_SUCH_PORTABLE_IMAGE       "org.freedesktop.portable1.NoSuchImage"
#define BUS_ERROR_BAD_PORTABLE_IMAGE_TYPE      "org.freedesktop.portable1.BadImageType"

#define BUS_ERROR_NO_SUCH_SESSION              "org.freedesktop.login1.NoSuchSession"
#define BUS_ERROR_NO_SESSION_FOR_PID           "org.freedesktop.login1.NoSessionForPID"
#define BUS_ERROR_NO_SUCH_USER                 "org.freedesktop.login1.NoSuchUser"
#define BUS_ERROR_NO_USER_FOR_PID              "org.freedesktop.login1.NoUserForPID"
#define BUS_ERROR_NO_SUCH_SEAT                 "org.freedesktop.login1.NoSuchSeat"
#define BUS_ERROR_SESSION_NOT_ON_SEAT          "org.freedesktop.login1.SessionNotOnSeat"
#define BUS_ERROR_NOT_IN_CONTROL               "org.freedesktop.login1.NotInControl"
#define BUS_ERROR_DEVICE_IS_TAKEN              "org.freedesktop.login1.DeviceIsTaken"
#define BUS_ERROR_DEVICE_NOT_TAKEN             "org.freedesktop.login1.DeviceNotTaken"
#define BUS_ERROR_OPERATION_IN_PROGRESS        "org.freedesktop.login1.OperationInProgress"
#define BUS_ERROR_SLEEP_VERB_NOT_SUPPORTED     "org.freedesktop.login1.SleepVerbNotSupported"
#define BUS_ERROR_SESSION_BUSY                 "org.freedesktop.login1.SessionBusy"
#define BUS_ERROR_NOT_YOUR_DEVICE              "org.freedesktop.login1.NotYourDevice"

#define BUS_ERROR_AUTOMATIC_TIME_SYNC_ENABLED  "org.freedesktop.timedate1.AutomaticTimeSyncEnabled"
#define BUS_ERROR_NO_NTP_SUPPORT               "org.freedesktop.timedate1.NoNTPSupport"

#define BUS_ERROR_NO_SUCH_PROCESS              "org.freedesktop.elogind1.NoSuchProcess"

#define BUS_ERROR_NO_NAME_SERVERS              "org.freedesktop.resolve1.NoNameServers"
#define BUS_ERROR_INVALID_REPLY                "org.freedesktop.resolve1.InvalidReply"
#define BUS_ERROR_NO_SUCH_RR                   "org.freedesktop.resolve1.NoSuchRR"
#define BUS_ERROR_CNAME_LOOP                   "org.freedesktop.resolve1.CNameLoop"
#define BUS_ERROR_ABORTED                      "org.freedesktop.resolve1.Aborted"
#define BUS_ERROR_NO_SUCH_SERVICE              "org.freedesktop.resolve1.NoSuchService"
#define BUS_ERROR_DNSSEC_FAILED                "org.freedesktop.resolve1.DnssecFailed"
#define BUS_ERROR_NO_TRUST_ANCHOR              "org.freedesktop.resolve1.NoTrustAnchor"
#define BUS_ERROR_RR_TYPE_UNSUPPORTED          "org.freedesktop.resolve1.ResourceRecordTypeUnsupported"
#define BUS_ERROR_NO_SUCH_LINK                 "org.freedesktop.resolve1.NoSuchLink"
#define BUS_ERROR_LINK_BUSY                    "org.freedesktop.resolve1.LinkBusy"
#define BUS_ERROR_NETWORK_DOWN                 "org.freedesktop.resolve1.NetworkDown"
#define BUS_ERROR_NO_SUCH_DNSSD_SERVICE        "org.freedesktop.resolve1.NoSuchDnssdService"
#define BUS_ERROR_DNSSD_SERVICE_EXISTS         "org.freedesktop.resolve1.DnssdServiceExists"
#define _BUS_ERROR_DNS                         "org.freedesktop.resolve1.DnsError."

#define BUS_ERROR_NO_SUCH_TRANSFER             "org.freedesktop.import1.NoSuchTransfer"
#define BUS_ERROR_TRANSFER_IN_PROGRESS         "org.freedesktop.import1.TransferInProgress"

#define BUS_ERROR_NO_PRODUCT_UUID              "org.freedesktop.hostname1.NoProductUUID"

#define BUS_ERROR_SPEED_METER_INACTIVE         "org.freedesktop.network1.SpeedMeterInactive"
#define BUS_ERROR_UNMANAGED_INTERFACE          "org.freedesktop.network1.UnmanagedInterface"

#define BUS_ERROR_NO_SUCH_HOME                 "org.freedesktop.home1.NoSuchHome"
#define BUS_ERROR_UID_IN_USE                   "org.freedesktop.home1.UIDInUse"
#define BUS_ERROR_USER_NAME_EXISTS             "org.freedesktop.home1.UserNameExists"
#define BUS_ERROR_HOME_EXISTS                  "org.freedesktop.home1.HomeExists"
#define BUS_ERROR_HOME_ALREADY_ACTIVE          "org.freedesktop.home1.HomeAlreadyActive"
#define BUS_ERROR_HOME_ALREADY_FIXATED         "org.freedesktop.home1.HomeAlreadyFixated"
#define BUS_ERROR_HOME_UNFIXATED               "org.freedesktop.home1.HomeUnfixated"
#define BUS_ERROR_HOME_NOT_ACTIVE              "org.freedesktop.home1.HomeNotActive"
#define BUS_ERROR_HOME_ABSENT                  "org.freedesktop.home1.HomeAbsent"
#define BUS_ERROR_HOME_BUSY                    "org.freedesktop.home1.HomeBusy"
#define BUS_ERROR_BAD_PASSWORD                 "org.freedesktop.home1.BadPassword"
#define BUS_ERROR_BAD_RECOVERY_KEY             "org.freedesktop.home1.BadRecoveryKey"
#define BUS_ERROR_LOW_PASSWORD_QUALITY         "org.freedesktop.home1.LowPasswordQuality"
#define BUS_ERROR_BAD_PASSWORD_AND_NO_TOKEN    "org.freedesktop.home1.BadPasswordAndNoToken"
#define BUS_ERROR_TOKEN_PIN_NEEDED             "org.freedesktop.home1.TokenPinNeeded"
#define BUS_ERROR_TOKEN_PROTECTED_AUTHENTICATION_PATH_NEEDED \
                                               "org.freedesktop.home1.TokenProtectedAuthenticationPathNeeded"
#define BUS_ERROR_TOKEN_USER_PRESENCE_NEEDED   "org.freedesktop.home1.TokenUserPresenceNeeded"
#define BUS_ERROR_TOKEN_ACTION_TIMEOUT         "org.freedesktop.home1.TokenActionTimeout"
#define BUS_ERROR_TOKEN_PIN_LOCKED             "org.freedesktop.home1.TokenPinLocked"
#define BUS_ERROR_TOKEN_BAD_PIN                "org.freedesktop.home1.BadPin"
#define BUS_ERROR_TOKEN_BAD_PIN_FEW_TRIES_LEFT "org.freedesktop.home1.BadPinFewTriesLeft"
#define BUS_ERROR_TOKEN_BAD_PIN_ONE_TRY_LEFT   "org.freedesktop.home1.BadPinOneTryLeft"
#define BUS_ERROR_BAD_SIGNATURE                "org.freedesktop.home1.BadSignature"
#define BUS_ERROR_HOME_RECORD_MISMATCH         "org.freedesktop.home1.RecordMismatch"
#define BUS_ERROR_HOME_RECORD_DOWNGRADE        "org.freedesktop.home1.RecordDowngrade"
#define BUS_ERROR_HOME_RECORD_SIGNED           "org.freedesktop.home1.RecordSigned"
#define BUS_ERROR_BAD_HOME_SIZE                "org.freedesktop.home1.BadHomeSize"
#define BUS_ERROR_NO_PRIVATE_KEY               "org.freedesktop.home1.NoPrivateKey"
#define BUS_ERROR_HOME_LOCKED                  "org.freedesktop.home1.HomeLocked"
#define BUS_ERROR_HOME_NOT_LOCKED              "org.freedesktop.home1.HomeNotLocked"
#define BUS_ERROR_NO_DISK_SPACE                "org.freedesktop.home1.NoDiskSpace"
#define BUS_ERROR_TOO_MANY_OPERATIONS          "org.freedesktop.home1.TooManyOperations"
#define BUS_ERROR_AUTHENTICATION_LIMIT_HIT     "org.freedesktop.home1.AuthenticationLimitHit"
#define BUS_ERROR_HOME_CANT_AUTHENTICATE       "org.freedesktop.home1.HomeCantAuthenticate"

BUS_ERROR_MAP_ELF_USE(bus_common_errors);
