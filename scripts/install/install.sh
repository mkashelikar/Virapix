#!/bin/bash

[ -L "$0" ] && { echo "Please run the install script directly, not through its symbolic link."; exit -1; }

SCRIPTLOC="$(cd "${0%/*}" 2>/dev/null; echo "$PWD")"
[ "$PWD" != "$SCRIPTLOC" ] && { echo "You have to be in $SCRIPTLOC to install virapix."; exit -1; }

#Source common functionality
. scripts/util.sh

################################################### Validation #############################################

[ "$USER" != root ] && { echo "You must be logged in as root to install virapix."; exit -1; }

! ValidateArgs "$#" "$1" && exit -1
! ValidateLogin && exit -1
! ValidateMount && exit -1
! CheckDskSpace && exit -1

echo "IMPORTANT! You have chosen to install virapix suite in $MOUNTPOINT. Press y or Y to confirm or any other key to abort."
read ans
[ "$ans" != Y -a "$ans" != y ] && { umount $MOUNTPOINT; exit 0; }

################################################### Directory creation #############################################

CreateDirs

################################################### Package installation #############################################

clear
echo -e "Please wait while we install virapix suite...\n"

cd virapix
. ../scripts/instVirapix.sh 
cd ../fluxbox
. ../scripts/instFluxbox.sh
cd ../firefox
. ../scripts/instFirefox.sh
cd ../tools
. ../scripts/instTools.sh
cd ../ooo3
. ../scripts/instOffice.sh

#Thanks to packages that "own" the root (/) directory which automatically makes root the owner of the directory...
chown xguest $NEWROOT
chgrp xguest $NEWROOT

cat << EOF

***********************************************************************************************************************

Congratulations! Virapix suite is successfully installed. Henceforth, all operations with virapix (except uninstall!)
will have to be performed as xguest. To begin, run $NEWROOT/virapix/bin/checkdeps before anything else if virapix is 
being run for the first time on a machine. For more information, please consult 
$NEWROOT/usr/share/doc/virapix/README. Lastly, don't forget to unmount the USB device before physically removing it!

***********************************************************************************************************************
EOF
