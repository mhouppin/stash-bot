#!/usr/bin/env python3

import chess, chess.pgn, chess.engine
import time, random, multiprocessing, queue, subprocess

## Configuration
THREADS = 3
INPUT_NAME = "Stash.pgn"
OUTPUT_NAME = "Stash.book"
FENS_PER_GAME = 10

VALUE = {
    "1-0": "1.0",
    "0-1": "0.0",
    "1/2-1/2": "0.5",
}

class Visitor(chess.pgn.BaseVisitor):

    def begin_game(self):
        self.fens = []

    def visit_move(self, board, move):
        if not board.is_check() and not board.is_stalemate() and not board.is_capture(move) and move.promotion == None:
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
        with open("SPLIT_OUT_%d.pgn" % ii, "w") as fout:
            for game in piece:
                for line in game:
                    fout.write(line)

    time.sleep(5) # Give time to handle files

def parseGamesFromPGN():

    # Parse each game from the PGN
    games = []
    tokens = []
    count = 0
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
                    print("Initial parsing: %d games read" % len(games))

    print("Total games %d\n" % len(games))
    return games

def parseFENSFromPGNS(idx, q):

    accepted = rejected = parsed = 0
    nextCheckpoint = time.perf_counter() + 1.0

    # Output FENS to a thread specific file
    fout = open("SPLIT_PARSE_%d.fen" % idx, "w")

    # Parse only games from our designated PGN split
    with open("SPLIT_OUT_%d.pgn" % idx, "r") as pgn:

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
                rejected += 1
                continue

            # Sample FENS_PER_GAME times and save the positions
            for fen in random.sample(fens, FENS_PER_GAME):
                fout.write("%s %s\n" % (fen, VALUE[game.headers["Result"]]))

            # No criteria met to skip this game
            accepted += 1

    fout.close()

    # Final stat reporting
    print("Thread # %2d Accepted %d Rejected %d" % (idx, accepted, rejected))

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
            print("%d games processed" % count)

        for p in processes[:]:
            if not p.is_alive():
                p.join()
                processes.remove(p)

    # Build final FEN file from process outputs
    split_pgn_list = ["SPLIT_OUT_%d.pgn" % i for i in range(THREADS)]
    split_fen_list = ["SPLIT_PARSE_%d.fen" % i for i in range(THREADS)]

    with open(OUTPUT_NAME, "w") as fout:
        subprocess.run(["cat"] + split_fen_list, stdout=fout)

    subprocess.run(["rm", "-vf"] + split_fen_list + split_pgn_list)

if __name__ == "__main__":
    buildTexelBook()
