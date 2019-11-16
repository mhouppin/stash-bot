# **************************************************************************** #
#                                                           LE - /             #
#                                                               /              #
#    bench.sh                                         .::    .:/ .      .::    #
#                                                  +:+:+   +:    +:  +:+:+     #
#    By: stash <stash@student.le-101.fr>            +:+   +:    +:    +:+      #
#                                                  #+#   #+    #+    #+#       #
#    Created: 2019/11/09 17:54:19 by stash        #+#   ##    ##    #+#        #
#    Updated: 2019/11/09 18:02:41 by stash       ###    #+. /#+    ###.fr      #
#                                                          /                   #
#                                                         /                    #
# **************************************************************************** #

valgrind --tool=callgrind ./stash-bot << EOF
setoption name Threads value 4
position startpos moves e2e4 e7e5 g1f3 b8c6
go movetime 5000
EOF

valgrind --tool=callgrind ./stash-bot << EOF
position fen 8/ppp5/8/PPP3kp/8/6KP/8/8 w - - 0 1
go movetime 10000 searchmoves a5a6 b5b6 c5c6
EOF
