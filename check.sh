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

	engine_output=`$ENGINE < tmp | grep bestmove | cut -d ' ' -f 2`

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

# 1-move mates

check_e "6k1/4Rppp/8/8/8/8/5PPP/6K1 w - - 0 1"									1000	"e7e8"
check_e "8/1p6/kp6/1p6/8/8/5PPP/5RK1 w - - 0 1"									1000	"f1a1"
check_e "R7/4kp2/5N2/4P3/8/8/8/6K1 w - - 0 1"									1000	"a8e8"
check_e "5r2/1b2Nppk/8/2R5/8/8/5PPP/6K1 w - - 0 1"								1000	"c5h5"
check_e "6rk/6pp/8/6N1/8/8/8/7K w - - 0 1"										1000	"g5f7"
check_e "7k/5B1p/8/8/8/8/8/5KB1 w - - 0 1"										1000	"g1d4"
check_e "r1bq3k/pp2R2p/3B2p1/2pBbp2/2Pp4/3P4/P1P3PP/6K1 w - - 0 1"				1000	"d6e5"
check_e "2kr4/3p4/8/8/5B2/8/8/5BK1 w - - 0 1"									1000	"f1a6"
check_e "5k2/8/6Q1/8/8/6B1/8/6K1 w - - 0 1"										1000	"g3d6"
check_e "7k/5R2/5N2/8/8/8/8/7K w - - 0 1"										1000	"f7h7"
check_e "7k/7p/8/6N1/8/8/8/6RK w - - 0 1"										1000	"g5f7"
check_e "7k/5p1p/8/8/7B/8/8/6RK w - - 0 1"										1000	"h4f6"
check_e "5rk1/5p1p/8/8/8/8/1B6/4K2R w - - 0 1"									1000	"h1g1"
check_e "5rk1/6p1/6P1/7Q/8/8/8/6K1 w - - 0 1"									1000	"h5h7"
check_e "6k1/5p2/5PpQ/8/8/8/8/6K1 w - - 0 1"									1000	"h6g7"
check_e "4k3/5p2/8/6B1/8/8/8/3R2K1 w - - 0 1"									1000	"d1d8"
check_e "6k1/6P1/5K1R/8/8/8/8/8 w - - 0 1"										1000	"h6h8"
check_e "1r6/pk6/4Q3/3P4/8/8/8/6K1 w - - 0 1"									1000	"e6c6"
check_e "r1b1q1r1/ppp3kp/1bnp4/4p1B1/3PP3/2P2Q2/PP3PPP/RN3RK1 w - - 0 1"		1000	"f3f6"
check_e "rR6/5k2/2p3q1/4Qpb1/2PB1Pb1/4P3/r5R1/6K1 w - - 0 1"					1000	"e5e8"
check_e "3r1r2/4k3/R7/3Q4/8/8/8/6K1 w - - 0 1"									1000	"d5e6"
check_e "8/8/2P5/3K1k2/2R3p1/2q5/8/8 b - - 0 1"									1000	"c3e5"
check_e "3rkr2/8/5Q2/8/8/8/8/6K1 w - - 0 1"										1000	"f6e6"
check_e "5r2/pp3k2/5r2/q1p2Q2/3P4/6R1/PPP2PP1/1K6 w - - 0 1"					1000	"f5d7"
check_e "8/7R/1pkp4/2p5/1PP5/8/8/6K1 w - - 0 1"									1000	"b4b5"
check_e "5rk1/5p1p/8/3N4/8/8/1B6/7K w - - 0 1"									1000	"d5e7"
check_e "7k/6p1/6P1/8/8/1B6/8/6K1 w - - 0 1"									1000	"g6h5"
check_e "2Q5/5Bpk/7p/8/8/8/8/6K1 w - - 0 1"										1000	"c8g8"
check_e "5rk1/7p/8/6N1/8/8/1BB5/6K1 w - - 0 1"									1000	"c2h7"
check_e "1nb5/1pk5/2p5/8/7B/8/8/3R3K w - - 0 1"									1000	"h4d8"
check_e "3q1b2/4kB2/3p4/4N3/8/2N5/8/6K1 w - - 0 1"								1000	"c3d5"
check_e "2kr4/8/1Q6/8/8/8/5PPP/3R1RK1 w - - 0 1"								1000	"d1d8"
check_e "8/3p4/3k4/2R4Q/8/4K3/8/8 w - - 0 1"									1000	"h5e5"
check_e "4k3/R7/4N3/3r4/8/B7/4K3/8 w - - 0 1"									1000	"a7e7"

# 2-move mates

