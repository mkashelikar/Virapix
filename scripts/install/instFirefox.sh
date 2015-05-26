. ../scripts/firefoxUtil.sh

mkdir -p $NEWROOT/usr/lib/mozilla/plugins

! rpmi lib64xulrunner1.9.1.3-1.9.1.3-2mdv2010.0.x86_64.rpm && exit -1
! rpmi firefox-3.5.3-2mdv2010.0.x86_64.rpm && exit -1

! rpmi flash-plugin-10.1.82.76-release.i386.rpm && exit -1
FlashSetup install

! rpmi nspluginwrapper-i386-1.3.0-1mdv2010.0.x86_64.rpm && exit -1

! rpmi nspluginwrapper-1.3.0-1mdv2010.0.x86_64.rpm && exit -1
NsPluginWrapperSetup install

! rpmi swfdec-mozilla-0.8.2-2mdv2010.0.x86_64.rpm && exit -1
