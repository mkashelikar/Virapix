#!/bin/bash

shopt -s expand_aliases

#Source helpful functions
. ./instUtil.sh

################################################### Validation #############################################

! ValidateMount $# $1 && exit -1

echo "IMPORTANT! You have chosen $1 to install OpenOffice in. Press Y to confirm or any other key to abort."
read ans

if [ "$ans" != Y -a "$ans" != y ]
then
    umount $1
    exit 0
fi

! ValidateLogin && exit -1

if ! rpm -q fontconfig > /dev/null 2>&1
then
    echo "fontconfig package needs to be installed first."
    exit -1
fi

################################################### Variable declarations #############################################

NEWROOT="$1/rt"
ALTDIR=$NEWROOT/etc/alternatives
ADMINDIR=$NEWROOT/var/lib/dpkg/alternatives
XGUESTHOME=$NEWROOT/$xguestHome
FONTCONFDIR=$NEWROOT/etc/fonts
FONTCACHEDIR=$NEWROOT/var/cache/fontconfig

################################################### Directory creation #############################################

if [ ! -d $NEWROOT ]
then
   mkdir $NEWROOT
   chown xguest $NEWROOT
   chgrp xguest $NEWROOT
fi

if [ ! -d $XGUESTHOME/tmp ]
then
    mkdir -p $XGUESTHOME/tmp
    chown xguest $XGUESTHOME/tmp 
    chgrp xguest $XGUESTHOME/tmp
fi

[ ! -d $ALTDIR ] && mkdir -p $ALTDIR
[ ! -d $ADMINDIR ] && mkdir -p $ADMINDIR
[ ! -d $FONTCACHEDIR ] && mkdir -p $FONTCACHEDIR

alias rpmi="rpm --root $NEWROOT --nodeps --noscripts -i"
alias ualti="update-alternatives --altdir $ALTDIR --admindir $ADMINDIR --install"
alias ualtr="update-alternatives --altdir $ALTDIR --admindir $ADMINDIR --remove"
alias ualta="update-alternatives --altdir $ALTDIR --admindir $ADMINDIR --auto"
alias unopkgas='$NEWROOT/usr/bin/unopkg add --shared'
alias unopkgls='$NEWROOT/usr/bin/unopkg list --shared'

################################################### RPM installation #############################################

cd ooo3

if rpmi tomcat5-servlet-2.4-api-5.5.27-0.5.0mdv2010.0.noarch.rpm
then
    ualti $NEWROOT/usr/share/java/servlet.jar servlet $NEWROOT/usr/share/java/tomcat5-servlet-2.4-api.jar 20400
    ualti $NEWROOT/usr/share/java/servlet_2_4_api.jar servlet_2_4_api $NEWROOT/usr/share/java/tomcat5-servlet-2.4-api.jar 20400
else
    exit -1
fi

if rpmi jpackage-utils-1.7.5-4.0.5mdv2010.0.x86_64.rpm
then
    cp -af $NEWROOT/usr/lib/security/classpath.security.real $NEWROOT/usr/lib/security/classpath.security
    cp -af $NEWROOT/usr/lib/logging.properties.real $NEWROOT/usr/lib/logging.properties

    cat $NEWROOT/usr/lib/security/classpath.security | grep -v "^security.provider." > \
        $NEWROOT/usr/lib/security/classpath.security.bak
    mv -f $NEWROOT/usr/lib/security/classpath.security.bak $NEWROOT/usr/lib/security/classpath.security

    providers=$(ls $NEWROOT/etc/java/security/security.d | sort | awk -F- '{ print $2 }')
    count=0

    for provider in $providers
    do
        case $provider in
        *.rpmsave|*.rpmorig|*.rpmnew|*~|*.orig|*.bak)
            ;;

        *)
            count=$((count + 1))
            echo "security.provider."$count"="$provider >> $NEWROOT/usr/lib/security/classpath.security
            ;;
        esac
    done
