#! /bin/sh


ARCHIVE_NAME="esos_backup_conf_$(hostname -s)_$(date +%Y%m%d)"
ARCHIVE_DIR="/tmp/${ARCHIVE_NAME}"
ARCHIVE_FILE="${ARCHIVE_DIR}.tgz"
SYNC_LOCK="/tmp/conf_sync_lock"


# Prevent conf_sync.sh from running
touch ${SYNC_LOCK} || exit 1
trap 'rm -f ${SYNC_LOCK}' 0


# Make the tarball
tar cvzfp /tmp/${ARCHIVE_FILE} --exclude='rc.d' --exclude='.git' /var/lib /opt /root/.ssh /etc 2> /dev/null || exit 1
rm -f ${SYNC_LOCK}

# All done
echo "${ARCHIVE_FILE}"
