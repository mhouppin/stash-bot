
exe_name=$2

# Check which test we are running

case $1 in
    --asan)
        name=ASan
        suffix='2>&1 | grep -A50 "AddressSanitizer:"'
        threads=1 ;;

    --ubsan)
        name=UbSan
        suffix='2>&1 | grep -A50 "runtime error:"'
        threads=1 ;;

    *)
        exit 0 ;;
esac

# Start by testing simple cases

for args in "d" \
    "go nodes 1000" \
    "go depth 10" \
    "go movetime 1000" \
    "go wtime 8000 btime 8000 winc 500 binc 500" \
    "bench 8"
do
    echo "--- TEST: $name checking, command \'$args\'"
    eval "./stash \'$args\' $suffix"
    if $? == 0
    then
        echo "--- ERROR: $name failed"
        exit 1
    fi
done

# Check if pseudo-games work

cat << EOF > game.exp
set timeout 240
spawn $exe_name

send "uci\n"
expect "uciok"

send "setoption name Threads value $threads\n"

send "ucinewgame\n"
send "position startpos\n"
send "go nodes 1000\n"
expect "bestmove"

send "position startpos moves e2e4 e7e6\n"
send "go nodes 1000\n"
expect "bestmove"

send "ucinewgame\n"
send "position fen 5rk1/1K4p1/8/8/3B4/8/8/8 b - - 0 1\n"
send "go depth 20\n"
expect "bestmove"

send "quit\n"
expect eof
EOF

echo "TEST: $name checking, pseudo-game"
eval "expect game.exp $suffix"
if $? == 0
then
    echo "--- ERROR: $name failed"
    exit 1
fi

rm game.exp