check_e "2r1r1k1/5ppp/8/8/Q7/8/5PPP/4R1K1 w - - 0 1"							4000	"e1e8|a4e8"
check_e "5r1k/1b2Nppp/8/2R5/4Q3/8/5PPP/6K1 w - - 0 1"							4000	"e4h7"
check_e "6rk/6pp/6q1/6N1/8/7Q/6PP/6K1 w - - 0 1"								4000	"h3h7"
check_e "3r3k/1p1b1Qbp/1n2B1p1/p5N1/Pq6/8/1P4PP/R6K w - - 0 1"					4000	"f7g8"
check_e "r3k2r/pbpp1ppp/1p6/2bBPP2/8/1QPp1P1q/PP1P3P/RNBR3K b kq - 0 1"			4000	"h3f3"
check_e "2k1rb1r/ppp3pp/2n2q2/3B1b2/5P2/2P1BQ2/PP1N1P1P/2KR3R b - - 0 1"		4000	"f6c3"
check_e "2kr1b1r/pp1npppp/2p1bn2/7q/5B2/2NB1Q1P/PPP1N1P1/2KR3R w - - 0 1"		4000	"f3c6"
check_e "5rk1/3Q1p2/6p1/P5r1/R1q1n3/7B/7P/5R1K b - - 0 1"						4000	"c4f1"
check_e "5rk1/p4p1p/1p1rpp2/3qB3/3PR3/7P/PP3PP1/6K1 w - - 0 1"					4000	"e4g4"
check_e "4rk2/1p1q1p2/3p1Bn1/p1pP1p2/P1P5/1PK3Q1/8/7R w - - 0 1"				4000	"h1h8"
check_e "rn1r2k1/ppp2ppp/3q1n2/4b1B1/4P1b1/1BP1Q3/PP3PPP/RN2K1NR b KQ - 0 1"	4000	"d6d1"
check_e "8/8/1Q6/8/6pk/5q2/8/6K1 w - - 0 1"										4000	"b6h6"
check_e "1k1r4/pp1q1B1p/3bQp2/2p2r2/P6P/2BnP3/1P6/5RKR b - - 0 1"				4000	"d8g8"
check_e "r1b3nr/ppp3qp/1bnpk3/4p1BQ/3PP3/2P5/PP3PPP/RN3RK1 w - - 0 1"			4000	"h5e8"
check_e "r4r1k/ppn1NBpp/4b3/4P3/3p1R2/1P6/P1P3PP/R5K1 w - - 0 1"				4000	"e7g6"
check_e "2r5/8/8/5K1k/4N1R1/7P/8/8 w - - 0 1"									4000	"e4f6"
check_e "1r1n1rk1/ppq2p2/2b2bp1/2pB3p/2P4P/4P3/PBQ2PP1/1R3RK1 w - - 0 1"		4000	"c2g6"

# 3-move mates

check_e "8/8/3k4/8/8/4K3/8/Q6R w - - 0 1"										9000	"h1h6|a1a6"
check_e "6k1/3qb1pp/4p3/ppp1P3/8/2PP1Q2/PP4PP/5RK1 w - - 0 1"					9000	"f3f7"
check_e "5r1b/2R1R3/P4r2/2p2Nkp/2b3pN/6P1/4PP2/6K1 w - - 0 1"					9000	"e7g7"
check_e "2b1Q3/1kp5/p1Nb4/3P4/1P5p/p6P/K3R1P1/5q2 w - - 0 1"					9000	"e8c8"
check_e "5rk1/1b3ppp/8/2RN4/8/8/2Q2PPP/6K1 w - - 0 1"							9000	"d5e7"
check_e "1r5k/6pp/2pr4/P1Q3bq/1P2Bn2/2P5/5PPP/R3NRK1 b - - 0 1"					9000	"f4e2"
check_e "5rk1/1R2R1pp/8/8/8/8/8/1K6 w - - 0 1"									9000	"e7g7"
check_e "r4nk1/pp2r1p1/2p1P2p/3p1P1N/8/8/PPPK4/6RR w - - 0 1"					9000	"h5f6"
check_e "3qrk2/p1r2pp1/1p2pb2/nP1bN2Q/3PN3/P6R/5PPP/R5K1 w - - 0 1"				9000	"h5f7"
check_e "r4r2/1q3pkp/p1b1p1n1/1p4QP/4P3/1BP3P1/P4P2/R2R2K1 w - - 0 1"			9000	"h5h6"
check_e "rn3rk1/p5pp/2p5/3Ppb2/2q5/1Q6/PPPB2PP/R3K1NR b KQ - 0 1"				9000	"c4f1"
check_e "1k2r3/pP3pp1/8/3P1B1p/5q2/N1P2b2/PP3Pp1/R5K1 b - - 0 1"				9000	"f4h4"
check_e "R7/8/8/7p/6n1/6k1/3r4/5K2 b - - 0 1"									9000	"g4h2"

# 4-move mates

