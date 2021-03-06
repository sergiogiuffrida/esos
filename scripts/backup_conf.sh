#! /bin/sh

# Use this script to create a full backup/archive of the entire ESOS
# configuration that resides on the persistent boot media file system
# (LABEL=esos_conf). The archive is created in the /tmp directory and
# the absolute path of the file name is returned.

ARCHIVE_NAME="esos_backup_conf-$(date +%s)"
ARCHIVE_DIR="/tmp/${ARCHIVE_NAME}"
ARCHIVE_FILE="${ARCHIVE_DIR}.tgz"
SYNC_LOCK="/tmp/conf_sync_lock"
CONF_MNT="/mnt/conf"

# We use a temporary directory while collecting the data
mkdir -p ${ARCHIVE_DIR} || exit 1

# Sync the config here for good measure
conf_sync.sh || exit 1

# Prevent conf_sync.sh from running
touch ${SYNC_LOCK} || exit 1
trap 'rm -f ${SYNC_LOCK}' 0

# Mount the ESOS config FS
mount ${CONF_MNT} || exit 1

# Grab all items from the config
rsync -aq ${CONF_MNT}/* ${ARCHIVE_DIR} || exit 1

# Make the tarball
tar cpfz ${ARCHIVE_FILE} --transform 's,^tmp/,,' \
    ${ARCHIVE_DIR} 2> /dev/null || exit 1

# All done
umount ${CONF_MNT} || exit 1
rm -rf ${ARCHIVE_DIR} || exit 1
echo "${ARCHIVE_FILE}"

