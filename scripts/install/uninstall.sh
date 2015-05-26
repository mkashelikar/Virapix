#!/bin/bash

[ -L "$0" ] && { echo "Please run the uninstall script directly, not through its symbolic link."; exit -1; }

SCRIPTLOC="$(cd "${0%/*}" 2>/dev/null; echo "$PWD")"
[ "$PWD" != "$SCRIPTLOC" ] && { echo "You have to be in $SCRIPTLOC to uninstall virapix."; exit -1; }

#Source common functionality
. scripts/util.sh

################################################### Validation #############################################

[ "$USER" != root ] && { echo "You must be logged in as root to uninstall virapix."; exit -1; }

! ValidateArgs "$#" "$1" "uninstall.sh" && exit -1
! ValidateMount && exit -1

echo "IMPORTANT! You have chosen to uninstall virapix suite from $MOUNTPOINT. Press y or Y to confirm or any other key to abort."
read ans
[ "$ans" != Y -a "$ans" != y ] && { umount $MOUNTPOINT; exit 0; }

################################################### Package removal #############################################

clear
echo -e "Please wait while we remove virapix suite...\n"

{ ProgressBar& } 2> /dev/null
bgPid=$!

cd ooo3
. ../scripts/uninstOffice.sh
cd ../tools
. ../scripts/uninstTools.sh
cd ../firefox
. ../scripts/uninstFirefox.sh
cd ../fluxbox
. ../scripts/uninstFluxbox.sh
cd ../virapix
. ../scripts/uninstVirapix.sh

kill -9 $bgPid
clear

cat << EOF

**************************************************************************************************************

Uninstall complete! You may have to manually delete the directories that the uninstall program did not remove. 
Remember also to first unmount the device before detaching it!

***************************************************************************************************************
EOF
