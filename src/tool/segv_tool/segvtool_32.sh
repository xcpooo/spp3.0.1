#!/bin/sh
# This script was generated using Makeself 2.1.2
CRCsum="2913781188"
MD5="df4788fe488d911756054a6c0e56ef04"
TMPROOT=${TMPDIR:=/tmp}

label="SegvTool"
script="./segvtool.sh"
scriptargs="`pwd`"
targetdir="segvtool"
filesizes="20816"
keep=n

print_cmd_arg=""
if type printf > /dev/null; then
    print_cmd="printf"
elif test -x /usr/ucb/echo; then
    print_cmd="/usr/ucb/echo"
else
    print_cmd="echo"
fi

MS_Printf()
{
    $print_cmd $print_cmd_arg "$1"
}

MS_Progress()
{
    while read a; do
	MS_Printf .
    done
}

MS_dd()
{
    if [ "$(which tdds 2>/dev/null)" ]
    then 
        DD_BIN="tdds"
    elif [ "$(which dd 2>/dev/null)" ] 
    then
        DD_BIN="dd"
    else
        echo "ERROR: can not find dd or tdds,exit...."
        exit 1
    fi

    blocks=`expr $3 / 1024`
    bytes=`expr $3 % 1024`
    $DD_BIN if="$1" ibs=$2 skip=1 obs=1024 conv=sync 2> /dev/null | \
    { test $blocks -gt 0 && $DD_BIN ibs=1024 obs=1024 count=$blocks ; \
      test $bytes  -gt 0 && $DD_BIN ibs=1 obs=1024 count=$bytes ; } 2> /dev/null
}

MS_Help()
{
    cat << EOH >&2
Makeself version 2.1.2
 1) Getting help or info about $0 :
  $0 --help   Print this message
  $0 --info   Print embedded info : title, default target directory, embedded script ...
  $0 --lsm    Print embedded lsm entry (or no LSM)
  $0 --list   Print the list of files in the archive
  $0 --check  Checks integrity of the archive
 
 2) Running $0 :
  $0 [options] [--] [additional arguments to embedded script]
  with following options (in that order)
  --confirm             Ask before running embedded script
  --keep                Do not erase target directory after running
			the embedded script
  --nox11               Do not spawn an xterm
  --nochown             Do not give the extracted files to the current user
  --target NewDirectory Extract in NewDirectory
  --                    Following arguments will be passed to the embedded script
EOH
}

MS_Check()
{
    OLD_PATH=$PATH
    PATH=${GUESS_MD5_PATH:-"$OLD_PATH:/bin:/usr/bin:/sbin:/usr/local/ssl/bin:/usr/local/bin:/opt/openssl/bin"}
    MD5_PATH=`which md5sum 2>/dev/null || type md5sum 2>/dev/null`
    MD5_PATH=${MD5_PATH:-`which md5 2>/dev/null || type md5 2>/dev/null`}
    PATH=$OLD_PATH
    MS_Printf "Verifying archive integrity..."
    offset=`head -n 361 "$1" | wc -c | tr -d " "`
    verb=$2
    i=1
    for s in $filesizes
    do
	crc=`echo $CRCsum | cut -d" " -f$i`
	if test -x "$MD5_PATH"; then
	    md5=`echo $MD5 | cut -d" " -f$i`
	    if test $md5 = "00000000000000000000000000000000"; then
		test x$verb = xy && echo " $1 does not contain an embedded MD5 checksum." >&2
	    else
		md5sum=`MS_dd "$1" $offset $s | "$MD5_PATH" | cut -b-32`;
		if test "$md5sum" != "$md5"; then
		    echo "Error in MD5 checksums: $md5sum is different from $md5" >&2
		    exit 2
		else
		    test x$verb = xy && MS_Printf " MD5 checksums are OK." >&2
		fi
		crc="0000000000"; verb=n
	    fi
	fi
	if test $crc = "0000000000"; then
	    test x$verb = xy && echo " $1 does not contain a CRC checksum." >&2
	else
	    sum1=`MS_dd "$1" $offset $s | cksum | awk '{print $1}'`
	    if test "$sum1" = "$crc"; then
		test x$verb = xy && MS_Printf " CRC checksums are OK." >&2
	    else
		echo "Error in checksums: $sum1 is different from $crc"
		exit 2;
	    fi
	fi
	i=`expr $i + 1`
	offset=`expr $offset + $s`
    done
    echo " All good."
}

UnTAR()
{
    if [ "$(which tar 2>/dev/null)" ]
    then 
        TAR_BIN="tar"
    elif [ "$(which ttars 2>/dev/null)" ] 
    then
        TAR_BIN="ttars"
    else
        echo "ERROR: can not find tar or ttars,exit...."
        exit 1
    fi
    $TAR_BIN $1vf - 2>&1 || { echo Extraction failed. > /dev/tty; kill -15 $$; }
}

finish=true
xterm_loop=
nox11=n
copy=none
ownership=

while true
do
    case "$1" in
    -h | --help)
	MS_Help
	exit 0
	;;
    --info)
	echo Identification: "$label"
	echo Target directory: "$targetdir"
	echo Uncompressed size: 72 KB
	echo Compression: gzip
	echo Date of packaging: Thu Mar 20 09:39:32 CST 2014
	echo Built with Makeself version 2.1.2 on linux
	if test x$script != x; then
	    echo Script run after extraction:
	    echo "    " $script $scriptargs
	fi
	if test x"" = xcopy; then
		echo "Archive will copy itself to a temporary location"
	fi
	if test x"n" = xy; then
	    echo "directory $targetdir is permanent"
	else
	    echo "$targetdir will be removed after extraction"
	fi
	exit 0
	;;
    --dumpconf)
	echo LABEL=\"$label\"
	echo SCRIPT=\"$script\"
	echo SCRIPTARGS=\"$scriptargs\"
	echo archdirname=\"segvtool\"
	echo KEEP=n
	echo COMPRESS=gzip
	echo filesizes=\"$filesizes\"
	echo CRCsum=\"$CRCsum\"
	echo MD5sum=\"$MD5\"
	echo OLDUSIZE=72
	echo OLDSKIP=362
	exit 0
	;;
    --lsm)
cat << EOLSM
No LSM.
EOLSM
	exit 0
	;;
    --list)
	echo Target directory: $targetdir
	offset=`head -n 361 "$0" | wc -c | tr -d " "`
	for s in $filesizes
	do
	    MS_dd "$0" $offset $s | eval "gzip -cd" | UnTAR t
	    offset=`expr $offset + $s`
	done
	exit 0
	;;
    --check)
	MS_Check "$0" y
	exit 0
	;;
    --confirm)
	verbose=y
	shift
	;;
    --keep)
	keep=y
	shift
	;;
    --target)
	keep=y
	targetdir=${2:-.}
	shift 2
	;;
    --nox11)
	nox11=y
	shift
	;;
    --nochown)
	ownership=n
	shift
	;;
    --xwin)
	finish="echo Press Return to close this window...; read junk"
	xterm_loop=1
	shift
	;;
    --phase2)
	copy=phase2
	shift
	;;
    --)
	shift
	break ;;
    -*)
	echo Unrecognized flag : "$1" >&2
	MS_Help
	exit 1
	;;
    *)
	break ;;
    esac
done

case "$copy" in
copy)
    SCRIPT_COPY="$TMPROOT/makeself$$"
    echo "Copying to a temporary location..." >&2
    cp "$0" "$SCRIPT_COPY"
    chmod +x "$SCRIPT_COPY"
    cd "$TMPROOT"
    exec "$SCRIPT_COPY" --phase2
    ;;
phase2)
    finish="$finish ; rm -f $0"
    ;;
esac

if test "$nox11" = "n"; then
    if tty -s; then                 # Do we have a terminal?
	:
    else
        if test x"$DISPLAY" != x -a x"$xterm_loop" = x; then  # No, but do we have X?
            if xset q > /dev/null 2>&1; then # Check for valid DISPLAY variable
                GUESS_XTERMS="xterm rxvt dtterm eterm Eterm kvt konsole aterm"
                for a in $GUESS_XTERMS; do
                    if type $a >/dev/null 2>&1; then
                        XTERM=$a
                        break
                    fi
                done
                chmod a+x $0 || echo Please add execution rights on $0
                if test `echo "$0" | cut -c1` = "/"; then # Spawn a terminal!
                    exec $XTERM -title "$label" -e "$0" --xwin "$@"
                else
                    exec $XTERM -title "$label" -e "./$0" --xwin "$@"
                fi
            fi
        fi
    fi
fi

if test "$targetdir" = "."; then
    tmpdir="."
else
    if test "$keep" = y; then
	echo "Creating directory $targetdir" >&2
	tmpdir="$targetdir"
    else
	tmpdir="$TMPROOT/selfgz$$"
    fi
    mkdir $tmpdir || {
	echo 'Cannot create target directory' $tmpdir >&2
	echo 'You should try option --target OtherDirectory' >&2
	eval $finish
	exit 1
    }
fi

location="`pwd`"
if test x$SETUP_NOCHECK != x1; then
    MS_Check "$0"
fi
offset=`head -n 361 "$0" | wc -c | tr -d " "`

