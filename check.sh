#!/bin/zsh

if [ $# -eq 0 ]
then
	echo " usage: $0 uci_engine"
	exit
fi

ENGINE="$1"

SCORE=0
MAX_SCORE=0

check_e() {
	let "MAX_SCORE += 1"

	position="$1"

	movetime="$2"

	expected="$3"

	echo uci > tmp
	echo position fen $position >> tmp
	echo go movetime $movetime >> tmp

	engine_output=`./$ENGINE < tmp | grep bestmove | cut -d ' ' -f 2`

	rm tmp

	printf "position %71s - received $engine_output - expected $expected\n" \
		"$position"

	if test x$engine_output = x$expected
	then
		let "SCORE += 1"
	fi
}

# 3Pv3P Endgame

check_e "8/8/p7/1pP1k2p/1P6/P3K3/8/8 w - - 0 1"		19600	e3f3
check_e "8/8/p7/1pP2k1p/1P6/P4K2/8/8 w - - 2 2"		16900	f3g3
check_e "8/8/p7/1pP1k2p/1P6/P5K1/8/8 w - - 4 3"		14400	g3h4
check_e "8/8/p7/1pPk3p/1P5K/P7/8/8 w - - 6 4"		12100	h4h5
check_e	"8/8/p1k5/1pP4K/1P6/P7/8/8 w - - 1 5"		10000	h5g5
check_e "8/8/p7/1pPk2K1/1P6/P7/8/8 w - - 3 6"		8100	g5f5
check_e "8/8/p1k5/1pP2K2/1P6/P7/8/8 w - - 5 7"		6400	f5e6
check_e "8/2k5/p3K3/1pP5/1P6/P7/8/8 w - - 7 8"		4900	e6d5
check_e "8/3k4/p7/1pPK4/1P6/P7/8/8 w - - 9 9"		3600	c5c6
check_e "2k5/8/p1P5/1p1K4/1P6/P7/8/8 w - - 1 10"	2500	d5d6
check_e "3k4/8/p1PK4/1p6/1P6/P7/8/8 w - - 3 11"		1600	c6c7
check_e "2k5/2P5/p2K4/1p6/1P6/P7/8/8 w - - 1 12"	900		d6c6
check_e "2k5/2P5/2K5/pp6/1P6/P7/8/8 w - - 0 13"		400		b4a5
check_e "2k5/2P5/2K5/P7/1p6/P7/8/8 w - - 0 14"		100		a5a6

# BPvP Endgame

check_e "2KB4/1P6/2k5/8/8/8/7b/8 w - - 0 1"			12100	d8h4
check_e "2K5/1P6/1k6/8/7B/8/7b/8 w - - 2 2"			10000	h4f2
check_e "2K5/1P6/k7/8/8/8/5B1b/8 w - - 4 3"			8100	f2c5

echo "score $SCORE/$MAX_SCORE"
