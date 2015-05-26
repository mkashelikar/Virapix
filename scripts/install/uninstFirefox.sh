. ../scripts/firefoxUtil.sh

rpme swfdec-mozilla

NsPluginWrapperSetup preun
rpme nspluginwrapper
rpme nspluginwrapper-i386

FlashSetup preun
rpme flash-plugin

rpme firefox
rpme lib64xulrunner1.9.1.3
