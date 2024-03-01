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
� �F*S�}y|U�puRIl��%jT��iJhX�E��ָ@X�e:#��:��`b��ʞ��2:��o�e EV��0�CD
�������q�}<����W��|�~Z㟷?����׼~3�W�z���n�a}?�?��x=���q~�����yI��	�Ѿ��G��Q^����?�\�fa[�z^�ӯ��(���ר��4�V�����/̸7�Ĺ��͓�.1V^��<aƌ��F0wi���l\�wy/�](̓y�i��P��d��B����+	��-Z"	��2O�s.�%	���^���x�%���]�0o�y�<j���`.��#�d�b)w�!/o����܅s��$ؙ�
�_ԱH�W�e�ss���``����(���>�]К
%l&�y8��&��
$���5+}�n5��S�
T�a����-0�<�Rw�� ]|b
��j���w\��=K#�3���f9��j��*�>;�1>�a�<I�|#S��]I��"dt��U 	wkP�u�:�O�r��������9��W�MQD�l��t��>l�s8�N�(F82ET�HPG�KEy��rhS��k4��
}�a�oT��p7U�G�E��e��=�tOVo�� W�K�=S���7�b�����omtԽ��n�E�� �R<�V�{)�mֆ�;��ɡ�D������YF�	��"��H��g.��@�LJi_��*�QF@��4	��C�Lj�+͞��^�*IQ� dP��4
$�6�Cq =�ˉ���v�ʛ�ߒY��r}�Lw<2�O�;
敕�*Ʃwч���B�Ҹ�.��Z�^�n0���l8l5��m��)d�t�9rueF��5�#1n�!������S&)�`�M�ڗf�նCjC��Ve�
1�3�5�[�c�{���^&�bc-o<.u��l��=ˏL��O��S9���WtvP���o �=����V�+�ǥ6Į�O�W;�J�=|�B[q0lJ��|}���VG������&�ެ�o�ٛ�U�V��4�������7bH�[���Zq��4fΰ�=f^ko6�U���ԆW_Goۥ��:�
C�d��N�Iq������ϕ�P|��$�v*�N>8@}#tVM����Jf��w�ޚQ�6-]d��i���F+A~���ab�k�Xu����
�Ȏ A���,�h�M�X���gW���1��0R���|��"Uwjܠ8���O[ѐ8d��	s�x�[ko2��z=�4j
e�1Θ �v�����v����)�0��X��!6θf�����[�0���1�=�����}q�A���ᲜV���s�:�sW��S�9ׅ���9L WɱpЈ���8�"9����P`�x�!���"P����`~�����7�����f��OL0�����	'dƂ�ݻĩ$�26?��>�{u!��f�i@�@<��9���x[�׏����A:�W�4��R�_CGX���*~o��8X�m�w�Y�x�����)�3MԴ'g�*�����<zG���n��K�G?�xRJS�ko��.�JGB�N����ӯ���>�����?���A:)�I��Ľ���l��G��؂���h��KQ\h=�#�͓��ͥ�af�s�'�W4��\�������>��1s�(��1(��ƭ�^�I��!�"�/� ���]��Tp��>���]��~���<A�܈���a(CP�7��"s�ހ:�>y8�ZK�pQKX�4nEφa�[G��~�Rārׂ-�[ǀK��>�G��z8��k�؏x�-J1��p�{8���.~Ldn�
Q����  �uƶ�
����aW��6��n�-�s\ �G��Yv��n��l�9�����"8���8�	�Qf�S�3>�p~�Dg�@�����N6���O�*�?C`*��h}�T�U���M%M��0�p��M%:O���}���4�i��~7��
SP�
���ma�%c)i@���Q8��A��#o�#�m!�h�`ԪV>�P4z���qkg_,$2�μ�j3�����$�g�h)r[;c^Jg픾�,��мЮ�G������&�_�����9��z�TG�{�ˎ�7�M~��F_���7���dK�<���h/�����(�g���ϵ3Ε���ǧ[:�u�թ-���Zt�I�ܺp��B��F�����puϿ/F�O�;DϮ28#��y���G7\���Zt���=s�a�9�Qzt�շN��=�u�\5 px=:L�n���$_x�4��^����ލ_hou�����-AZ|����ho�˼����h�	x����2PtK;"����ؿ^�J,=ȉ�/c�;"�a���D'�9�����������f��8��nb����0h�p�oQ_�<Ļ�y8� �C�O`m�[X��E�+�N�F�Û�}��Ӈw������з��]��u�H�;C��A�4��ûz���"��[@~"�����!4"�>���/��K��>���X���đ;�>t�շ~ԻFԢ��]׆����pg�ٻ�H}����B8�
��²<���7|!�&��J��Rԯk� �D<���т�/h@{E�� �O3ᾩ$P��F��Q�5V3�h�ŮL0�jH����;8Z����	NR��M����C���{j4����D�Qco���ы]��U_�^}4nh��Ѹ�5���Cu$�����!���PiH�Й��C�>9r�2#��O0H7D���:t
����d��S��{�.���%-��p�@����*i���q�\i�#�����E�el��\*�[�Vۓۛ��2�C��_Y/�p.x�>�u����n���a�g�?��_��k���?���u����o���؁���1��E�c�E�Y�j÷�x�~A��2�\�vҶ���'�{��m?aQ��d����!}<g�|%v�Ɛ�c�������;iޘc
�΢3��������p��ʹ��A6z.�B
�������J�[�����O����>=�ڈ�_��A�����QO�������&V���v��wA�����EG������R�'�gR
�ph�ڇF���[�/�6x�%L��y�W�?Y���F _���n����rPv��u�OU�x���X����7�z_�F�1t�/x�`\���r�E2+�D<��w��>�h	�<��{P�OR��kXT�Y�{VV_ۡY���#0�i�uP�ן�E�����hq��)��<%�I�sVL�R,,{ф�u�ȋ��y�5��z��0[C�_�K>���d?�zZ���=�}�Z���z%a�p­5p�5���H���ػEr���A�/�6�ؒ�:��؂S�㇋&m��=wS*�W��r5�s���n;��գ����Ȧ�p�����X���Y~��eybk*���`f)y��
�]����SfW��T�
��lQl��u��Lx�;�������r�n����j;��,~���9-I�L43�Z�(R�4�
_<��<�K���boS6{����u���U{�����̅��Br�61>�}_��a��&�s¶	�B,��$A���D�h�Z�Sz�����S���.D��ǉ4r_���=��-��!o=���i`���Թ5R{i�>�!�T@�Q�C��R>э�JOq�}F��!�׆?��W��5aÅ�}��v�ֽ�)z	@Ao&���^4zq��1<�]�����)L����
���4U�1�0C�4��i�#��M.p��y����
�����]�R�7l}W�˴W�TQ ��xq��
�Ƚ�T|� "ݭ�]a�}��p���c�RQ���kݮ�O�}��~D��>����Tk�����m	EԔ�je����;����O�?s��c
��M������,nj�%'��繃��O�J��s�\��~� D���#zo0�'I��k����R���
�g4 �T��1(Td��\��g��S���;P�>�}|�@w����w�J�ӿ���C.���՛^K�V�����a�ʻ�Tڑ%�	�������2���O�r�Y���e{��h
�7e$J�P�U\ǒ]�B�W
�1�3��l8�?�yo��˵[N��q�2��U���;_�p������?̗ �N��N�?�$W��f�l�/T��8&�Eɴh��o��d�{�Lt`�E���uF4�k�I)�!	4��~��）��L����B��0��M��7)u��(�h&X�&�r+����찺L�ԝ��xS��{,�	f��[�m�&��^O{IN=�ڊ�}�+�Ǣ�׵�-o���zŬo��v�Jս�

��z���ͥ�v��aO��Jˍrq�\��!�&7���o�`��?��[F���L�ɪ�W���k��A&����TB}~�'�L@���J��;���)$��&ciL>�#�)\��n7Ů.�tr�T8a	�#���%��zY8��Ҥ�!76!��%�I��	��1�x�WLw�&��[?�)���F!�>xQ��30t^��	��y6_t]֨;�����}�a#
YU=��D���|�Z	�h^�U���7;��L��*���t"pA��Y�KF���/t)K��<���5�1s�/��M��$�Z-��ŏk/J�j��Fߗ*'p��:1�U
�Ř^|3d��_i�jTI_���o�VU��<� N
��*�&ݺyTq���Q]z�0Q�6���t8CM
��{�,�V���$�
��m��=�o�_3�PC�]0���B�"˾�c�M@h� �/��ϴSc7F�÷� [,h��Qo�`a�~]�Q�/T��G=����d�m��
�'��H�>#*�m��=����__F�wm[C�0����~�-d�
K��ts��TES����Z��7�|V|8�?��p~B�
���!�65�~s�@OWiB�F:
tF%�I⋎�5q"Y�^����7]��,@���f��c���9���,=�� �&�,�]q�p�d2��3�0�+f�g�O���A�ٱI�]��Eo�f���w�#��V
�Op�����wr��Pd^�R(r�	f�k�P̔[���	�X=�tJne�Ü��,L�(/WK�X�-+�{WS��ٴ����*�;�@ڃ�(�
pY��>>��c�.�HS8�4�f����o�#���no)^ �k �\<��K��ߐ�ܚ
�ث��+s?�P0���[�a.lSt6:���)��g����A���S;�U��j+[Pf������*����r��k�+����H��B���8�p�=*����P�b[~�a��b��Կ~nV@�r�G��0֡:Y����.����I��}ީ���/4��/d���n�����D��#̚9�YSF��YE�GZk���^	e�	ހ(xV�W��2,>cX�8,��C�K�n�Fy��D�'q�/�/GN�Z�,�am�¹<�}�����H�]���s���<`�>|� p�xpu� G�� �����-�_ /�^� � x�K�6�S� Px��}� ��G@�	X��g<���4O������(|K���éӨ&��<�`�`��� ��`�8M�:��d���$��O���_�)ɡ�=��aϩ�5�S�t/�����נI��נ��'xbW�^�ϮS�����,��j����oG��9[x
�~��s��됪�Q��3����߀�V@#�M�k�݀��m� ���� v�}����O��� �
p�� p ��� �ת�ˀ?�t�88��;`;@lT66 \��ڄW�-pӫ e ���
��`, �L��������N�)v�E��.Zj��FC�0��Rd��<��쟒�*�09Bw�ֵ����/��h7��_<]���XQ����V��lUU
�iJ��197��9�����)
�:���G�(t8_Ift(����W<='�!f0�Ln���r�����\&��L��T��G>7�H��2��K��fX�d:�\?
t�|]Y.�\î�qA?��WY��
:Gr�Ҍ�٘%ӕ��ǰq,GCǞ������ �
�N�J�X��[�|
:t��ۜHSҽ��$���z�ka����"��@wz�r�
:�g���F�kS�m�9��j�������G:\qk�t2i$݇�g�#�ǉ�<�6����wy���������-#~�g���8{�,7���f����3f���2�837��5sg�������E�O~��iu۬�2������q�*U�'���bl?��<��P˃ޯ�r����iy��@.`�g}>�y0!&�1�g}~�4��V�19�c�x�%H�X�x���� �zHC��������e#���
��K�^@:{�����\������Z{��6ݍ���k��;�w�'
�u>�O�|H�c�ll��{�����й�^�\��(n �0b݃��q!t�K,����N�Z�A�ͷ��qbjwF��ߗ�x�^�2qwS2��Z�^4n"&�P�����s�v9c�/������te���JT��⃦��!�k�?�v�4�����%�����j\C��k	S���kSh���u�2_ch�N��W�Uw�,�A.���H�_E���y��k9��Qc���������{�̑ȹ�a^}��T�q��S��L�q� �3Oi��u�/�
�UM毹:�Wa�JVt����A)u5�A6������N�`>DW�hp�-D{
��tP�E��t�zU�:����s�@U�x1_�?��7A1X�m}#+�U�CP&��5��nj/�ب����L;�,�&Q���}}�������k�̯*>I�eR�Jb��?�S�����mt�2ܤ�	J��9C����X�J�\	C�.3��)˕.R~<���O�:=�����kv�1-��X|6j��m��+6';Y�9�=xT>x$U�#��P�� �]�

�E/A�CnE�ܚcOE��襷ނ"��KQ���������rI.eK�[1�By������
��^��2`���7�^A�)��:�!n��H�mx���!i!|����V׽�sۍ}v�����{W��N;���;�y��.��e�?���gg��� ���sq��O�;�Y������<vO?�����������_`̿����7���������7�݆�;���?����}�4�?/9����y}��H�ק��y}�����G�����g&��Q1w������O�Lo'��Q�8sJ^�
c��n���	��]�F�m��Ĵ����d
���H'��}�ݰsg�^�(����j�]��Txng�	*�^w߀�p�HS��S��W����^Gm��Z;>� �D�M������p<��@�X��\b�q��.���;(bR��.D�ت�Xb���y� 躩3�g(	�2IF�.l��@ҭ7vH��2���@������A�J5bWb0�>���������>�'Q_x�]3zs���p�X�1�uY*	��Jd�I��ow�9�:L��.����D��'��F�
/�^)������	�\��s�7і�[���>�UE�=��E4`M�;��[��]�������5f*��Lf_B!�����`��;8�RF���R\`i��|�8�V�/���yF�	�u\ ��چC=�XI��Z��0�(
�1�a�����ۮ
e̕9_�HO\J���O/a�X���kβ�
u���xWE����;�P��w��6]
i�$���߲jH��E�M����o\w�k�TۜɎՈ"	�����㥄�q�
�	�qG9B���R++g�'ة���|�m����ƴDج*x�gJ�7a��UB��Zuu��|w�)��O��%�1�&xE��9E'��%�7���	�o�����Tv�����~�]0B0$6�b��NV�A�W��X�����QeG�d����"�O��0���F/ By7FT�J�C=�M&,b~9�{щ����ʚ��ǂl-E��9"Q�)�*'BA� I8P�P�$ ���m:A�K�N�T�>'l�b��:�>��>"��7l�aJ���?�3�L�6�inĝf&�-g��f�iӒ�`��-�a�#�V�����k�ܚ�zU�������Є��Y�8�(	nC�k�>C��"�����P �!����o)��S�8��>�_�����Ƒ������R�@
7�7һO�Qy��֑W�s�u��]����x�#����K,|��&��H��u�p�$}�t��sE95s-�d��3U�}�K�@��U������:py0�#~y
<�8��e;ׅg�%���n9�ͩ��y�9`*�%l*�5�Td+Uc�j4��f���YK�}n�����&2#�✠���n�H�{���Nv�;�B�]L��I2��ٕ�a7.��<v�n���J���4����|sO��Yq풿�I�?���N_����C��AI� �  