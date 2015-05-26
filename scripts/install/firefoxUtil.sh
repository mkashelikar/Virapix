FlashSetup()
{
    # These are the standard browser locations as found within Red Hat, Mandrake and SuSE Linux, and the tarball installers.
    local LOCATIONS="$NEWROOT/usr/lib/mozilla $NEWROOT/usr/lib/mozilla-* $NEWROOT/usr/lib/firefox-* $NEWROOT/usr/lib/seamonkey-* \
    $NEWROOT/usr/lib/netscape $NEWROOT/usr/lib/opera $NEWROOT/usr/lib/firefox $NEWROOT/usr/local/netscape $NEWROOT/usr/local/mozilla \
    $NEWROOT/usr/local/firefox $NEWROOT/usr/local/seamonkey $NEWROOT/opt/mozilla $NEWROOT/opt/netscape $NEWROOT/opt/firefox $NEWROOT/opt/seamonkey" 

    deleteold() 
    {
        local FILES="libflashplayer.so ShockwaveFlash.class flashplayer.xpt libgnashplugin.so"

        for DIR in $LOCATIONS
        do
            # Skip symlinks
            if [ -h $DIR ]; then continue; fi

            for F in $FILES
            do
                # Add old plugin files to delete lists
                [ -f $DIR/plugins/$F ] && DELETELIST="$DELETELIST $DIR/plugins/$F"

                # Add symbolic links to the delete list
                [ -h $DIR/plugins/$F ] && DELETELIST="$DELETELIST $DIR/plugins/$F"
            done
        done

        # Delete files if delete list contains files.
        [ "x$DELETELIST" != "x" ] && rm -f $DELETELIST

        # Remove $NEWROOT/etc/flash.license as it is not used anymore
        [ -f $NEWROOT/etc/flash.license ] && rm -f $NEWROOT/etc/flash.license
    }

    detectbrowsers() 
    {
        # Detect Mozilla plugin compatible browsers
        for DIR in $LOCATIONS
        do
            # Skip symlinks
            if [ -h $DIR ]; then continue; fi
            if [ -d $DIR/plugins ]; then export LIST="$LIST $DIR"; fi
        done
    }

    createlinks() 
    {
        # Link Mozilla plugin compatible browsers
        for DIR in $LIST
        do
            ln -sf $NEWROOT/usr/lib/flash-plugin/libflashplayer.so $DIR/plugins/libflashplayer.so
        done
    }

    deletelinks() 
    {
        # Delete symlinks
        # Remove Mozilla plugin compatible browsers
        for DIR in $LIST
        do
            rm -f $DIR/plugins/libflashplayer.so
        done
    }

    #=======================
    # Main Section
    #=======================
    # Pre-Uninstall
    if [ "$1" = "preun" ]; then
        detectbrowsers
        deletelinks
        return 0
    fi

    # Installation
    if [ "$1" = "install" ]; then
        deleteold
        detectbrowsers
        createlinks
        return 0
    fi

    # Manual Setup
    detectbrowsers
    createlinks
}

NsPluginWrapperSetup()
{
    local OLDHOME=$HOME
    local VIRAPIX=$NEWROOT/virapix/bin/virapix 
    local PATCHELF=$NEWROOT/usr/bin/patchelf

    export HOME=$NEWROOT/home/xguest #Doesn't matter even if the directory doesn't exist. This trick is necessary to properly run virapix.

    $PATCHELF --set-rpath $NEWROOT/usr/lib64 $VIRAPIX

    if [ "$1" = install ]
    then
        mkdir -p $NEWROOT/etc/sysconfig
        $VIRAPIX $NEWROOT/usr/bin/nspluginwrapper -v -i $NEWROOT/usr/lib/flash-plugin/libflashplayer.so
        rm -f $NEWROOT/virapix/log/virapix.log

        if [ ! -f $NEWROOT/etc/sysconfig/nspluginwrapper ]
        then
            cat > $NEWROOT/etc/sysconfig/nspluginwrapper << EOF
            USE_NSPLUGINWRAPPER=yes
            MDV_PLUGINS="flashplayer,nppdf,rpnp,nphelix"
            USER_PLUGINS=""
EOF
        else
            sed -i "s/MDV_PLUGINS=.*/MDV_PLUGINS=\"flashplayer,nppdf,rpnp,nphelix\"/" $NEWROOT/etc/sysconfig/nspluginwrapper
        fi
    elif [ "$1" = preun ]
    then
        $VIRAPIX $NEWROOT/usr/bin/nspluginwrapper -v -r $NEWROOT/usr/lib64/mozilla/plugins/npwrapper.libflashplayer.so
    fi

    $PATCHELF --set-rpath "" $VIRAPIX
    chown xguest $VIRAPIX
    chgrp xguest $VIRAPIX
    export HOME=$OLDHOME
}
