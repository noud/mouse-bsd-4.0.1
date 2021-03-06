#!/bin/sh
#
# Test dd(1) functionality and bugs
#

test_dd_io() {
	res="`echo -n "$2" | eval $1`"
        if [ x"$res" != x"$3" ]; then
		echo "Expected \"$3\", got \"$res\": $1"
		exit 1
	fi
}

allbits1="\000\001\002\003\004\005\006\007\010\011\012\013\014\015\016\017\020\021\022\023\024\025\026\027\030\031\032\033\034\035\036\037\040\041\042\043\044\045\046\047\050\051\052\053\054\055\056\057\060\061\062\063\064\065\066\067\070\071\072\073\074\075\076\077\100\101\102\103\104\105\106\107\110\111\112\113\114\115\116\117\120\121\122\123\124\125\126\127\130\131\132\133\134\135\136\137\140\141\142\143\144\145\146\147\150\151\152\153\154\155\156\157\160\161\162\163\164\165\166\167\170\171\172\173\174\175\176\177\200\201\202\203\204\205\206\207\210\211\212\213\214\215\216\217\220\221\222\223\224\225\226\227\230\231\232\233\234\235\236\237\240\241\242\243\244\245\246\247\250\251\252\253\254\255\256\257\260\261\262\263\264\265\266\267\270\271\272\273\274\275\276\277\300\301\302\303\304\305\306\307\310\311\312\313\314\315\316\317\320\321\322\323\324\325\326\327\330\331\332\333\334\335\336\337\340\341\342\343\344\345\346\347\350\351\352\353\354\355\356\357\360\361\362\363\364\365\366\367\370\371\372\373\374\375\376\377"

ebcdicbits1="\000\001\002\003\067\055\056\057\026\005\045\013\014\015\016\017\020\021\022\023\074\075\062\046\030\031\077\047\034\035\036\037\100\132\177\173\133\154\120\175\115\135\134\116\153\140\113\141\360\361\362\363\364\365\366\367\370\371\172\136\114\176\156\157\174\301\302\303\304\305\306\307\310\311\321\322\323\324\325\326\327\330\331\342\343\344\345\346\347\350\351\255\340\275\232\155\171\201\202\203\204\205\206\207\210\211\221\222\223\224\225\226\227\230\231\242\243\244\245\246\247\250\251\300\117\320\137\007\040\041\042\043\044\025\006\027\050\051\052\053\054\011\012\033\060\061\032\063\064\065\066\010\070\071\072\073\004\024\076\341\101\102\103\104\105\106\107\110\111\121\122\123\124\125\126\127\130\131\142\143\144\145\146\147\150\151\160\161\162\163\164\165\166\167\170\200\212\213\214\215\216\217\220\152\233\234\235\236\237\240\252\253\254\112\256\257\260\261\262\263\264\265\266\267\270\271\272\273\274\241\276\277\312\313\314\315\316\317\332\333\334\335\336\337\352\353\354\355\356\357\372\373\374\375\376\377"

allvisbits=`echo -n "$allbits1" | unvis | vis`
ebcdicvisbits=`echo -n "$ebcdicbits1" | unvis | vis`

# This checks the combination of bs= with conv=ebcdic
# Prior to revision 1.24 of dd's args.c, the conv option
# would be ignored.

test_dd_io "unvis | dd 2>/dev/null | vis" "$allvisbits" "$allvisbits"
test_dd_io "unvis | dd ibs=1 2>/dev/null | vis" "$allvisbits" "$allvisbits"
test_dd_io "unvis | dd obs=1 2>/dev/null | vis" "$allvisbits" "$allvisbits"
test_dd_io "unvis | dd bs=1 2>/dev/null | vis" "$allvisbits" "$allvisbits"

test_dd_io "unvis | dd conv=ebcdic 2>/dev/null | vis" "$allvisbits" "$ebcdicvisbits"
test_dd_io "unvis | dd conv=ebcdic ibs=512 2>/dev/null | vis" "$allvisbits" "$ebcdicvisbits"
test_dd_io "unvis | dd conv=ebcdic obs=512 2>/dev/null | vis" "$allvisbits" "$ebcdicvisbits"
test_dd_io "unvis | dd conv=ebcdic bs=512 2>/dev/null | vis" "$allvisbits" "$ebcdicvisbits"

test_dd_io "unvis | dd conv=ebcdic 2>/dev/null | vis" "$allvisbits" "$ebcdicvisbits"
test_dd_io "unvis | dd conv=ebcdic ibs=1 2>/dev/null | vis" "$allvisbits" "$ebcdicvisbits"
test_dd_io "unvis | dd conv=ebcdic obs=1 2>/dev/null | vis" "$allvisbits" "$ebcdicvisbits"
test_dd_io "unvis | dd conv=ebcdic bs=1 2>/dev/null | vis" "$allvisbits" "$ebcdicvisbits"

exit 0