check_e "8/8/3k4/8/8/4K3/8/R6R w - - 0 1"										16000	"h1h6|a1a6"
check_e "2r1nrk1/p4p1p/1p2p1pQ/nPqbRN2/8/P2B4/1BP2PPP/3R2K1 w - - 0 1"			16000	"f5e7"
check_e "6k1/1p1b3p/2pp2p1/p7/2Pb2Pq/1P1PpK2/P1N3RP/1RQ5 b - - 0 1"				16000	"d7g4"
check_e "r4k1r/1q3p1p/p1N2p2/1pp5/8/1PPP4/1P3PPP/R1B1R1K1 w - - 0 1"			16000	"c1h6"
check_e "r2q1rk1/pbp3pp/1p1b4/3N1p2/2B5/P3PPn1/1P3P1P/2RQK2R w K - 0 1"			16000	"d5e7"

# 5-move mates

check_e "8/8/3k4/8/8/2QBK3/8/8 w - - 0 1"										25000	"d3b5|e3f4|e3e4|c3f6"
check_e "8/8/3k4/8/8/2QNK3/8/8 w - - 0 1"										25000	"d3e5|e3f4|c3f6"
check_e "5rk1/1R1R1p1p/4N1p1/p7/5p2/1P4P1/r2nP1KP/8 w - - 0 1"					25000	"d7f7"
check_e "2rqnrk1/pp3ppp/1b1p4/3p2Q1/2n1P3/3B1P2/PB2NP1P/R5RK w - - 0 1"			25000	"g5g7"
check_e "q1r4r/1b2kpp1/p3p3/P1b5/1pN1P3/3BBPp1/1P4P1/R3QRK1 b - - 0 1"			25000	"h8h1"
check_e "r3k3/ppp2pp1/8/2bpP2P/4q3/1B1p1Q2/PPPP2P1/RNB4K b q - 0 1"				25000	"e4h4"

# 6-move mates

check_e "8/8/3k4/8/8/4K3/8/4Q3 w - - 0 1"										36000	"e3d4"
check_e "r4rk1/2R5/1n2N1pp/2Rp4/p2P4/P3P2P/qP3PPK/8 w - - 0 1"					36000	"c7g7"
check_e "r1k4r/ppp1bq1p/2n1N3/6B1/3p2Q1/8/PPP2PPP/R5K1 w - - 0 1"				36000	"e6f8|e6c5"
check_e "2r2rk1/5ppp/pp6/2q5/2P2P2/3pP1RP/P5P1/B1R3K1 w - - 0 1"				36000	"g3g7"
check_e "4r1qk/5p1p/pp2rPpR/2pbP1Q1/3pR3/2P5/P5PP/2B3K1 w - - 0 1"				36000	"e4h4"

# 1-move winning positions

check_e "1k6/ppp3q1/8/4r3/8/8/3B1PPP/R4QK1 w - - 0 1"							1000	"d2c3"
check_e "r4rk1/pp1p1ppp/1qp2n2/8/4P3/1P1P2Q1/PBP2PPP/R4RK1 w - - 0 1"			1100	"b2f6"
check_e "4r1r1/2p5/1p1kn3/p1p1R1p1/P6p/5N1P/1PP1R1PK/8 w - - 0 1"				1200	"f3g5"

# 2-move winning positions

check_e "7k/8/8/4n3/4P3/8/8/6BK w - - 0 1"										4000	"g1d4"
check_e "5k2/p1p2pp1/7p/2r5/8/1P3P2/PBP3PP/1K6 w - - 0 1"						4400	"b2a3"
check_e "4k3/6p1/5p1p/4n3/8/7P/5PP1/4R1K1 w - - 0 1"							4800	"f2f4"
check_e "8/1r3k2/2q1ppp1/8/5PB1/4P3/4QK2/5R2 w - - 0 1"							5200	"g4f3"
check_e "r2r2k1/2p2ppp/5n2/4p3/pB2P3/P2q3P/2R2PP1/2RQ2K1 w - - 0 1"				5600	"c2d2"
check_e "8/3qkb2/8/8/4KB2/5Q2/8/8 b - - 0 1"									6000	"f7d5"

# 3-move winning positions

check_e "q5k1/5pp1/8/1pb1P3/2p4p/2P2r1P/1P3PQ1/1N3R1K b - - 0 1"				9000	"f3h3"
check_e "4rr1k/ppqb2p1/3b4/2p2n2/2PpBP1P/PP4P1/2QBN3/R3K2R b KQ - 0 1"			9900	"e8e4"
check_e "2Q5/1p4q1/p4k2/6p1/P3b3/6BP/5PP1/6K1 w - - 0 1"						10800	"g3e5"
check_e "5Q2/2k2p2/3bqP2/R2p4/3P1p2/2p4P/2P3P1/7K w - - 0 1"					11700	"f8f7"
check_e "6q1/6p1/2k4p/R6B/p7/8/2P3P1/2K5 w - - 0 1"								12600	"a5a8"

# 4-move winning positions

check_e "5k2/pp1b4/3N1pp1/3P4/2p5/q1P1QP2/5KP1/8 w - - 0 1"						16000	"e3h6"


echo "score $SCORE/$MAX_SCORE"
