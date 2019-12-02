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

	if test x$engine_output != x
	then
		if [[ "$engine_output" =~ "$expected" ]]
		then
			let "SCORE += 1"
		fi
	fi
}

# 1-mate moves

check_e "6k1/4Rppp/8/8/8/8/5PPP/6K1 w - - 0 1"								1000	"e7e8"
check_e "8/1p6/kp6/1p6/8/8/5PPP/5RK1 w - - 0 1"								1050	"f1a1"
check_e "R7/4kp2/5N2/4P3/8/8/8/6K1 w - - 0 1"								1100	"a8e8"
check_e "5r2/1b2Nppk/8/2R5/8/8/5PPP/6K1 w - - 0 1"							1150	"c5h5"
check_e "6rk/6pp/8/6N1/8/8/8/7K w - - 0 1"									1200	"g5f7"
check_e "7k/5B1p/8/8/8/8/8/5KB1 w - - 0 1"									1250	"g1d4"
check_e "r1bq3k/pp2R2p/3B2p1/2pBbp2/2Pp4/3P4/P1P3PP/6K1 w - - 0 1"			1300	"d6e5"
check_e "2kr4/3p4/8/8/5B2/8/8/5BK1 w - - 0 1"								1350	"f1a6"
check_e "5k2/8/6Q1/8/8/6B1/8/6K1 w - - 0 1"									1400	"g3d6"
check_e "7k/5R2/5N2/8/8/8/8/7K w - - 0 1"									1450	"f7h7"
check_e "7k/7p/8/6N1/8/8/8/6RK w - - 0 1"									1500	"g5f7"
check_e "7k/5p1p/8/8/7B/8/8/6RK w - - 0 1"									1550	"h4f6"
check_e "5rk1/5p1p/8/8/8/8/1B6/4K2R w - - 0 1"								1600	"h1g1"
check_e "5rk1/6p1/6P1/7Q/8/8/8/6K1 w - - 0 1"								1650	"h5h7"
check_e "6k1/5p2/5PpQ/8/8/8/8/6K1 w - - 0 1"								1700	"h6g7"

# 2-mate moves

check_e "2r1r1k1/5ppp/8/8/Q7/8/5PPP/4R1K1 w - - 0 1"						4000	"e1e8|a4e8"
check_e "5r1k/1b2Nppp/8/2R5/4Q3/8/5PPP/6K1 w - - 0 1"						4200	"e4h7"
check_e "6rk/6pp/6q1/6N1/8/7Q/6PP/6K1 w - - 0 1"							4400	"h3h7"
check_e "3r3k/1p1b1Qbp/1n2B1p1/p5N1/Pq6/8/1P4PP/R6K w - - 0 1"				4600	"f7g8"
check_e "r3k2r/pbpp1ppp/1p6/2bBPP2/8/1QPp1P1q/PP1P3P/RNBR3K b kq - 0 1"		4800	"h3f3"
check_e "2k1rb1r/ppp3pp/2n2q2/3B1b2/5P2/2P1BQ2/PP1N1P1P/2KR3R b - - 0 1"	5000	"f6c3"
check_e "2kr1b1r/pp1npppp/2p1bn2/7q/5B2/2NB1Q1P/PPP1N1P1/2KR3R w - - 0 1"	5200	"f3c6"
check_e "5rk1/3Q1p2/6p1/P5r1/R1q1n3/7B/7P/5R1K b - - 0 1"					5400	"c4f1"
check_e "5rk1/p4p1p/1p1rpp2/3qB3/3PR3/7P/PP3PP1/6K1 w - - 0 1"				5600	"e4g4"
check_e "4rk2/1p1q1p2/3p1Bn1/p1pP1p2/P1P5/1PK3Q1/8/7R w - - 0 1"			5800	"h1h8"

# 3-mate moves

check_e "6k1/3qb1pp/4p3/ppp1P3/8/2PP1Q2/PP4PP/5RK1 w - - 0 1"				9000	"f3f7"
check_e "5r1b/2R1R3/P4r2/2p2Nkp/2b3pN/6P1/4PP2/6K1 w - - 0 1"				9450	"e7g7"
check_e "2b1Q3/1kp5/p1Nb4/3P4/1P5p/p6P/K3R1P1/5q2 w - - 0 1"				9900	"e8c8"
check_e "5rk1/1b3ppp/8/2RN4/8/8/2Q2PPP/6K1 w - - 0 1"						10350	"d5e7"
check_e "1r5k/6pp/2pr4/P1Q3bq/1P2Bn2/2P5/5PPP/R3NRK1 b - - 0 1"				10800	"f4e2"
check_e "5rk1/1R2R1pp/8/8/8/8/8/1K6 w - - 0 1"								11250	"e7g7"
check_e "r4nk1/pp2r1p1/2p1P2p/3p1P1N/8/8/PPPK4/6RR w - - 0 1"				11700	"h5f6"
check_e "3qrk2/p1r2pp1/1p2pb2/nP1bN2Q/3PN3/P6R/5PPP/R5K1 w - - 0 1"			12150	"h5f7"
check_e "r4r2/1q3pkp/p1b1p1n1/1p4QP/4P3/1BP3P1/P4P2/R2R2K1 w - - 0 1"		12600	"h5h6"

# 5-mate moves

check_e "5rk1/1R1R1p1p/4N1p1/p7/5p2/1P4P1/r2nP1KP/8 w - - 0 1"				25000	"d7f7"
check_e "2rqnrk1/pp3ppp/1b1p4/3p2Q1/2n1P3/3B1P2/PB2NP1P/R5RK w - - 0 1"		26250	"g5g7"
check_e "q1r4r/1b2kpp1/p3p3/P1b5/1pN1P3/3BBPp1/1P4P1/R3QRK1 b - - 0 1"		27500	"h8h1"

# 6-mate moves

check_e "r4rk1/2R5/1n2N1pp/2Rp4/p2P4/P3P2P/qP3PPK/8 w - - 0 1"				36000	"c7g7"
check_e "r1k4r/ppp1bq1p/2n1N3/6B1/3p2Q1/8/PPP2PPP/R5K1 w - - 0 1"			37800	"e6f8|e6c5"
check_e "2r2rk1/5ppp/pp6/2q5/2P2P2/3pP1RP/P5P1/B1R3K1 w - - 0 1"			39600	"g3g7"
check_e "4r1qk/5p1p/pp2rPpR/2pbP1Q1/3pR3/2P5/P5PP/2B3K1 w - - 0 1"			41400	"e4h4"

echo "score $SCORE/$MAX_SCORE"