if test x"$verbose" = xy; then
	MS_Printf "About to extract 72 KB in $tmpdir ... Proceed ? [Y/n] "
	read yn
	if test x"$yn" = xn; then
		eval $finish; exit 1
	fi
fi

MS_Printf "Uncompressing $label"
res=3
if test "$keep" = n; then
    trap 'echo Signal caught, cleaning up >&2; cd $TMPROOT; /bin/rm -rf $tmpdir; eval $finish; exit 15' 1 2 3 15
fi

for s in $filesizes
do
    if MS_dd "$0" $offset $s | eval "gzip -cd" | ( cd "$tmpdir"; UnTAR x ) | MS_Progress; then
	if test x"$ownership" != x; then
	    (PATH=/usr/xpg4/bin:$PATH; cd "$tmpdir"; chown -R `id -u` .;  chgrp -R `id -g` .)
	fi
    else
	echo
	echo "Unable to decompress $0" >&2
	eval $finish; exit 1
    fi
    offset=`expr $offset + $s`
done
echo

cd "$tmpdir"
res=0
if test x"$script" != x; then
    if test x"$verbose" = xy; then
	MS_Printf "OK to execute: $script $scriptargs $* ? [Y/n] "
	read yn
	if test x"$yn" = x -o x"$yn" = xy -o x"$yn" = xY; then
	    $script $scriptargs $*; res=$?;
	fi
    else
	$script $scriptargs $*; res=$?
    fi
    if test $res -ne 0; then
	test x"$verbose" = xy && echo "The program '$script' returned an error code ($res)" >&2
    fi
fi
if test "$keep" = n; then
    cd $TMPROOT
    /bin/rm -rf $tmpdir
