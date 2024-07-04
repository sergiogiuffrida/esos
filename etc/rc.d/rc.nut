#! /bin/sh

source /etc/rc.d/common

UPSD="/usr/sbin/upsd"
UPSMON="/usr/sbin/upsmon"
UPSDRVCTL="/usr/sbin/upsdrvctl"
UPSD_LOCK="/var/lock/upsd"
UPSMON_LOCK="/var/lock/upsmon"
UPSDRVCTL_LOCK="/var/lock/upsdrvctl"
NUT_USER="nutmon"

check_args ${@}

start() {
    /bin/echo "Starting NUT UPS drivers..."
    ${UPSDRVCTL} -u ${NUT_USER} start || exit 1
    /bin/touch ${UPSDRVCTL_LOCK}
    /bin/echo "Starting NUT UPS daemon..."
    ${UPSD} -u ${NUT_USER} || exit 1
    /bin/touch ${UPSD_LOCK}
    /bin/echo "Starting NUT UPS monitor..."
    ${UPSMON} -u ${NUT_USER} || exit 1
    /bin/touch ${UPSMON_LOCK}
}

stop() {
    /bin/echo "Stopping NUT UPS monitor..."
    ${UPSMON} -c stop && /bin/rm -f ${UPSMON_LOCK}
    /bin/echo "Stopping NUT UPS daemon..."
    ${UPSD} -c stop && /bin/rm -f ${UPSD_LOCK}
    /bin/echo "Stopping NUT UPS drivers..."
    ${UPSDRVCTL} stop && /bin/rm -f ${UPSDRVCTL_LOCK}
}

status() {
    /usr/bin/pidof ${UPSD} > /dev/null 2>&1
    exit ${?}
}

# Perform specified action
${1}
