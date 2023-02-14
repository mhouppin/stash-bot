

# Start by checking if bench result matches the committed value

echo "--- TEST: Bench value matching"

_BENCHMARK=$(echo "$1" | grep -m 1 -F "Bench:" | grep -o '[1-9,]\+' | tr -d ',')
if test -z "$_BENCHMARK"
then
    echo "--- WARNING: Missing bench field, skipping bench test"
else
    echo "Expected bench: $_BENCHMARK"
    _OUTPUT=$(./stash bench | grep -m 1 -F "NODES:" | grep -o '[1-9]\+')
    echo "Our bench: $_OUTPUT"
    if test "$_BENCHMARK" != "$_OUTPUT"
    then
        echo "--- ERROR: bench values don't match"
        exit 1
    fi
fi

# Test if search is reproducible

cat << EOF > repeat.exp
set timeout 10
lassign \$argv nodes
spawn ./stash

send "uci\n"
expect "uciok"

send "ucinewgame\n"
send "position startpos\n"
send "go nodes \$nodes\n"
expect "bestmove"

send "position startpos moves e2e4 e7e6\n"
send "go nodes \$nodes\n"
expect "bestmove"

send "ucinewgame\n"
send "position startpos\n"
send "go nodes \$nodes\n"
expect "bestmove"

send "position startpos moves e2e4 e7e6\n"
send "go nodes \$nodes\n"
expect "bestmove"

send "quit\n"
expect eof
EOF

for i in $(seq 1 20)
do
    nodes=$((100*3**i/2**i))
    echo "--- TEST: reprosearch with $nodes nodes"

    expect repeat.exp $nodes 2>&1 | grep -o 'nodes [0-9]\+' | sort | uniq -c | awk '{if ($1%2!=0) exit(1)}'
    if test $? != 0
    then
        echo "--- ERROR: failed reprosearch"
        exit 1
    fi
done

rm repeat.exp

# Verify perft values

cat << EOF > perft.exp
set timeout 10
lassign \$argv pos depth result
spawn ./stash
send "position \$pos\n"
send "go perft \$depth\n"
expect "nodes \$result" {} timeout {exit 1}
send "quit\n"
expect eof
EOF

function check_perft {
    echo "--- TEST: perft on position '$1' at depth '$2'"
    expect perft.exp "$1" "$2" "$3" > /dev/null
    if test $? != 0
    then
        echo "--- ERROR: failed perft"
        exit 1
    fi
}

check_perft startpos 5 4865609
check_perft "fen r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1" 5 193690690
check_perft "fen 8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1" 6 11030083
check_perft "fen r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1" 5 15833292
check_perft "fen rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8" 5 89941194
check_perft "fen r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10" 5 164075551

rm perft.exp