else
    exit -1
fi

if rpmi jakarta-commons-codec-1.3-9.4.3mdv2010.0.x86_64.rpm
then
    if [ -x $NEWROOT/usr/bin/rebuild-gcj-db ]
    then 
        $NEWROOT/usr/bin/rebuild-gcj-db || true
    fi
else
    exit -1
fi

if rpmi jakarta-commons-lang-2.3-2.3.3mdv2010.0.noarch.rpm
then
    echo -e "<dependencies>\n" > $NEWROOT/etc/maven/maven2-depmap.xml

    if [ -d $NEWROOT/etc/maven/fragments ] && [ -n "`find $NEWROOT/etc/maven/fragments -type f`" ]
    then
        cat $NEWROOT/etc/maven/fragments/* >> $NEWROOT/etc/maven/maven2-depmap.xml
    fi

    echo -e "</dependencies>\n" >> $NEWROOT/etc/maven/maven2-depmap.xml
else
    exit -1
fi

! rpmi lib64raptor1-1.4.19-1mdv2010.0.x86_64.rpm && exit -1
! rpmi lib64rasqal1-0.9.16-3mdv2010.0.x86_64.rpm && exit -1
! rpmi lib64ieee1284_3-0.2.11-6mdv2010.0.x86_64.rpm && exit -1
! rpmi lib64wpd0.8_8-0.8.14-2mdv2010.0.x86_64.rpm && exit -1
! rpmi lib64wps0.1_1-0.1.2-3mdv2010.0.x86_64.rpm && exit -1
! rpmi lib64wpg-0.1_1-0.1.3-2mdv2010.0.x86_64.rpm && exit -1
! rpmi lib64redland0-1.0.9-4mdv2010.0.x86_64.rpm && exit -1
! rpmi lib64sane1-1.0.20-7mdv2010.0.x86_64.rpm && exit -1
! rpmi sane-backends-iscan-1.0.20-7mdv2010.0.x86_64.rpm && exit -1
! rpmi sane-backends-1.0.20-7mdv2010.0.x86_64.rpm && exit -1
! rpmi jakarta-commons-logging-1.1-3.3.5mdv2010.0.noarch.rpm && exit -1

if rpmi jakarta-commons-httpclient-3.1-2.3.3mdv2010.0.x86_64.rpm
then
    if [ -x $NEWROOT/usr/bin/rebuild-gcj-db ]
    then 
        $NEWROOT/usr/bin/rebuild-gcj-db || true
    fi
else
    exit -1
fi

! rpmi hsqldb-1.8.0.10-0.0.3mdv2010.0.noarch.rpm && exit -1
! rpmi clipart-openclipart-0.18-10mdv2010.0.noarch.rpm && exit -1
! rpmi java-1.6.0-sun-alsa-1.6.0.16-1mdv2010.0.x86_64.rpm && exit -1
! rpmi java-1.6.0-sun-jdbc-1.6.0.16-1mdv2010.0.x86_64.rpm && exit -1

if rpmi java-1.6.0-sun-1.6.0.16-1mdv2010.0.x86_64.rpm
then
    ualti $NEWROOT/usr/bin/java java $NEWROOT/usr/lib/jvm/jre-1.6.0-sun/bin/java 1600 \
    --slave $NEWROOT/usr/bin/keytool keytool $NEWROOT/usr/lib/jvm/jre-1.6.0-sun/bin/keytool \
    --slave $NEWROOT/usr/bin/orbd orbd $NEWROOT/usr/lib/jvm/jre-1.6.0-sun/bin/orbd \
    --slave $NEWROOT/usr/bin/policytool policytool $NEWROOT/usr/lib/jvm/jre-1.6.0-sun/bin/policytool \
    --slave $NEWROOT/usr/bin/rmid rmid $NEWROOT/usr/lib/jvm/jre-1.6.0-sun/bin/rmid \
    --slave $NEWROOT/usr/bin/rmiregistry rmiregistry $NEWROOT/usr/lib/jvm/jre-1.6.0-sun/bin/rmiregistry \
    --slave $NEWROOT/usr/bin/servertool servertool $NEWROOT/usr/lib/jvm/jre-1.6.0-sun/bin/servertool \
    --slave $NEWROOT/usr/bin/tnameserv tnameserv $NEWROOT/usr/lib/jvm/jre-1.6.0-sun/bin/tnameserv \
    --slave $NEWROOT/usr/share/man/man1/java.1.lzma java.1.lzma $NEWROOT/usr/share/man/man1/java-java-1.6.0-sun.1.lzma \
    --slave $NEWROOT/usr/share/man/man1/keytool.1.lzma keytool.1.lzma $NEWROOT/usr/share/man/man1/keytool-java-1.6.0-sun.1.lzma \
    --slave $NEWROOT/usr/share/man/man1/orbd.1.lzma orbd.1.lzma $NEWROOT/usr/share/man/man1/orbd-java-1.6.0-sun.1.lzma \
    --slave $NEWROOT/usr/share/man/man1/policytool.1.lzma policytool.1.lzma $NEWROOT/usr/share/man/man1/policytool-java-1.6.0-sun.1.lzma \
    --slave $NEWROOT/usr/share/man/man1/rmid.1.lzma rmid.1.lzma $NEWROOT/usr/share/man/man1/rmid-java-1.6.0-sun.1.lzma \
    --slave $NEWROOT/usr/share/man/man1/rmiregistry.1.lzma rmiregistry.1.lzma $NEWROOT/usr/share/man/man1/rmiregistry-java-1.6.0-sun.1.lzma \
    --slave $NEWROOT/usr/share/man/man1/servertool.1.lzma servertool.1.lzma $NEWROOT/usr/share/man/man1/servertool-java-1.6.0-sun.1.lzma \
    --slave $NEWROOT/usr/share/man/man1/tnameserv.1.lzma tnameserv.1.lzma $NEWROOT/usr/share/man/man1/tnameserv-java-1.6.0-sun.1.lzma \
    --slave $NEWROOT/usr/share/man/man1/jvisualvm.1.lzma jvisualvm.1.lzma $NEWROOT/usr/share/man/man1/jvisualvm-java-1.6.0-sun.1.lzma \
    --slave $NEWROOT/usr/share/man/ja_JP.eucJP/man1/java.1.lzma java.lzma.ja_JP.eucJP \
            $NEWROOT/usr/share/man/ja_JP.eucJP/man1/java-java-1.6.0-sun.1.lzma \
    --slave $NEWROOT/usr/share/man/ja_JP.eucJP/man1/keytool.1.lzma keytool.lzma.ja_JP.eucJP \
            $NEWROOT/usr/share/man/ja_JP.eucJP/man1/keytool-java-1.6.0-sun.1.lzma \
    --slave $NEWROOT/usr/share/man/ja_JP.eucJP/man1/orbd.1.lzma orbd.lzma.ja_JP.eucJP \
            $NEWROOT/usr/share/man/ja_JP.eucJP/man1/orbd-java-1.6.0-sun.1.lzma \
    --slave $NEWROOT/usr/share/man/ja_JP.eucJP/man1/policytool.1.lzma policytool.lzma.ja_JP.eucJP \
            $NEWROOT/usr/share/man/ja_JP.eucJP/man1/policytool-java-1.6.0-sun.1.lzma \
    --slave $NEWROOT/usr/share/man/ja_JP.eucJP/man1/rmid.1.lzma rmid.lzma.ja_JP.eucJP \
            $NEWROOT/usr/share/man/ja_JP.eucJP/man1/rmid-java-1.6.0-sun.1.lzma \
    --slave $NEWROOT/usr/share/man/ja_JP.eucJP/man1/rmiregistry.1.lzma rmiregistry.lzma.ja_JP.eucJP \
            $NEWROOT/usr/share/man/ja_JP.eucJP/man1/rmiregistry-java-1.6.0-sun.1.lzma \
    --slave $NEWROOT/usr/share/man/ja_JP.eucJP/man1/servertool.1.lzma servertool.lzma.ja_JP.eucJP \
            $NEWROOT/usr/share/man/ja_JP.eucJP/man1/servertool-java-1.6.0-sun.1.lzma \
    --slave $NEWROOT/usr/share/man/ja_JP.eucJP/man1/tnameserv.1.lzma tnameserv.lzma.ja_JP.eucJP \
            $NEWROOT/usr/share/man/ja_JP.eucJP/man1/tnameserv-java-1.6.0-sun.1.lzma \
    --slave $NEWROOT/usr/share/man/ja_JP.eucJP/man1/jvisualvm.1.lzma jvisualvm.lzma.ja_JP.eucJP \
            $NEWROOT/usr/share/man/ja_JP.eucJP/man1/jvisualvm-java-1.6.0-sun.1.lzma \
    --slave $NEWROOT/usr/bin/ControlPanel ControlPanel $NEWROOT/usr/lib/jvm/jre-1.6.0-sun/bin/ControlPanel \
    --slave $NEWROOT/usr/bin/javaws javaws $NEWROOT/usr/lib/jvm/jre-1.6.0-sun/bin/javaws \
    --slave $NEWROOT/usr/share/mime/packages/java.xml java.xml $NEWROOT/usr/lib/jvm/jre-1.6.0-sun/lib/sharedmimeinfo \
    --slave $NEWROOT/usr/lib/jvm/jre jre $NEWROOT/usr/lib/jvm/jre-1.6.0-sun \
    --slave $NEWROOT/usr/lib/jvm-exports/jre jre_exports $NEWROOT/usr/lib/jvm-exports/jre-1.6.0-sun

    for file in $NEWROOT/usr/lib/jvm/java-1.6.0-sun-1.6.0.16/jre/lib/security/local_policy.jar \
        $NEWROOT/usr/lib/jvm/jre-1.6.0-sun/lib/security/US_export_policy.jar
    do
            [ -L "$file" ] || rm -f "$file"
    done

    ualti $NEWROOT/usr/lib/jvm/jre-1.6.0-sun/lib/security/local_policy.jar jce_1.6.0_sun_local_policy \
    $NEWROOT/usr/lib/jvm-private/java-1.6.0-sun/jce/vanilla/local_policy.jar 1600 \
    --slave $NEWROOT/usr/lib/jvm/jre-1.6.0-sun/lib/security/US_export_policy.jar jce_1.6.0_sun_us_export_policy \
    $NEWROOT/usr/lib/jvm-private/java-1.6.0-sun/jce/vanilla/US_export_policy.jar

    ualti $NEWROOT/usr/lib/jvm/jre-sun jre_sun $NEWROOT/usr/lib/jvm/jre-1.6.0-sun 1600 \
    --slave $NEWROOT/usr/lib/jvm-exports/jre-sun jre_sun_exports $NEWROOT/usr/lib/jvm-exports/jre-1.6.0-sun

    ualti $NEWROOT/usr/lib/jvm/jre-1.6.0 jre_1.6.0 $NEWROOT/usr/lib/jvm/jre-1.6.0-sun 1600 \
    --slave $NEWROOT/usr/lib/jvm-exports/jre-1.6.0 jre_1.6.0_exports $NEWROOT/usr/lib/jvm-exports/jre-1.6.0-sun

    [ ! -e $NEWROOT/usr/bin/java ] && ualta java
else
    exit -1
fi

if rpmi java-1.6.0-sun-fonts-1.6.0.16-1mdv2010.0.x86_64.rpm
then
    ualti $NEWROOT/usr/share/fonts/java/LucidaBrightDemiBold.ttf LucidaBrightDemiBold.ttf \
    $NEWROOT/usr/lib/jvm/java-1.6.0-sun-1.6.0.16/jre/lib/fonts/LucidaBrightDemiBold.ttf 1600 \
    --slave $NEWROOT/usr/share/fonts/java/LucidaBrightItalic.ttf LucidaBrightItalic.ttf \
    $NEWROOT/usr/lib/jvm/java-1.6.0-sun-1.6.0.16/jre/lib/fonts/LucidaBrightItalic.ttf \
    --slave $NEWROOT/usr/share/fonts/java/LucidaSansDemiBold.ttf LucidaSansDemiBold.ttf \
    $NEWROOT/usr/lib/jvm/java-1.6.0-sun-1.6.0.16/jre/lib/fonts/LucidaSansDemiBold.ttf \
    --slave $NEWROOT/usr/share/fonts/java/LucidaTypewriterBold.ttf LucidaTypewriterBold.ttf \
    $NEWROOT/usr/lib/jvm/java-1.6.0-sun-1.6.0.16/jre/lib/fonts/LucidaTypewriterBold.ttf \
    --slave $NEWROOT/usr/share/fonts/java/LucidaBrightDemiItalic.ttf LucidaBrightDemiItalic.ttf \
    $NEWROOT/usr/lib/jvm/java-1.6.0-sun-1.6.0.16/jre/lib/fonts/LucidaBrightDemiItalic.ttf \
    --slave $NEWROOT/usr/share/fonts/java/LucidaBrightRegular.ttf LucidaBrightRegular.ttf \
    $NEWROOT/usr/lib/jvm/java-1.6.0-sun-1.6.0.16/jre/lib/fonts/LucidaBrightRegular.ttf \
    --slave $NEWROOT/usr/share/fonts/java/LucidaSansRegular.ttf LucidaSansRegular.ttf \
    $NEWROOT/usr/lib/jvm/java-1.6.0-sun-1.6.0.16/jre/lib/fonts/LucidaSansRegular.ttf \
    --slave $NEWROOT/usr/share/fonts/java/LucidaTypewriterRegular.ttf LucidaTypewriterRegular.ttf \
    $NEWROOT/usr/lib/jvm/java-1.6.0-sun-1.6.0.16/jre/lib/fonts/LucidaTypewriterRegular.ttf

    mkfontscale $NEWROOT/usr/share/fonts/java
    mkfontdir $NEWROOT/usr/share/fonts/java

    fontConf=/etc/fonts/fonts.conf
    mv -f $fontConf ${fontConf}.bak00 2> /dev/null
    CreateFontConf $NEWROOT > $fontConf
    fc-cache
    mkdir -p $FONTCONFDIR
    mv $fontConf $FONTCONFDIR
    mv ${fontConf}.bak00 $fontConf 2> /dev/null

    [ ! -e $NEWROOT/usr/share/fonts/java/LucidaBrightDemiBold.ttf ] && ualta LucidaBrightDemiBold.ttf
else
    exit -1
fi

if rpmi java-1.6.0-sun-plugin-1.6.0.16-1mdv2010.0.x86_64.rpm
then
    ualti $NEWROOT/usr/lib64/mozilla/plugins/libjavaplugin.so libjavaplugin.so.x86_64 \
    $NEWROOT/usr/lib/jvm/java-1.6.0-sun-1.6.0.16/jre/lib/amd64/libnpjp2.so 1600
    [ ! -e $NEWROOT/usr/lib64/mozilla/plugins/libjavaplugin.so ] && ualta libjavaplugin.so.x86_64
else
    exit -1
fi

! rpmi openoffice.org-core-3.1.1-2mdv2010.0.x86_64.rpm && exit -1

if rpmi openoffice.org-common-3.1.1-2mdv2010.0.x86_64.rpm
then
    ualtr ooffice /usr/bin/ooffice2.1 || :
    ualtr ooffice /usr/bin/ooffice2.3 || :

    ualti $NEWROOT/usr/lib64/ooo/program/bootstraprc oobr_bootstraprc $NEWROOT/usr/lib64/ooo/program/bootstraprc.ooo 1 \
     --slave $NEWROOT/usr/lib64/ooo/program/versionrc oobr_versionrc $NEWROOT/usr/lib64/ooo/program/versionrc.ooo \
     --slave $NEWROOT/usr/lib64/ooo/basis3.1/share/registry/data/org/openoffice/Setup.xcu oobr_Setup.xcu \
             $NEWROOT/usr/lib64/ooo/basis3.1/share/registry/data/org/openoffice/Setup.xcu.ooo
    ualta oobr_bootstraprc

    if [ -f $NEWROOT/ooobuildtime.log ]
    then
        mkdir -p $NEWROOT/tmp/ooo.tmp.mdv.rc2
        mv $NEWROOT/ooobuildtime.log $NEWROOT/tmp/ooo.tmp.mdv.rc2
        rm -rf $NEWROOT/tmp/ooo.tmp.mdv.rc2
    fi
else
    exit -1
fi

! rpmi openoffice.org-java-common-3.1.1-2mdv2010.0.x86_64.rpm && exit -1
! rpmi openoffice.org-base-3.1.1-2mdv2010.0.x86_64.rpm && exit -1

if rpmi openoffice.org-writer2xhtml-0.5.0.2-5mdv2010.0.x86_64.rpm
then
    unopkgas --force --link $NEWROOT/usr/share/openoffice.org/extensions/writer2xhtml.oxt || :
else
    exit -1
fi

! rpmi openoffice.org-openclipart-3.1.1-2mdv2010.0.x86_64.rpm && exit -1
! rpmi openoffice.org-style-crystal-3.1.1-2mdv2010.0.x86_64.rpm && exit -1
! rpmi openoffice.org-draw-3.1.1-2mdv2010.0.x86_64.rpm && exit -1

if rpmi openoffice.org-pdfimport-3.1.1-2mdv2010.0.x86_64.rpm
then
    unopkgas $NEWROOT/usr/lib64/ooo/pdfimport.oxt 2> /dev/null
    unopkgls &> /dev/null 
else
    exit -1
fi

! rpmi openoffice.org-impress-3.1.1-2mdv2010.0.x86_64.rpm && exit -1

if rpmi openoffice.org-presenter-screen-3.1.1-2mdv2010.0.x86_64.rpm 
then
    unopkgas $NEWROOT/usr/lib64/ooo/presenter-screen.oxt 2> /dev/null
    unopkgls &> /dev/null
else
    exit -1
fi

if rpmi openoffice.org-presentation-minimizer-3.1.1-2mdv2010.0.x86_64.rpm
then
    unopkgas $NEWROOT/usr/lib64/ooo/sun-presentation-minimizer.oxt 2> /dev/null
    unopkgls &> /dev/null
else
    exit -1
fi

! rpmi openoffice.org-filter-binfilter-3.1.1-2mdv2010.0.x86_64.rpm && exit -1
! rpmi openoffice.org-calc-3.1.1-2mdv2010.0.x86_64.rpm && exit -1
! rpmi openoffice.org-writer-3.1.1-2mdv2010.0.x86_64.rpm && exit -1

if rpmi openoffice.org-wiki-publisher-3.1.1-2mdv2010.0.x86_64.rpm
then
    unopkgas $NEWROOT/usr/lib64/ooo/wiki-publisher.oxt 2> /dev/null
    unopkgls &> /dev/null
else
    exit -1
fi

if rpmi openoffice.org-writer2latex-0.5.0.2-5mdv2010.0.x86_64.rpm
then
    unopkgas --force --link $NEWROOT/usr/share/openoffice.org/extensions/writer2latex.oxt || :
else
    exit -1
fi
