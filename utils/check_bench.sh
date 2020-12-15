_BENCHMARK=$(echo "$1" | fgrep "Bench" | sed "s/Bench: //g")
echo "Expected benchmark is $_BENCHMARK"
_OUTPUT=$(./stash bench | fgrep "NODES:" | sed "s/NODES: //g")
echo "Got $_OUTPUT"
test "$_BENCHMARK" == "$_OUTPUT" || exit 1
