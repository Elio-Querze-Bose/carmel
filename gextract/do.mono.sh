. ~graehl/isd/hints/aliases.sh
. ~graehl/isd/hints/bashlib.sh
wd=${wd:-exp}
noise=${noise:-.1}
iter=${iter:-50}
in=${in:-10k}
inlimit=${inlimit:-30}
nin=${nin:-1000}
nviz=${nviz:-6}
showvars_required wd noise iter in nviz

mkdir -p $wd
inm=mono.$nin.0$noise
set -e
sub=$wd/$inm
./subset-training.py -n $nin -u $inlimit --pcorrupt=$noise --monotone --inbase=$in --outbase=$sub
alignbase=$sub.i=$iter
for s in e-parse f a info; do
    cp $sub.$s $alignbase.$s
done
[ "$noise" == 0 ] && cp $sub.a $sub.a-gold
#if [ "$iter" -gt 0 ] ; then
 out=$alignbase.out
 log=$alignbase.log
 [ "$skip" ] || ./gextract.py --notest --golda $sub.a-gold --alignment-out $alignbase.a --inbase=$sub --iter=$iter 2>&1 2>&1 >$out | tee $log
pr=" `lastpr $log`"
#fi
vizin=$alignbase.first$nviz
./subset-training.py --comment="noise=$noise iter=$iter$pr" -l 10 -u 25 -n $nviz --inbase=$alignbase --outbase=$vizin
lang=eng vizalign $vizin
showvars pr out log
grep zeroprob $log || echo "no zeroprob"