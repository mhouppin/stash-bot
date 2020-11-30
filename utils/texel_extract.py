import chess
import chess.pgn
import sys

def main(argv):
    if len(argv) < 2:
        print("usage: {} file.pgn [other.pgn ...]".format(argv[0]))
        return

    epd_file = open("output.epd", 'w')

    for filename in argv[1:]:
        with open(filename, 'r') as f:
            print("Reading file \"{}\"...".format(filename))
            count = 0
            game = chess.pgn.read_game(f)

            while game != None:
                count += 1
                if count % 100 == 0:
                    print("Parsed {} games".format(count))

                result = 0.0 if game.headers["Result"] == "0-1" else 1.0 if game.headers["Result"] == "1-0" else 0.5

                while len(game.variations) != 0:
                    game = game.variations[0]

                    if "book" in game.comment:
                        continue

                    if "+M" in game.comment or "-M" in game.comment:
                        break

                    board = game.board()

                    epd_file.write("{} {}\n".format(board.fen(), result if board.turn == chess.WHITE else 1.0 - result))

                game = chess.pgn.read_game(f)

    epd_file.close()

if __name__ == '__main__':
    main(sys.argv)
