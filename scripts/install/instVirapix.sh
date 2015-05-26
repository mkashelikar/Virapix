! rpmi lib64log4cpp4-1.0-0mdv2010.0.x86_64.rpm && exit -1
! rpmi patchelf-0.5-1.x86_64.rpm && exit -1

if rpmi virapix-1.0-1mdv2010.0.x86_64.rpm
then
    virapixDir=$NEWROOT/virapix
    cfgDir=$virapixDir/cfg
    fluxDir=$XGUESTHOME/.fluxbox

    mkdir -p $fluxDir
    cp -f $cfgDir/fluxbox-menu $fluxDir/menu

    chown -R xguest $virapixDir $fluxDir
    chgrp -R xguest $virapixDir $fluxDir
else
    exit -1
fi
