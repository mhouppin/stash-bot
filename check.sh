#!/bin/sh

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

	if test x$engine_output == x$expected
	then
		let "SCORE += 1"
	fi
}

check_e "2k1r2r/ppq2pp1/4p3/2pp2Np/3P2n1/7P/PP1N1PP1/2RQ1RK1 b - - 0 17" 1000 c7h2
check_e "r2q1rk1/1p1nb1pp/p2p4/4pbB1/P1Pp4/3P3P/1PN1BPP1/R2Q1RK1 b - - 3 15" 1000 e7g5
check_e "1B4k1/5p1p/8/p6B/2p2n2/P7/n4P1P/6K1 w - - 0 28" 1000 b8f4
check_e "r2qr3/6kp/pp4p1/P2pn3/1P3R2/4n1N1/3QN1PP/5RK1 w - - 2 27" 1000 d2e3
check_e "rnbqk2r/1p3ppp/p2b1n2/1P1pp3/8/P1NBPN2/2P2PPP/R1BQK2R b KQkq - 0 9" 4000 e5e4
check_e "rn3rk1/pbp2pp1/1p2pn1p/6q1/1P5N/P1N3P1/2PPBP1P/R2Q1RK1 w - - 1 14" 4000 f2f4
check_e "r3k2r/2Q2ppp/p4n2/8/1b1B3P/2NK4/1q3PP1/5B1R w kq - 6 18" 4000 c7c6
check_e "2k4r/1pp1RQ1p/3r2p1/p3p3/4P2q/P1N2P2/1PP1N1PP/3R2K1 w - - 1 22" 16000 e7c7
check_e "r1bk1b1r/ppp2Qp1/7p/3Bp1N1/3n2q1/3P1P2/PPP3PP/RNBK3R b - - 0 11" 16000 g4g2
check_e "r1b2rk1/pp4pp/3p3q/3Pp3/3pP1P1/1B1P1p1P/PP3P1K/R2Q1R2 b - - 5 19" 36000 c8g4

echo "score $SCORE/$MAX_SCORE"
