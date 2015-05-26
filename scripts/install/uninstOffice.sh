rpme openoffice.org-math

unopkgrs org.openoffice.da.writer2latex.oxt || :
rpme openoffice.org-writer2latex
# clear disk cache
unopkgls > /dev/null 2>&1 || :

idextension=$(unopkgls 2> /dev/null | sed -ne 's/^Identifier: \(com.sun.wiki-publisher\)/\1/p');
if [ "z$idextension" != "z" ]; then
    unopkgrs $idextension 2> /dev/null
    unopkgls &> /dev/null
fi
rpme openoffice.org-wiki-publisher

rpme openoffice.org-writer
rpme openoffice.org-calc
rpme openoffice.org-filter-binfilter

idextension=$(unopkgls 2> /dev/null | sed -ne 's/^Identifier: \(com.sun.star.PresentationMinimizer-linux.*\)/\1/p');
if [ "z$idpdfimport" != "z" ]; then
    unopkgrs $idextension 2> /dev/null
    unopkgls &> /dev/null
fi
rpme openoffice.org-presentation-minimizer

idextension=$(unopkgls 2> /dev/null | sed -ne 's/^Identifier: \(com.sun.PresenterScreen-linux.*\)/\1/p');
if [ "z$idextension" != "z" ]; then
    unopkgrs $idextension 2> /dev/null
    unopkgls &> /dev/null
fi
rpme openoffice.org-presenter-screen

rpme openoffice.org-impress

idpdfimport=$(unopkgls 2> /dev/null | sed -ne 's/^Identifier: \(com.sun.star.PDFImport-linux.*\)/\1/p');
if [ "z$idpdfimport" != "z" ]; then
    unopkgrs $idpdfimport 2> /dev/null
    #clean footprint cache
    unopkgls &> /dev/null
fi
rpme openoffice.org-pdfimport

rpme openoffice.org-draw
rpme openoffice.org-style-crystal
rpme openoffice.org-openclipart

unopkgrs org.openoffice.da.writer2xhtml.oxt || :
rpme openoffice.org-writer2xhtml

rpme openoffice.org-base
rpme openoffice.org-java-common

rpme openoffice.org-common
# BrOffice support %postun common
 if [ ! -e "$NEWROOT/usr/lib64/ooo/program/bootstraprc.ooo" ]; then
       ualtr oobr_bootstraprc $NEWROOT/usr/lib64/ooo/program/bootstraprc.ooo
 fi
# End of BrOffice support %postun common
rpme openoffice.org-core

rpme java-1.6.0-sun-plugin
if ! [ -e "$NEWROOT/usr/lib/jvm/java-1.6.0-sun-1.6.0.16/jre/lib/amd64/libnpjp2.so" ]; then
    ualtr libjavaplugin.so.x86_64 $NEWROOT/usr/lib/jvm/java-1.6.0-sun-1.6.0.16/jre/lib/amd64/libnpjp2.so
fi

rpme java-1.6.0-sun-fonts
if ! [ -e $NEWROOT/usr/lib/jvm/java-1.6.0-sun-1.6.0.16/jre/lib/fonts/LucidaBrightDemiBold.ttf ]; then
    ualtr LucidaBrightDemiBold.ttf $NEWROOT/usr/lib/jvm/java-1.6.0-sun-1.6.0.16/jre/lib/fonts/LucidaBrightDemiBold.ttf
    fontConf=/etc/fonts/fonts.conf
    mv -f $fontConf ${fontConf}.bak00 2> /dev/null
    cp -f ${NEWROOT}${fontConf} $fontConf 2> /dev/null
    fc-cache
    mv ${fontConf}.bak00 $fontConf 2> /dev/null
fi

if [ -d $NEWROOT/usr/share/fonts/java ]; then
    mkfontscale $NEWROOT/usr/share/fonts/java
    mkfontdir $NEWROOT/usr/share/fonts/java
fi

rpme java-1.6.0-sun
if ! [ -e "$NEWROOT/usr/lib/jvm/jre-1.6.0-sun/bin/java" ]; then
    ualtr java $NEWROOT/usr/lib/jvm/jre-1.6.0-sun/bin/java
    ualtr jce_1.6.0_sun_local_policy $NEWROOT/usr/lib/jvm-private/java-1.6.0-sun/jce/vanilla/local_policy.jar
    ualtr jre_sun $NEWROOT/usr/lib/jvm/jre-1.6.0-sun
    ualtr jre_1.6.0 $NEWROOT/usr/lib/jvm/jre-1.6.0-sun
fi

rpme java-1.6.0-sun-jdbc
rpme java-1.6.0-sun-alsa
rpme clipart-openclipart
rpme hsqldb

rpme jakarta-commons-httpclient
if [ -x $NEWROOT/usr/bin/rebuild-gcj-db ]; then $NEWROOT/usr/bin/rebuild-gcj-db || true ; fi

rpme jakarta-commons-logging
rpme sane-backends
rpme sane-backends-iscan
rpme lib64sane1
rpme lib64redland0
rpme lib64wpg-0.1_1
rpme lib64wps0.1_1
rpme lib64wpd0.8_8
rpme lib64ieee1284_3
rpme lib64rasqal1
rpme lib64raptor1

rpme jakarta-commons-lang
echo -e "<dependencies>\n" > $NEWROOT/etc/maven/maven2-depmap.xml
if [ -d $NEWROOT/etc/maven/fragments ] && [ -n "`find $NEWROOT/etc/maven/fragments -type f`" ]; then
    cat $NEWROOT/etc/maven/fragments/* >> $NEWROOT/etc/maven/maven2-depmap.xml
fi
echo -e "</dependencies>\n" >> $NEWROOT/etc/maven/maven2-depmap.xml

rpme jakarta-commons-codec
if [ -x $NEWROOT/usr/bin/rebuild-gcj-db ]; then $NEWROOT/usr/bin/rebuild-gcj-db || true ; fi

rpme jpackage-utils

rpme tomcat5-servlet-2.4-api
ualtr servlet $NEWROOT/usr/share/java/tomcat5-servlet-2.4-api.jar
ualtr servlet_2_4_api $NEWROOT/usr/share/java/tomcat5-servlet-2.4-api.jar