fi
eval $finish; exit $res
‹ ÔF*Sì}y|U¶puRIlì%jTˆiJhXšEÉÈÖ¸@X†e:#›¤:Á‘`b§õÊž‰£2:®ŽoÆe EV…„0éCD‚cÐ ÕtÔLˆlýÎ9÷V§ºÓ@â¼ïûãû}ñ×Üºuï=ë½çž{êT9Çµ`ñŒs,Zò˜ðê/þî4KÛwÞAu[:+Óliéw¤¥¶ÓÒod”–&¤ÙÚn$$§	ÿþ\yÒ¬%ÉÉ—ìçœ5Î,ÓBáÿ©¿'ìãF†P=Fˆ°6ó¿Dc:”)v?]Hº}…$¡·OuøAø%Ã üÅÁ=~±ð›õÉÅ¢= Þƒ·øþ`,þ&BßB6^°ðö"ü'Îˆ¿ÚAØ Hãy;T+´[¡ÕPÇ_<Ç?' qnü¡r“umø[	í+¡‰POÔµ?*ÍñœhÄŸñéÚ'@{4Yjøo›ŸûëÛæÏ¹u~îB×Òy‹d÷-œ÷Ñ¿rpY·Ñd7É5ÉŽ?l7ºn„òFÿ:.Gí¯7/¯F=Á„%tƒßþ]Ãy×þÏõüúZNþ]®ëcæe"/»Â/‰ë5~=;8·—hïšsì¯;/M\.WEô¿~WÀ¯ü®ä:Á¿>[Þ¼|•ÃÏåõOxýw¼žÂë×q¢'óúbÞþ®Õ{òö;yûïyûB^ß¿†˜8£H´^-˜x;o_Ãû?Ãá¹y=™·¿Ãëcy}
¯¯äýïäõq¼}<¯ßÅÛWòú|Þ~ZãŸ·?Îë¼ý×¼~3¯Wòzïÿ¤n®a}?¯?Êëx=“×ûq~¿æõ±òyIÐì	«Ñ¾×GðúQ^ÈëœÛ?‹\¾fa[¼z^œÓ¯Ÿƒ(ãçà×¨ÓÏ4ÞVÊÇ÷Œè/Ì¸7ÆÄ¹¿ÉÍ“æ.1V^ÞÜ<aÆŒßÀF0wiÆ–ûl\æwy/É](ÍƒyÒiÑüP›»dá¬ùBÞÜ¿™+	æ-Z"	óà2OÈs.˜%	°ùÎ^ü˜°xî’%‹–‹]Ð0oî¢yÂ<j¶”»`.ö#á¿dÑb)wá!/oö¬…ó„Ü…s¥³$Ø™ šbôÊã æÍž¿( Hs ‚˜-Íæ,É•æ"U‹ó-ž»P˜1ö~ zNîÂ®¼¹s€ä‹3¹`VîBaÁâ%‹¤¹³òlçÜ93Ë;»Í^:ÐÏ]š+	£Ç>bÆ@0xÚ•­íjÀí¡ë4Zõ1´ÛÄ\ô?~qwT
ü_Ô±HëW³eÝss»¡¥``÷® º(æõ¼>¹]ÐšçuµÇ¿â“¥ö”ø¶ŽqÂùF³î'@Ÿ“XêV,Á(žÅ’67(ÁÀŠX‚3b	Ó„%`´`	ÆÔŠ%³D,ÁØ%aÙ÷P(Á§`	F¸/–@`*–`øÒ°„IšŽ%lƒ±#8K˜¸YX‚a‰%Ï1X‚‡%lã±ì…{3”° Äû4,a£˜‰%Ó9X&ã
%l&ó±y8ä£î&£š¬«¯ã?0u„šJ!8H	ol¢ýZ‚7¢¤œxéoÂß(1'¶ùk©Ž’s¢èýåTG	:q»ñ¯£:JÒ‰ËÏÿÕQ¢Î¾X/£:JÖ‰“È_Du”°s0ÖS%íÌÂúLª£Äc°>žê(y'šNÕQÎ±žFuÔ„s&Ö“©Žq"C~ÕQ3NÜüÕQCÎ¥Xo>uÔ”³ˆø§:jÌ¹’ø§:jÎYFüS5è|žø§:jÒùñOuÔ¨óoÄ?ÕQ³ÎuÄ?ÕQÃÎÍÄ?ÕQÓÎrâŸê¨qg5ñOuÔ¼³–ø§:Î g=ñOuœ	ÎâŸê8#œ*ñOuœÎfâŸê8Cœ­Äÿ¹`P¿ÿÚ¾Ÿ.q76Ÿ<Ñ)´‚?f&LqZÑ¨VÂà“ee0¯&¹›D„/L-®œœ/Ê3g®§NŠS_‹§ÖT–Ñ›ƒECUX“‚d
$¹ËÅ5+}°n5ŠÅSç
TÅa»±þ«-0Á<åRw”‘ ]|b
ÞÖj€ùŸw\Šä=K#»3âäìf9ç¤ìj•Ï*‰>;í1>ûaò•<I°|#SŒ€]Iž–"dt²ÔU 	wkPÊu·:¥Oôr·ÞãÍ¤áîÖ9®W£MQD·l¯ìt·æ¹>l»s8°N‰(F82ETßHPG÷KEyÃõrhSœ¥k4‚ª
}ûaèoTìõp7U½GÃEš˜e€û=ºtOVoº¥ W™K‚=SÙÞäµ7ÉbŠº˜áúomtÔ½£èn¯E´× ªR<ÂVŠ{)´mÖ†œ;‹íÉ¡öDÖþ©ÖþåYFÐ	¸´"ÖßH Çg.¹Å@ÃLJi_‘ä*¢QF@ÞÒ4	½çCïLj—+ÍžƒÐ^è*IQ× dP—À4áü0„‡D hdŸRšÑk„(–¥"—0 õî’Ãz$	Èuv³­\É9©¸Z•Â³rSŽwÖÙÊp]6œ!ù".5ý,éª—æÍ¬M¥Ú“šëõÍaÍè«-g´æ¸›5Óôûœ]?ƒ{ë4]ã¥î€ûºù³\É¶NRrLÞaÁ¡çýSNçÇÙÊ§æTæ:,åÎ¥bÆþ%ÍÞ'éª"ï[¹º_­¼S±ÃD-»Öå™±yû+ºxGÆŠ™FWU ê—Ãó”/ýÀÕµó|e[9@t˜ŠOï¸¹‹PÐúV|bÄœJ¯t“ k?1ÝPÐÍ+NTÆŠÑ;Ò]Ê[Ñ@ËG§(.PeTb½b–2ÎmUF3ª¤<,*ÌOzA4tÇ¼âÏÚeÞle„¸Õý¯Þ#¼+Ÿ(¹V«O}-WWë‘[WÕ~µ^kVÿ‘ÏX½¦x¹šú:í­}*N÷è·“wÚ/ï^ÚˆÊ”ÜV)n+?8U®Äo¸	íÐcŠ’úIÍrKñNø(x÷|FUA]ÆOïž3—>ãU`?2~ÚŠ–Ñ\úüõÀzÈûºðB4~‹:Ïï=ÃÍ!~»çÖeAù-º ¿Ý‘_êÄùýã%ø-jã‡ýü¢•6™7ÙMrRŠ’m‘¥QÉI’—Áu%Ïs·Æ(…)æMŽ4sI1†;úÌs«sI7X€›‘8Fáèëµ²FÄšKVAW¸]‡Šö­„‚‚yÅ§ÕXÀ_° wø^0É3žº+uGŒàšªd÷õ–Ü•×²A±§zÊÍ%ý>Ü;°/FP©rŒ­<ð\Î×âŸö‰BÁ5 ¯Z(¸2ws,Ë|ªWSFÁ\²PoÆMÕ?þÙŒÌû@ÞÛÅ?ßŸ+Œ‚ñ'HÏÜý{»pÇí7	®[ˆž“áèÉNõÔ˜K^D‰•L9hÒèñÇ øs¾Ðÿ¼¢$áºO,Èµ6½Ö•Š=‘ Ç¤vï*¸úà˜t¸À…©ž 4JÉNô–t=·4¸o´Í8	Éw¡!½Nk0Ä•êi‘z@Ý:´G¨ÿï yçI¿¤OèÓ{. ‡mÕ	;¯¦M«4ô‰«Q‹ÒCLƒ:{Ëô'¥’îŽšPyæÒµ¨m”¤Ü¨…»¨‚ü$å@9ÔQ¤æÒ+°
$ð6ÜCq =ùË‰—À¹vªÊ›†ß’YŠÊr}¨Lw<2ò²OÝ;r­ìh>õüIE {î¡¥€¡_N³boÎ0H«|Ô¦ÿ0Œ•\å®2È“‹2Œ ¦ó/ùQÑß¯MßŠÝPb™7i0âv£2Ìž9ÈûâöGº}>0„ÿ[y…ßœ[³¨ï—MøÌ%ÐÕGÃäh¢¡Ò]	4¬0Êõ§¾tãöÈ3Dÿ_™>€mœ^ÞôDb'˜Ùó*v’¸cÐŠ³]t¬ˆn¹5ãA˜5¬¤·Ð_¡a„z„Ž}·Æ¾nÜ>9Oôßc /.3oRàÍpãB3{#Þ)âŽ¯~õIâ…±ruyBnM5è›#7—Ïß;Ø1QÕÆ÷A9¹Ô6ŽS Þ#OýC¯g#Ù—ÍäbÄµ‘?€Vîº˜üi	®‹ÍO"Û°NÌïµ	ëâò»Â…ÜãêrwyBÈðiöj3³Ï`´Ð`åÉRšÈn=ƒ.;„|s„eòÔ-OµÕdnÛ…°ÞÃ]=×•=R1Ç[’ˆã2–ŠÞÅ¢t¹»Â±Ûõ›Ý[Á$zj$£2Ê¨8,rŒâ°ÚÊÁ­þ‹2
æ••Ó*Æ©wÑ‡ûé¼âBãÒ¸ü.ŠÝZ¡^ån0ÀÂâl8l5°ßÂ†mÁ™)dìt‰9rueFíÕ5î#1n¿!ÐÌÁ…ìóS&)…`MŠÚ—fÊÕ¶CjCõþVeò
1ã3×5[½c‚{žèãµ^&×bc-o<.uÏølÉïƒ†Œ=ËLµ•OŸ‘S9•”‚WtvP²ào ›=‡¤ŠÁV—+õÇ¥6IÌ¨ÍOêW;ÃJù=|èB[q0lJ š|}…éÛVG»›’èÑ&ÙÞ¬ÜoôÙ›°UÎV•Ñ4ð¯àÙæ¿í³7bH”[”‘¯Zq‚å4fÎ°ä=f^ko6ØU¤­îÔ†W_GoÛ¥š×:šŸ‘ÊaßS‚‚¹Ç’ùŠ½ÉSîÚJM°™™BM®7{ôè©<`ÙüÑÞ½{+¾Ž9õuÅ±˜LGc~†ìj
C¼d…­NÉIq˜À†¢ÆÍÏ•ËP|÷®$¹v*ÊN>8@}#tVMÐÛåÚJfÿwå‹ÞšQÉ6-]d¼“iºäF+A~ÃÅÌabkÞXu×ÆÅÈ
¤ÈŽ A¶×â,œh’MÊX£­ÎgW™†•1Üò0R©¸š|ö¦"UwjÜ 8­‘O[Ñ8dÇáŒ	séx¦[ko2Ø±z=û4jdÂäÉèù×*Ž 7¨ˆê-9€'§ñÔ·†J<þ¡‰±‚'b>Í•:V±« @/µÚ-0èþ‘µš=KPêödè{­‘t«L°´©÷pþmrŽê¿ÍÆl€R“"”:„+ÕV‡JMæJ…q=H©*ôÐ”ÕI è
e¸1Î˜ ™v“ù¨‚vö‹Îê)À0ªäXñä»!6Î¸fþ³þñÚ[á0æÁâ1É=Ëá¯°Þ}qØA©–‡á²œV÷ƒ¼sÍ:¸sWÊÔSµ9×…ÈÔð9L WÉ±pÐˆöµ¸8Ž"9ž¡À’P`¡xç!Š§â"PàùÅ`~Ã‰†ìû7·¬ŒÚåf˜ØOL0ËãÙÄÆ	'dÆ‚ÔÝ»Ä©$œ26?‹›>à{u!˜Šfši@Ï@<·Á9òÅèx[åº×ÅûòÃA:‘WñŽ4³’RÔ_CGXûÄÌ*~oú÷8XêmÞwýYÑx—»†Ëé)ê3MÔ´'gÚ*‘¤‚°”<zG‹ˆünK¿G?«xRJSÅkoÆã.âJGB×NäýªâÓ¯–„‚>°‹ºòÈ?‰§A:)ÐIÙÄ½»Àûl‡ùGÄüØ‚þˆ¶hÁKQ\h=À#³Í“íÇÍ¥§afÂsÜ'˜W4ÁÎ\ü¸ˆþ˜¹´>ò1s„(òŽ1(ËÜÆ­è^áI„Î!ä"í/â ù²‚]™ÛTp¬ò>Èõ‘]’ˆ~¬µ·<A³ÜˆðÜØa(CP7ì–â¥"søÞ€:œ>y8¬ZKæpQKXŸ4nEÏ†aí–[GžÉ~èRÄr×‚-™[Ç€K“·>×GÎ÷z8ž×kˆØx²-J1öÊpí†{8ßÀ.~Ldn©ûd°£àc¥{a—S–· #Qq,V®=u¤âXBn¹&ŸÀŽ]Ä±r¯‚™[û‚c“÷\G^² Ÿã9äzÙ|¬@TŠ±}(¡Zñ"†U|Þ,ƒLw¡™“í¼Ïñèª:Fµ¢!†¢6ÙI`Åäq`ßî 9 CÀÑgšZ²‚ÙÉh±KoìbOçDhí+ß…–ìuöùY’ÖÇVN“®x¹ˆžµ¹ôœ§ñò(1s”(T–•ÝåÙ¾6çÖ‘#êØ[ÄÞŠc¯|%h{{_paó>[ÉÞ¬”¤¡Oúþ;ðœ¤”bóP¿âðÃt×¼)û¸6ƒ]Ï¦Ñ)¦ÙÍVYÔŒ8Æ0Ê¸c´*Ù*™ðhLÂV¬YüTlÏv˜O}åÁ´ÇÂù©H·µd_”xo´À±þ›€"œ÷'æÒ|~­ÆšKa8-;Ø¸
QžÛüÁ  ¡uÆ¶ê
˜¥»ýaWƒ¿6ÄÍnä-÷s\ ŒG£÷YvâÊnˆàl‰9ƒ­Â"8›è×8‹	ã¬QfÇS3>•p~ÃDg‹@¶†©„§N6•êÛO¥*¢?C`*±³h}ÄTªUµ©«M%M¥Ã0•p‘µM%:O·ùÓ}ñÀþ4Åiç¹Ý~7´ŒR·°#¢fç¿ýÆUõ¯ŸÂžï±|®yÇ38üÏÓ}N‡æò‰îûDÚÇ‰ ç@¸Oe±J),µÅ¼ƒQÈ2»:Q‰CPÜFµ¢’¢^¦Ò×¤b!Dõñ'žrÁ3ÀS.n!n†ñèâpÓw„±réíhàýžö}¡ÊâÌC1f•Y¹¶]oÎN€cÂcŽòpKŸ£°œù-ÐTØÜ½ŒÜgõ­emû¯»éA9ö^“ìúŒèÎà¾²ð[¤néÍtßN[ÝwŒ6› \D3ysª´]N£gåwŒžÚ£(£ÀÇ=å¬¾CGOqÓfrX/&²ƒG™ÈÌ+«™¸Ú‰íNFÆœ÷âöå™¤0QÃ÷Þ%Ä›+[±—eTIwj²ä½,gí{¿e´[²83º^@ÿBoŒ Î·¤Ö°	èµïôÙwâsXŸ½JšŸl÷Ùwã³ß*û>¢ì£Æ00‹Ì &°	ñƒmè#˜ôÒ»—SðG(ã²³Õ'±^v¾C}&Fês#ópÅ’^ñÄ÷ªó$¾x)[ñ®^tNÚTJbÃ!LÖ ÜÂïŽbÜ¦•œ~ˆ˜²üFs0^áþ™»ÉÈ<>±x°éškøú×·ñö«"Ú#ù~E;©S_ÔxŽVëjz!].Çë™kñÅ¸AÇ0ì1\}„úRŽGF©EÇèÓ  â2ðNèüô€§H½zo¢'Ø-sân½é…Ö}'a<ýh‹­¼¦R?žL	‡ñÇR…lBŽ¨ëï]—zƒŽ+\¡Õg‰Ä+ä„ð§zÞ¾ÓxCä[²AGä_±ãcA¨µ:Áê|zz{ÖNÞ+M4•Óõ”©¶³•·n¸zæLHÞD§¨Øva}s Ùaú^¦“øq¸úø/Á?¦†ßÖaüºáê™Ó¿ ¿åÑàçÿtGñë†«GÇ¯ÄóyÑ6+Þº¤<Æ…ÑsÝéNÊ‡«Ÿ; ³À¬wYp›(<g‘ü+	f¸a†þ^Ø[l‚ÜÌ‰BlÍ«{JñÏlO‘ºàš¿(Pqª
SP¤Ún¦u¢uý3¼«7
Ë€™maë%c)i@€»ûQ8ª¨A¼‚#o“#¢m!ëhá`ÔªV>úP4zš¾‚qkg_,$2“Î¼Üj3ºŠ«º$³g¥h)r[;c^Jgí”¾Ã,ÃÐ¼Ð®ÐGóÓîþØÃ&÷_´ç¢ïŽ…Ñè«9ÕúzöTGé{åËŽÒ7§M~íÓF_¿ÎÐ7‡ËïdKÈ<¼«×h/™’€¡(™gœØ³Ïµ3Î•¿„­Ç§[:ºuÃÕ©-úýZt„IîÜºpÿ÷Bô¼F¡³ôàpuÏ¿/FÏO‡;DÏ®28#Ãèyôß¥G7\Í£‡ZtôÌê=sÀaô9ÙQztÃÕ·Nêé¡=ûuˆ\5 px=:Ln¸Úû$_x‡4‹Î^¼¶ð®ÕÞ_hou¤ÿöž-AZ|°Ñä÷hoòË¼ëƒ­h½	xúÑë2PtK;"­ÄÂœØ¿^ÊJ,=È‰õ/c²;"ÏaáóÿD'å9ŒæÿñŽÊóÓúÊóþfýæ8Á»nbŸóÁã0hÝpœoQ_á<Ä»úy8ö ÎC¹O`m [X¾ÖEé+òNúFúÃ›ê}¹ÒÓ‡wõôý½þ—Ð·«÷]±Þu¶Hú;CÂÀAß4ëéÃ»zúºü"ú’[@~"õ»´¹ô!4"Œ>¼«§/÷óKÐ¾>ž¿œXÿ°óÄ‘;º>tÃÕ·~Ô»FÔ¢·Ÿ]×†ÂÜ•—pgœÙ»î¶H}Žù±òB8¨rí	|ù<õ"ø?4Äy×hwþû¡3øÿ~è<þ¢ÛÞ£àÓüƒøÿø÷>-%x×MŽœ¯Õßw?Â þ¿¿þhç÷[øùýáïùù=å¥_{×õ‹”Ÿ±3ô#ôIÓt~Nß?{eG¡ÏÙÔú º½é1¾ð¶™ô˜°ýr £ö@7\}1ðÎ×Àèpÿ§³øG“ÿÓ!üì0‹ŠAGÇ;*8ô|0h.¸]nÕô"QÂnë‚Z}Œ›¸þ¡³£NI¼ç|¾_¾>åÓŸ*?Â¸£¦¤ÁüJ;oj&ó€XUöüÖŠÏW±Ïš#W {˜œúãJ7¾VÜ»¬SÃõCÈ8µWûÙ0WMà`»ù¡?O¿}†¤{ûúóôgÏÓ:0j¾_¿iP‹N/ÕRºí>”hÚÇ ã¥‹Ò—Ìò£³ÂÎûÖNÓ§£~¦êéKÃé;û1Ñwé+Û•¾ˆù-nøhŒ(?ÿ¨ßºáj¦>¿abïŒr>ûc›åõº‰!–ôPD²b.HÌ%ìŽ6fs”Æ°°¬/º¿˜­„™BFšZð\±,…«çñ‡2@A”E®£FjÒÆÞ©mÐË„Îûè‘ºN[ÚòÏ0ËÕ½Ó KC[¯¥¡­×ÒÐÖkihëCih5îŠó¦òŒŠó«”UÓ.ßK{x–¸_eöþ,“þ‰·¼#XwÁe¹+%n’wu”àO`¹Ý•y_Êõ,·ÛÄr»?ÊŒÏ«ÁÜîeñbf×vÔA/e´ˆg‘øFnTF–‡›”eTµ˜78OÞÊt(“ \åµæM+ð‘\¦™7®Ø‡o=(×ºË­,í!%yj³F÷ÕòvptxdÝ‹<½åƒîŠäyòˆD@q”é _¿Y9â¸úŠ®îTFåÉÊpŸØ7ÊÃŒÊp“<ÏtÇÏ‡òå£˜ù`åÃ×NT›7ˆžOIýØ©Û ]‘°þ{ð!x6÷.&öÐ”–’¢™Á@#ù))ï;ð±O> `7¶·ŸßôE]ÛÏTéæpú°+C ï&¬@ŸFÝMÑV{6ƒÓU[íulEø˜wwkûÍÆpÿšEÎ(XË}
­†Â²<®ùÃ7|!Ý&÷ÑJ“RÔ¯k¸ §D<åÛÁÑ‚¥/h@{E–î ’O3á¾©$Pü´F“öQä5V3è›hòÅ®L0ýjH¾÷éä;8Zôæû¯	NRôèM¤õž®C€ËÔ{j4‘¯áþD½Qcož¯£Ñ‹]½U_˜^}4nh½‘Ñ¸Ù5íéýCu$½¯Ö!œ·²PiHÖÐ™©ïCÑ>9r‘2#ƒ®O0H7Dºþµ:t<ÇH~›aØãÓxx±½¼£Æzz‰&oìÊäÝ…Ñü°NÞ££E>jà¾U¿KE>¯¢èOàŒÐ7ž+B”?™;M{¶Ågäù|¾8‘_­IÂ\¿ô²\ûc—}r·d¸Gé›Mhú]ª’e’íÊD#ì@ŽÃÊ‹§Î\ò%=n½cÇs7í¸	ÜÅQ¯ÏÛ¬—(os=ËÛl4Øc~©½^)y2,o“2Q µŠi&²5E]r;æmâÛWHm(}S—¸y¯bo !?ÅßêCîÂ27Sæ¦õ™›ŽyäœÿX<	XRŒ\¿F (ZÜ‹¨[µÄÍwJq„²‘]F–té‹+ÒÄüŠ¹çó˜yY‚Ä¢lå)$QU™B}5Z¢ò•ÊÈWx¶pÆtKþã¡4e¦)¿BiÊ…”¦|#|Ãçè»WqÉ<Ìò”·àMèim®×•l«§Fê	øÛ%*–›ÂPx Òše$Ê:®ô€H›¤&¹5ìn›d
›ü÷€dÂòS¢Ç{ò.ÜÝð%-°´p£@Œ–ìß*iÍ÷qò\i½#ÃÀ¿ù’EŒel±Ô\*œ[ÉVÛ“Û›•Ú2ûC‡â_Y/Ñp.x>þu¸£þ¸n¸úÖaÍgñ?£¼_ÕÿkìÀÃ?«ÃøuÃÕþÁoÞèàØ’¢Ó1ÒÕE§c¥EÇYÊjÃ·íxð~AŸÓ2æ\òvÒ¶èòî' {àãm?aQ­ÏdÑÆý×!}<g•|%vÂÆícò÷¡¯èò;iÞ˜c
ÍÎ¢3àÍ‰•²ŠÎp††Í´íÇA6z.çB
£žý‚¸¸’Jì•[¶Ÿ€ŽO·ÿ„Å>=ýÚˆÝ_èéA¾Š²¡ QO¿¿÷®•Šø&V»ü´vúÇwAçÃõÿEGõ¯®öÿâRú'ûgR
“phõÚ‡FÃÐ¸[ž/þ6xŽ%L«yÂWÉ?Y¦¶ŒF _†©ãn±èþîrPvÇâuOU­xÇÊîX½î¦«7Ãz_¢Fº1tî/x´`\‰ÛérŽE2+öD<ô¹w™È>±h	ø<ËÚ{PïOR‹ÎkXT’Y{VV_Û¡Y‚×Ã#0Ïi uP¨×Ÿó©E¾ÐþŸðhq¾î‹)¡è<%òIÉsVL”R,,{Ñ„ÙuüÈ‹©£yç5Öåzœ§0[CÙ_¡K>ÜÎíd?‡zZ¤õ=}æZŒ¢Òz%a¥pÂ­5p‹5¾°ƒHº¦•Ø»Er³æßAÍ/á6àØ’¼:ÎÇØ‚S¶ã‡‹&mÃä=wS*óWäÝr5æsž¹n;ÕüÕ£ŒÊåßÈ¦³p“ùÁàöX·ßà©Y~™§eybk*‹¿Ã`f)yø¥
õ]°ääõSfW÷®Tþ
†¶lQlìýuÔçLx®;´ÚöòÚ·žrñnŠŒ§ýj;¥¦,~žü“9-I¼L43ÉZ‘(Rß4ÊKxÄõ  îA’Ï&óÊûÈ{±àñÈ\r7Š¹yë[o¾ùæ©¯ánÅ1Q¶ K¶:|{w†îý/ú*½~aˆ3j'4Ì®A\V|þˆñ¶Jfw¹"rp5Mc™›>cb”&±L¾”dÎ,"Afy™hÕ3;-ÅÂ!Ûew: yEkwÃ¹@iHäÙ–O1ñ—Þ^ð´˜KfÙûü¡á`3¿ø æ×£4¿˜¿žFHbØ,¸–q‰~¥úöõz.Ü»ÒÐ×óG[~– ÿ”óø Žð”röÚF2‡ÄËÄk9£¦(é«\›j†qZ©gTÃº÷@³Ó¢0û=,2¿3ÈÌ„žß"Î/×*~ÿDýàºð›Ÿ='àüNÑó+¼Èøå°“xR¬“øÓ[Î­ûº‹sK8ÿõI·Ó£p‹'uÿoƒºxF¿ôñX” ì~.à –|¢÷Á´’m'è@¨÷~¢7ÿÔ¢›¨‹·â1‘™øµZþP8=#ï‡À®£çd]gèÑP·Öéé¡=[¢Ð¯.bÓ¡7ÿ=sê:¯ÖQoªÓïcE‹yÜ–vùizzÆ$@kÂâçöw–uÕ~==Ô¢ÏÞF,—ÉævÓ©ÇXiAªAZL°Ý»&OÕ9¤ÚÖËA›(q~z´Äù6³ìô®Z&üÆ‘½Š·2ÿõ5æ¿¾ ·ÀŽê¦Ç(Ð•¾»óß›³ÏÁšÓªe[7³¬ñiµ7öïæ¿"°1ƒæ
_<Šü<æKêÙÓboS6{££±·u“ÆÞU{Ÿ¿ÆðñÌ…øÐBrã61>ž}_ããaýú&úsÂ¶	ÌB,®¤$A÷öûDé¸h”ZÚSz«††ÉÁSÇøŸ.D®…Ç‰4r_ÞÈÈ=º¡-ø™!o=±ú‰i`¤ÞÔ¹5R{i¤>¹!ŒT@£QûCµœR>ÑˆJOqà}Fñí!Š×†?ß£WŸÍ5aÃ…é}ÿývôÖ½×)z	@Ao&§×ý^4zqÇ1<…]‰§°ßÂ)LšÏÇ-tþ
í»ü4Uô1´0C4†úiÞ#üM.p£Øyí¯ì¼ö
­×ü±ç]‹Rü7l}W›Ë´WåTQ ‰ˆxqæþŒ×?…dDœß"õ£7¼ëß½°~znh§ŸïvJ?Ü›™0C4¿ø£ù›õÑôCç½i«WÆÅál‚¶¿CÃv²3qx—Î‰vn^Á°,¾ß;,A`¦ØàØ³‡ÜÝ’oˆ«÷ËÅa	î*Cñù NšÔÎpŠ_^O2™M&¤ÏÖ ´Î6%>†•ŽH×;JÂ–S%}*ÔøSGðËf÷›¹ô#ü¬“Ü4ör 7³9;5“©Y×»LJ®ÃÇµ!Tg4m¼Þà/àok MÕ»y8“@›âwx€*‰Õx”ªrE©ü{è£cÔDˆ¶áw8Ò_.ãŒèf¥éy2)[‡ooÁ¯É£³”QFyt_E|¹Ã“Y¦yòè»•Q¢WÌòŽ,µÈeŠQ/š7=2OoÄ–r=¾ª}déyº¨Ä+SÄ~µÞçñNDsñ|vXp¥eèµ€ß*eÔ½Oõi?Ìu4ã“gÿ¢óaÃÂxÿ~’’môU_^HQQ¹ÛëñqÆ5ƒÀ=]³J|™ž½ÿmk™„±Ùlü—mOì.WÃ©s¦wZ÷ ×:Õ'>¬Æ¡òÈ§E[]àZX¦âT÷Èb£!cYWQ
ÓÈ½ÓT|¿ "Ý­ˆ]aš}‰îpšöÎcÆRQº»•kÝ®¢OÞ}¨§~D™Ê>’œÐTk÷þ¾»Ÿm	EÔ”åje¢¨Œ›Í¾ËÍÏÚO?sªÍc
àMŸšÞÎßÇ,njÕ%'²¯ç¹ƒóÃOJŽÑsÈ\‚¡~ü D¹ä#zo0¾'Iž×kžàðµÐRãóô²¯ú*t-?KƒP×TwWÀöŽÒqÿÜ—}ø‰…¶Ó½3Ïâ7àŒ†/¼ÖZ%¬ïv%&Ãnr}zK6"ðtXÁ|ü~†Ä¡Q¼£ì+Öö½ùa¥æ}*’}É)žŽÚ&÷‡D·Œv‹loû4åÍ¿ÿÒùþ`êÜ€¯éæ¨JvWÉ‰¢Â‚Ùs€ÞµŠ}ž4.W¦ÿ×!GÙ@’ÀÑ #
€g4 æ’T”1(Tdð\ÛØg¡ðS‘®“;P­>ö}|½@w÷†âw°JòÓ¿ŸËöC.ÛØç°Õ›^KëV¶×ããûa˜Ê»ð¹TÚ‘%½	‹±áž·ñû2´¢ð³OárëY†ÿæe{³ìh
ôŒµ7e$JÃPÇU\Ç’]þB®W
›1Ô3„”l8î?¥yoœÑËµ[NŸæˆqï2òµ‹UÉåÝ;_¾p«íåëðü?Ì— ÷N·¿N‘?›$Wº›fÂlŒ/Tã«Ô8&¿EÉ´hŠÍo³d‚{×Lt`ŒEŸ«uF4âk²I)ô!	4­ÿ~‹íï¼‰»ÈL¤™²·BŸ•0¦íMÞÒ7)uÿÕ(ñh&Xü&’r+ÀõÅáì°ºL×ÔšÊxSî{,©	fó×[êmà³&žÏ^O{IN=ÿÚŠ’}˜+ûÇ¢Â×µê-o¶ÍñzÅ¬o•¾vïJÕ½ßò…É`;än²nE{ÖOotWÆÒ«ãYxøîÿb1 æv±Çê×€n¯õe"Áô¼§ÆµÓ½ËÂÅßN¥`¨‘¬•$±‰«²o?áK5Óar–Š½)Ã5)9øøÖrv£~*oÆ¨¡Ô-­ÜÄäÏÆvI‘QôÑAüƒÐl 	ÿò²¶ç‰ÝP<è`Ýý$´eÙZXƒ§N®–\ÞºnÐØàûW&î_ÙaûWNhÿ2ÑW€´¥¥}ˆô›	ûYRØ~vßÏª!^1w+Àk7)­ÿ´®~üßhíL¾œöo­,(î°˜A^ ‚VÍ_ðÄ°ðòY´ÙÓC†4âùCÒjM\àKàfpŠËsH¤ô$:Gö>~‰þ‡	>ñI’úòîXêÞV,±äf4pñ¯ÇÿO§ÅÕÇWŒW4|EóáºÏˆXu´µxÉÕæÌŒòNë}i‚ô± LiúÐäêµrZ‹ižÕÆÆ™K10­‘lÄgIÓ×õ\_ÿà,9L ?n‡ÚbvÕ=ômh™½œñybúëzèÑffA)[P¡Ûß ó‡ÝE‹Q,ì`“ÇüÛ‹ªtH=Ófh3ðªþù9¬ZÔ¾½iáNÚ šéàÁ.E…2ò_àÄ]J#ÚrAÏ\òÒË…KŠ^+æ™Bš1—à‡içÓh'Ûª1ÒÞÌ%OQÅaaJeEjªk¯µ»¸ÖVéWYECŒa²‘”†rHÙfh¯Íd¦Í2Xë¦0mÂÎó$è¤5šúz›F÷„itZ4úÛÏ˜F§çù3ÿ=vf=2ÍØzäßÏ‹XšÛ­Çƒ¨½…ùOÖãÛ¢¯Ç!ÞÂFÍpuÈèö<Àù}ùþ£~µš¹W¡'qyd?é«-HÝVákQØüÐ¥ÞÇÞ"½w:<ºÆ±q kœ·]Œ‘|	{æ6=¯e9½Ä¯´¬-xÞ& ó …zìß<vÌü>¸­÷ûŠ@šò5ˆ;ð¤~à3ÔõÚÀ!ÑŽ¦H—ŸXµ9l¾¸gáŠrkPï®ý{×ÝT•ç_ÒWˆµ’²n‘îðXÛ•%6X~tŽ”D) %à/,¡IšhšÔä…¶ HM+}ÄˆŽ2º0ž=;®?ØU©²§¡l@––õÌÈxÐIiÕˆ”R›ý~ï½¯yý:Ìî™=¦çÓûî»ßwï÷Þwß÷Þ÷î÷~¿ŠãÐõŽty’7—\Ø”SÇÂÔÍŸòìugó8­"äÈÄÏÎ£aèl¸™{Ÿ§YŒm%¡‘ex­û«QÞ7ÿŽL8;Ä<¾Š²¸OÉÄM?¬¼§Xy¯CÊkþn”òè6ðúhåÉóÛQëÇ¬Ÿ\Þ/hy«F)/tÉ³5ðj÷—#ÖÓÙü¬†V¬‚¹È¦±Dý 0~¤¦4ÕBø“ò­€}šè}ûàòf "m[¾r4›¿â8ø‹ÇáÿyN'¯§Ëï{èŠî$2ŠÕ6¥÷µ5èË­¹õÇ'si¡Õ©ïàQv{°QcúÈïÕxm7c`>ö%*ntvî–¿=¢µÁpA¯úDú{;ŽÈ{šÈ{:‘cšÈ1R9/¡&[°ACOÛ¤hªã%úk4¶®ßz, þúâÔ¬lØü}¬P-?n7QÌÙ@Íæ_AV¶KæÖg™àî(Š€içé÷Â?@š‘š÷a{v½M¯ïÂë§^Ÿ…Bm„¾±˜c¼³Vo$þ¬àUS»å0º8¥­™ˆ÷•ØÞãñuné.ZàL†žÓ>ý!vç·cñÖ
ä¼Ìèm/‘…{G¤Ei!TÛ´äÜqMä8	¤]x}xW?ü‡bho×;"ïk"ïë0~w“ð­é`!iµŽ|€ä3—YqÙßâ²jôgð TÅº°÷cQ‡¥o[š½è …è«ÿZþŽÛAÏ±5×û©ýÃkÓÿƒ¦3U–[Xz÷N9ý—$k}ýÐ.ÚOÊLz[¿„­Ï{æª±-%Ü·P§š@rCßšý|'î¾ÚS!4ò’z–;ƒöA‰ž¤¢³*šTÑ›%ËYiuT
ôÈz…©”Í¥ÐvÑè•aOüJËrq‡\éÎ!&7Êƒéoþ`£Ü?ØÀ[FÓœÃL£ÉªÕWˆˆâkãýA&Ÿ”ÚñTB}~µ'ñ©L@üêÇJ¨õ;©„Ò)$”¶&ciL>#Â)\¡Én7Å®.œtrÙT8a	ß# ìì%¯ëzY8—Ò¤¾!76!›î%²I‡²	·¼1Ãx¦WLwà&ÂÔ[?’)¹¦«F!–>xQæÓ30t^…Â	Òy6_t]Ö¨;‰Ÿâå­}ßa#
YU=˜éDú©›|¿Z	ºh^±Uü¦Ì7;Œ­Lÿéˆ*‰Úô‰t"pAû†YƒKFŒ¯Ä/t)KŽÂ<»¾¶5´1sô/µ¶Mãë$¢Z-ÍçÅk/JjÄ˜Fß—*'p¥ù:1ìµU
Å˜^|3d™Š_i¥jTI_–ÐÐoŒVUæØ<‹ Nñ/„ó^ º‚gõÚšõäêT²ïHª!æ{í]h/]ûÆê˜±aÞêtmÍýÔCWa|¸~\BßšêS/ÔHæžÐB|$î2Æ»'„
ëƒÍ*Ó&ÝºyTqº‡èQ]zë0Qœ6÷ ât8CMþÏ²rt#~þøˆ¾!JÛ7Ò4¸iƒ.äO­H363ŽÖÑþS¿î:îÒˆãRLûFÁs¨¯Š˜úèÂü¸î&ú=~_Å¯«XC‹ŽT²'4^²@Mu¡‡pÎ³ ´1úV¦6þ¸¾{©0zþirfãÈxK¥¡Ê— ª=aþ´%ÎoEÓãKu¡‚§um¸s®EÒjèKãQÍ»äËL1 yq´ö¶è	7ÑAn6!7Aõõî±Ï@ñQU3koª¨¯Ã©™ÀGTÕ‡by…BO}2S6GMóÿ"êÿz)nPB­¼ÇdŽmÊŽ„
Óñ{ÿ,ÙVÐÓÝ$Ù
ÄÐm‹ü=òo”_3á·PCö]0–çëB"Ë¾ËcÇM@hì ¶/½õÏ´Sc7F¦Ã·Î [,hˆúQoµ`aù~]ÅQà/T¨ÇG=Á²¥Çd‰mšˆ
³' ƒH·>#*øm“ù=ÎúçÙ__F¾wm[CÆ0ûëÚà~º-d„v3ÙÜÎSSKë©
KëâƒtsÃÛTES§°ÏØZ¥ç7Ï|V|8œ?€á¦p~B˜
‚Š‘!ï65õ~sÈ@OWiB­F:AMx±Ë>.ñ]”˜ðVšaWd;³MgÙêâCEÎÄ´þó4íÂÀ >_°‡l‹ ß¿S¤\hÛÐµÐºó–éücæ‰Ûô¾4c+NZ’Dz±Fœ€=¥ÊDª4šN¾&'š¤ˆêcÞ«uá•*iîiJš·b‹Þ÷Îà7¶`’ªû•áóã`O.öEà(€®dz;iœÛêI²Àô*&m<È0õJ}Ú';ð|ìÒ7’¥]Þ4îwáSat]ºh—6ž„8±Í<šËÄªöwæh06Y2ŸÖÖ —íÂÓïäCCtšùt(Ð²N2ŸíüåöÂ³Q$¹æHÈ|F²Àëü¢óI8G û3¢'..H–¶K_˜,mÚ'p£¶æYò½¢-»yâ	©°M:†„Roçgxmg³mƒ
tF%óIâ‹Žô5q"Y‰^úšõÒ7]ý…,@¿…™f¤ûc¬ß“9ê›Œ™,=ëÒ „&€,¥Â“]qâpåd2ÍÜ3©0Š+f…gºO™šµAìƒÙ±I–]«ãŠEo®fÿ¢w£#ŠÆVjRgD“ž~‚ÈÕsø?»=Ò«†ÌÅ,c\ú î#‡#Aã×›xÓÆXàÓAQ‘ÙHæEQxÏŽW7ºHþ©¡çzéæuHÍÞÊ•_>~¿×»RXÝøÌ ¯¸½è×ÏÆOÅÛCÁþ8¯	vóp&CãìJZÙ¯\ÿXÔ_}6¿ïVÙÍ)ñi‹þ$-~k©}®å¦Û›U´:\n»0Ý/øEkÉ#IáV¡ËL¡ Ñ+,„ss³üž.B:ÅùŽÑ³S˜uQ ¤Ä ¹Öí®ü(Êp
¯OpÚÝåÎæwr‹ì¢Pd^´R(r–	f´kà PÌ”[îó–Ì	÷X=ÀtJneÖÃœÊé,Lá(/WK­XË-+·{WS³üÙ´Œ”¡ì*Ð;¨@Úƒú(Í
pYî€à>>—ÝcÜ.¿HS8ë4îf±¬üæoÀ#—”Ùno)^ ­k î\<ôÛK¼›ßÂÜš
Ø«„¬+s?´P0©Ø—[Ãa.lSt6:…¸°)…€g¥Õí²A“¢çS;²U°Öj+[Pfƒ†þÁÓþ*¿ËãðróÝký+ªÊíÈHÑ…B™Ë•8§pÌ=*­Ë¸ùPêb[~Àa€„b¿½Ô¿~nV@˜r°G¼é0Ö¡:Y¶¹‚ë.¨‚Íë¹IÖÛ}Þ©¹ÙÄ/4ŸË/d­·ån¯§´ÔîD§Õ#Ìš9ó–YSFäÃYEÑGZkÕÀ^	eå	Þ€(xVÆWÅÝ2,>cXÜ8,Ž¹CíK­nùFyËáD©'qÂ/ú/GN—Z–,ÉamÂ¹<ë°}…ÅËñùHÄ]óÍÅså¦õ÷<`Ÿ>|ø pÐxpu• G‡‡ ïÞì¼ø-à_ /ö^ì ¼ xðKÀ6ÀSè Pxð }é ü€G@À	X‹ÿg<§š”4OöÍí¾ç(|K«ÖßÃ©Ó¨&¥ò<ú`Î`¾°ç Ý¼`þ8M­:ÿºdó“ü–$î§ßO¿¿©_œ)É¡ì·=“ùaÏ©ã5èSþt/õ¹½§š× I¡–× Êö'xbWã^õÏ®SøÐÆÇá,ŒÓj–©žùoG¿î9[xë!Ï|ÍO`>ìaã=ú$K^ä%a%ÄÿÒºÕ¨èBˆžù%ÆßýPŸ‡ë[; ¯ Ž>œôR$^sàç€¹€;÷¬lì ¼88øpÐHÙ
×~˜¸sëÕëªàQùû3Ì¿üðß€ã€V@#àMÀk€Ý€€m€  °à¸ vÀ}€¢™ÿOª¸í€  
p ° p ˜˜ ˆ×ª¸Ë€?¾tþ88¨ü;`;@lT66 \€âÚ„Wö-pÓ« e  °°
°°`, ÌLÜ˜¸€¾°¿ƒNô)v¤EÌ¦.ZjÉòFC®0µÈRd–¸<ÊìŸ’ÿ*É09Bw÷Öµœø»/ç¯h7ÌÏ_<]´–²XQ ÈÌœV˜âlUUEg(õëì>¿Ëë)†4ŸÝtô Ü-b!.ø/Ú+á¿"äÅ9;g°;‹>k™½Øió%bœ¡DôúüP.ñ‘¢­e®(Î+’4ošÏZ?•xËÊìñGË›k™ìDy·ää
•iJ¹Ê197–Ñ9—ßÏÂ)
º: ÃùGú(t8_Ift(Ÿ÷¨ÐW<='Ó!f0ÕLn·ÃÁrÀÆÏä÷\&»ÕLÎÉT¾¯G>7¼H‡ò¹2º“K”«fXÆd:£\?
tè|]Y.þ\Ã®ÁqA?–ÖWY’¥
:GrÆÒŒ’Ù˜%Ó•³üÇ°q,GCÇžáí÷ˆ‚® è
€N§J‡X§ [²||yt:>¦ Ãqót%3/÷	Eá.ÃØPi|‚Nî[tüw0oLÌO•ù½¨ Kºt »uºß0::]…qg3m¯át¿UÐi€Nsºý
:tª®ÛœHSÒ½ÍÚ$‰Éz kaç”÷÷°"¿Ó@wz”r
:œgœº§F¡kSÐm†9Áæjèª‘íÜÁÊG:\qk‡t2i$Ý‡Šg€#ÏÇ‰£<‰6¦¿‚ÈwyðùóÁ‹±Çö×-#~³gÎÄÐ8{ö,7æÑŽfäÍÊãŒ3fäæÝ2Ó837—Ë5sgÍà„ÜÿøE«O~ÎiuÛ¬ö2¯çÿ×üÿqó’…*U¢'ª¡Çbl?Èý<û™PËƒÞ¯á¦r“¸ÈóŠiy›@.`ò—g}>ây0!&°1g}~°4¼VÇ19¯céxî%HàX¶xÃÒÉø ézHC ‘åˆüÌß×Üe#öìò
à˜KÆ^@:{ÿÓîþ\õ‰ó¿ÙíZ{³Û6ÝÓƒßk˜‘;‘w˜'Žò5rÛà$ˆL„V•¢JcïDò«­erH–›×°qÛUù¥åºaãÎ÷ýÆ°¹7LþÈc¦ú*×-bá/ 8½~K~lXÜÂâ,^©1^¦˜+`ü5ƒÅg²ø/añ›X|'‹ïVÑ¸ÅM,}é0úÇY|‹ãxðj˜¾»¦AË>8Œ¥ï®ƒò¾øÎuÅ÷ØK]~˜×.p[ý~»Ÿ+..APŒ’C,.æ ;”`7˜Å•û`þë€tœ{ÝÅd6ì±ºAÊ¸íörüYR^Å•Óï¨~oÉ#v‘syìb±ÕV¿]ÄsÞr‘C©,zñ«è*³{6kW¼xdlsyŠ~»JÁ‚eV—?à¹í®ÄíõÛ¹EKç/(žaÈ%wwô?Õà}WÁ_¦¢/é\®ëðºƒ,~ž³1ØáXh´†pñ¡cöbÜ!vªÝB§ã1„Ž§Á:u*†Ð¡uBgÖc;CèˆÂC `¨“÷«D³¡Èèfü÷oÜˆÂÅgž…’âYÑ—è;u<9s’í‰gðcFrèÄ´®vGNX½®GŽøØwí'qäÜ‰·¼k‰cœS1þ‰cMœØ˜]›IkäœƒñrÇš9oÇøÇ:ïÀørÇš:qÚKü,Ç³°ÆÎ{1žKâXsçŒ$Ž-àÄ
‘u>¹O¿|HúcðllùŠ{œ±§°ÆÐ¹ï^é\óê(n ½0bÝƒ›Èq!tç…K,ÉÑßàN®ZúAšÍ·Ý¥qbjwF°ß—‘xÇ^ˆ2qwS2¦«Zý^4n"&þP©¾™ÏÄsñv9cö/ÉäÝø£ýteéîêžJTˆ«âƒ¦±â!Ükº?Ìvï4 º¦îÃ%£°µ¿ñj\CÔÎk	S–ÕÈkSh‘žèu¢2_ch‘NªýWªUw•,‰A.£­ÒHÛ_E­ºÇy‡´k9¤ÚQcîÖÌèˆ¶ç¤í{ðÌ‘È¹Éa^}¢×T‹qí¶S¸ÆLŽqï ª3OiðÅuæ/µ
ŠUMæ¯¹:óWaó—JVtÃöˆÕA)u5ÓA6¸À¾ó¼ÚNª`>DW¾hp”-D{ÕÄ¬×JäÓÜªÅšhë·ï W¶É*nlÙÕ!Y³%UfxrhSSR\ˆ'Úp’å=ÉÜ}[›g	³µ/ÃY¿G:årôŽwqS—1¶œŽä´h?ôGÚBáíXAtä¶´…±‚L]Ø|ˆ¬’Œ1£“jìg•¾Ðv,4ØŒ¾Å~ƒÚYÛ03óéiæ³!Ë™i–¨«áö|-$ü©Tí1&5JæÚ3re²öf¸k}“Mã}¼)âOÍnA]¯‹Ñ7¯Äã¦mÍ-¨4µˆîlî‚¢¯à¶ú®±I	ã]7`?©ý‘wêÐ9t•ÝrLŸUâLÛ¬Ò>Žö©6™¥ÆSÒé¢¶^ìR¨UÁËªÇcÁ.U U{@]gîÓÖ›ûTÒGÚúÎH¯.xT§jÇ:A€þ­µõ4“îãC×wW&
ÔÖtPèE¹ÐtÓzUðœ:Áû°Às¤@Ux1_„?ÑÖ7A1Xàm}#+õUûCP&úª5žênj/ŠØ¨é•ýÍæL;—,ñ&QÍ„¢}}ÌËéô°ùêkóÌ¯*>IÃeRâ¦Jb¬?™SûŒ¨êúmtà2Ü¤¡	Jýª9C˜ÁüÓXþJ†\	C‡.3†Æ)Ë•.R~<ßÇÛO¹:=™šõÍÉkvÓ1-¼X|6jš“mìª+6';Y¬9Ù=xT>x$U²#”®P­Ç õ]Æsºw±‡ôk!hJvÁÿ´ÐB~æÊç÷anÚú…jÇ>‘å«¥> Æ§‚qQiäûÖÁ‰MºQ:X¥ûãxîz!¸Ô¡­?æ€n<±ášÇ¾ÿiïhzÛ8®vk á¢‰Ý?PŒ×›HlH“KQ”Lf:N4nèARlŠ\IS\š»©ˆ

ÄE/AÑCnEƒÜšcOEŽ´è¥·Þ‚"‡ÒKQäÒ‚¶ïÍÇÎÌrI.eK¦[1 Bî¼™yïÍû˜÷æý6Nõâ¬r³ò5–ê_`¸òá]Æs·)Žø÷EÐ£?RòüÕ•o@SÿtþýOðÂèù?|òæ»_½µñ)•Õ¯ñß >ûéßÿ‰]ðŸXïð×°z‡_þë­w ÷Yï êÀ9ªõ
¿^¯ð2`óÕú7¬^AÔ)ô¢:Ü!nö¶HÐmxüÛñ»!i!|ƒ¿ôÛV×½ÛsÛ}vÆÌö”{WžÙN;ÿùð;ÿyï¡.ÎÞeç?øùÏgg’Ï¾ º¿ÃsqêùOñ;ÿY¼ÇÎ‹ÿ<vO?ÿùòÝÙÏèÙÏ²ß_`Ì¿Áûðþ7¼Ÿ€÷‡ð¾ï«ð¾ï7áÝ†÷;ðþ¼?ø…ìÿ}ë4Ç?/9þÊêëy}šÓH‘×§¹‡y}‘ÿŸ–×G½Ãøç×g&çõQ1w•öóú Oä»Lo'åõQï¾8sJ^õ™’×G}_<Çô|R^íCñ³“òúhŸ><7=¯ÿ1À}ÿ<39¯y¯ßÃ„Ÿor^íá{OÏëpÜÍ85¯ÿ-À}ûxrÞü}5—üË—%åá£æëî±1pjþƒ¶O$Ïû;5_¹¶'’óõjþóŸ'ŒwŽçÿÏ*~á€{!NÍÿ[àê­ólýãø}¦äë1>-žGù;;’¯ÿs,Ïƒpßœ™ž¯_¼ÀÎæ)_<ù0î-ÿ_,VìÒ»dW*••R±lcþNóÿ'ðºt±°éµÁŽñÚµ›/;…^Ð-´üF½UàyUù}•ôû
c«×n„àÑ	­¦]ÌFÆmìøÄ´Šéö©dëL7Lè7ðÂšqhÞY#Ö%’wï’e²Aj$ÜqÛFÁ×»ŽI%<¼i¸-º’mU *pÅÑØòv'ÎíF=$–€"CRïß![~wÑsìš÷ŒsãzÍ{úéìËñ.Z^öðpá¶œÛî Ó%ði;Ü!wÛ¸„ƒZð·º\©BPr¹À´‚Ë&V‰XKÄ*kÙ0Îúîö^èû­ã´ Šþã+öÿrq¹²ú¿d—Êåµv	Ä	ë×÷'î	§µ?Úú¿Y0à–Yç{F£×ÅNhË6pÿîvP¿ÓR·na›iÐ:ûq ´Ñ4š½ÝÎ.Øàî>(R¯&@·Ô-v9€mÖé€x•0u¹ ôÁúKAï X*rä*´' BJWŸ²µ#IíÔ˜E8¼zwônÄR²‹&Q,ç&NÞ­7\²Ö©‡;…¦[oa-úVÖS[) q^õ{«Ú)òd­_.ôW}»²AÖvü ,´Ý>.•
•òÆH'äÀ}¶Ý°sg›^œ(°ùè±žj¡]¤·Txngó	*ÿ^wß€ÒpïHSñî´S•ðWà÷º°^GmÇªZ;>‰ èDÑM¾¶©¤ªÃp<ª£@ÚXš°\bÒq»ü.…„Ä;(bRô€š.D¦ØªƒXb­ÿºy¹ èº©3ùg(	ô2IFÏ.lý²@Ò­7vH×ï“2ÙÜÝ@‚¬Ž€¬ÆAìJ5bWb0ˆ>í¹°æø„¿ÛÄ>•'Q_xŒ]3zsŽô½pïX°1“uY*	¬—JdÓI§ëowë»9ô:Läü.‰˜õ«D©”'ôãF–
/¥^)©áÿ«ÔòÂ	ý\¯Ós 7Ñ–[§Ùí>¶UE‡=¨¿E4`Mã;þ®[ð½Æ]ðëÛ¿—Üð5f*ÒÒLf_B!¶½À¥ý`•Ø;8‡RF¿üêR\`iäè|º8éV°/®·±yF	£u\ ¡ÄÚ†C=úXIÑÙZ¡Ý0É(¾aøƒQ‹Á1ºH÷!Q|¢Íh¥We{Ûñ’\^6ù`5>šB`Ã¯ÆÄªÈîÆ/@9ddv÷û^3ÜqÊø†òêm‡ê®‰‚ý twÐ-Sá&¸ž’Šu£ëó{ø%Ó/ËYü ÉÔj´mu¸ªµ­*mvehW´V»"›D[b^q†dÜ ÞP¹Ì1]Åt‰cŠdwøŒ1ÚÅ´`€†í<3ï±ò(beŽØRi¸ÄçÔV@LY)+±öJ™¶³	8ÿ¨èjÓd"›G-ëLLØ:4;´óõÉ”E¤!UGgÓƒÄý÷H„•úAöýÙ…(t{mØñ]ôº	v¢› ?ÔVñ¢+4HŽµ8vËO†d»ëvˆ©\È5IþšÛ$ù6Y(ux¶!=OXzXöáB¶ÆiÎ¿Æö >ñ¡9J1½:LçzƒZÌkhˆé-^³Fb„(ìÔÂ›1lÔ`ö½Zßßtge"Î="sO=d
1Äaø´¡àäÛ®H>Ø$ß/“ü^7‘º±lÇ,LË®7Z×îí.Zv¶fÙO;¦ç¨¹PãÉ“_–&Oì¿õ¶™ƒarÖrÎ*ç¬¥œUÊâÂíynŸä…íOCZ‚¹"©Déª–U’ÈšVS®/„cƒÁÅh¶E'}õøH·+—íŠ$~„	WrÖjÎZÉY•qkœAf®úäÐÖVl‚0ªhHgfpÓ¯å¨|>bK)å™®æ
eÌ•9_ÊHO\Jˆ¥ŽO/að“XÍ¦èkÎ²á‘ÏlxhÃSÛøúØEÍÀ—‚ã¤7ƒY¨MVô8uqêcæ|Váy0†`e'Y×8mÎÑ%.?Ëœ£+qn>"¢ó ˆ†Þ‰)§aáÜÄQŠlÎÂÀYð:zˆ™«Yƒ@ù“MGŽcÓŽéL3E€ê×8* sÿ©¨¥7¶1fÎSôwbÍ[ìw„äwÜ«õèÅ}aï?ê;ñe|b¾ã]Éy‹øŽSþ¿â½ù‘›ˆöæElŽ/Ö“ÇÂì”DBÐ:Ö~Ç:HÂ5zÌú	¥YÖ2=?®ê˜¬ðÇºãuèÑ­Sœ›¾˜"¦Ã&F)£øóæ“ŠR‡éâ(Ä¢DË¹<ÇÇˆ3.«š•È´`
uÕîùxWE—‚¯´;½P©èwýö6]7ƒ‹4E2âkÁ+À‰Ð¥?¡KyD—rAâsÍ²"ñ¾G]¾"4zÕHe¢¡»!]‰X2æL¨
iî$ÉÀæß²jHþ†EíM²áõ¶o\w°k¢TÛœÉŽÕˆ"	üªå…ã¥„ÂqŠNäåˆ÷dÉqŠÏšfÕ$f¶v(@ ¬.ÇRSÅž’Èâi?Õ\¾*P&û™iÔ^2Êv¦-Gç{bzó”ñãQ;T÷ùÿ\«Þ¾#ÜgË÷;ô²±::ƒÕbþµµàþ>‚ õjú€Ã‘ Âø í&×•±	Zþ6³‹±b/­*Ì4ôr'[•‡hÝ‡Û&¥.£¹i;Î3‹æñEI-¯íÂîˆ›j°ºÖït¨Xâþ*z•¼ê¢	}å¥jZæ°îµHÞŽÄP±Ï”¿·HaljÃ÷˜]ÃÙÀdó‰ÁR3ù}\u"€,ácA"$FYQ_öº[o’çý®‹åä'þ¶¬ÞsÒ¾d½ŸDõ>F1i¾8¡»ó:@õèðÀ*#Cý^ZbL]+æ¯l¬Ø¹òú!Å(ðI®žßÊ]Ë_-Õ„Ç%xÎÆfÎÒ¬‡ÈVÁÄÝfFUS®kâ93¶ˆH¾ßØ­%&’VÔO ”9×BN}ôb»É$áìfôÑj¢¨DÕ–LgŠ1í§› |m"bÚ0W3âå ªõRÁÌ,Qæ”B:~ÞY&Í_U):TéFž²yžç’+­”žr~ãÌýÔÜ.J:êJ‰Û"¶…RÐÅ6S.bÞDM£Û/ô>R M-4‰ŒJ¬¥„ŸdK)[åŒ¾ã°Ù«`81ŠÕŸ>ã 0—°>í&n¯ÌCjÄ„áÎa¢‡´\jþ’)L¼æá¥0ò)¼¹^(D¶Ãã“Ëš!›82òkOàŒˆÆ<
±	‹qG9Bñ‚Àç¨R++gˆ'Ø©ÃÄß|Ëm„¦Ä”Æ´DØ¬*x°gJ7a¥ÕUBØøZuu³œ|wª)õìOõ¹%©1Ë&xEçÒ9E'ÂÎ%Ä7’Öè	Éo‘üóàõTvŽ­†©~ƒ]0B0$6‹bìNVë·AèWôÙX¥í¤¢ÔQeGôd¢óì³Õ"ŠO‚ô0¥¨ÑF/ By7FTÐJÑC=ë¸M&,b~9Ì{Ñ‰ê¨ÈíÊš€àÇ‚l-EÄÃ9"Qð)¢*'BAØ I8PòPŽ$ ×Ûëm:A¤KØN÷TÂ>'l§bãÖ:Á>ÚÜ>"¨ª7l§aJ½‰ù?ä3‚Lî´6ÎinÄf&•-gÈÎfÍiÓ’Ï`Ê¼-—a‰#ùV¨—À¥Äk÷ÜšzU•¦¸ŒŒ¸Ð„‚®YÏ8³(	nC¬k–>CÇã"‹ºÙËÒP ‹!à‘ŽÚo)àâS¾8°ý>¢_šªíÇê™Æ‘–ä’ŠªÇRÄ@
7³7Ò»OˆQyÐýÖ‘WÜsÅu¥ö]“×ýx¯#¹¯ñþK,|äÁ&¹°H¦‰u‚p¨$}˜t‘ÔsE95s-ïdû˜3UÃ}­Kß@×øU¦¥±“ÿ:py0Á#~y
<þ8‚Úe;×…g‘%˜˜Œn9ûÍ©¤ÓyÊ9`*Â%l*º5ðTd+Uc‚j4ãÃf–×ÜYK´}n†“ø¥’&2#‚âœ º ßn”H±{‚ÃíNvÂ;™BŽ]L‹†I2‹ñÙ•¶a7.âÚ<vÁnäÒÛJœÌÕ4ˆµš|sO „Yqí’¿øIš?ý‡§N_§¯Ó×Cý¡AIñ ð  