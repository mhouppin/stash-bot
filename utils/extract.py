import chess.pgn
import os, time, random, multiprocessing, queue

## Configuration
THREADS                 = 3
INPUT_NAME              = "Stash.pgn"
OUTPUT_NAME             = "Stash.book"
FENS_PER_GAME           = 10

VALUE = {
    "1-0": "1.0",
    "0-1": "0.0",
    "1/2-1/2": "0.5",
}

class Visitor(chess.pgn.BaseVisitor):

    def begin_game(self):
        self.fens = []

    def visit_move(self, board, move):
        if not board.is_capture(move):
            self.fens.append(board.fen())

    def result(self):
        return self.fens

def splitPGN():

    # Split games into THREADS-lists
    games = parseGamesFromPGN()
    pieces = [[] for f in range(THREADS)]
    for ii, game in enumerate(games):
        pieces[ii % THREADS].append(game)

    # Output to SPLIT_OUT_N for each piece
    for ii, piece in enumerate(pieces):
        with open("SPLIT_OUT_{0}.pgn".format(ii), "w") as fout:
            for game in piece:
                for line in game:
                    fout.write(line)

    time.sleep(5) # Give time to handle files

def parseGamesFromPGN():

    # Parse each game from the PGN
    games = []; tokens = []; count = 0
    with open(INPUT_NAME, "r") as pgn:

        while True:

            line = pgn.readline()
            if not line: break

            # python-chess expects half/full move counter
            if line.startswith("[FEN"):
                if len(line.split(" ")) <= 6:
                    line = line.replace("\"]", " 0 1\"]")
            tokens.append(line)

            # Count empty lines to check for new games
            if line.strip() == "":
                count += 1

            # Second empty line denotes a new game
            if count == 2:
                games.append(tokens[::])
                tokens = []
                count = 0
                if len(games) % 10000 == 0:
                    print("Initial parsing: {0} games read".format(len(games)))

    print("Total games {0}\n".format(len(games)))
    return games

def parseFENSFromPGNS(id, q):

    accepted = rejected = parsed = 0; outputs = []
    nextCheckpoint = time.perf_counter() + 1.0

    # Parse only games from our designated PGN split
    with open("SPLIT_OUT_{0}.pgn".format(id), "r") as pgn:

        # Parse all games from the PGN
        while True:

            if time.perf_counter() >= nextCheckpoint:
                q.put(parsed)
                parsed = 0
                nextCheckpoint = time.perf_counter() + 1.0

            # Grab the next game in the PGN
            game = chess.pgn.read_game(pgn)
            if game == None: break

            parsed += 1

            # Skip PGNs with strange end results (crashes, timelosses, disconnects, ...)
            if "Termination" in game.headers and game.headers["Termination"] != "adjudication":
                rejected += 1
                continue

            # Fetch all FENs; discard if too few
            fens = game.accept(Visitor())
            if len(fens) < FENS_PER_GAME:
                rejected += 1; continue

            # Sample FENS_PER_GAME times and save the position
            for fen in random.sample(fens, FENS_PER_GAME):
                outputs.append("{0} {1}\n".format(fen, VALUE[game.headers["Result"]]))

            # No criteria met to skip this game
            accepted += 1;

    # Output FENS to a thread specific file
    with open("SPLIT_PARSE_{0}.fen".format(id), "w") as fout:
        for fen in outputs:
            fout.write(fen)

    # Final stat reporting
    print ("Thread # {0:<2} Accepted {1} Rejected {2}".format(id, accepted, rejected))

def buildTexelBook():

    splitPGN() # Split main file into THREADS-pieces

    processes = [] # Process for each PGN parser

    q = multiprocessing.Queue()

    # Launch all of the procsses
    for ii in range(THREADS):
        processes.append(
            multiprocessing.Process(
                target=parseFENSFromPGNS, args=(ii, q)))

    # Wait for each parser to finish
    for p in processes: p.start()

    count = 0

    while len(processes) != 0:
        time.sleep(0.5)
        newCount = count
        while True:
            try:
                value = q.get_nowait()
                newCount += value
            except queue.Empty:
                break

        if newCount != count:
            count = newCount
            print("{} games processed".format(count))

        for p in processes[:]:
            if not p.is_alive():
                p.join()
                processes.remove(p)

    # Build final FEN file from process outputs
    os.system("rm {0}".format(OUTPUT_NAME))
    os.system("touch {0}".format(OUTPUT_NAME))
    for ii in range(THREADS):
        os.system("cat SPLIT_PARSE_{0}.fen >> {1}".format(ii, OUTPUT_NAME))
        os.system("rm SPLIT_OUT_{0}.pgn".format(ii))
        os.system("rm SPLIT_PARSE_{0}.fen".format(ii))

if __name__ == "__main__":
    buildTexelBook()
