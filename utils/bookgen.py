from typing import Iterable
import chess
import chess.engine
import optparse
from functools import total_ordering
from queue import PriorityQueue, Queue, Empty
import threading
from time import sleep, perf_counter
from datetime import timedelta

# Small wrapper class to avoid being annoyed with storing lines
class Line:
    def __init__(self, threshold: int, pv: Iterable[chess.Move] = []) -> None:
        self.threshold = threshold
        self.pv = [move for move in pv]

    # Adds a move to the PV and updates move acceptation threshold.
    def push_move(self, move: chess.Move, loss: int) -> None:
        self.threshold -= loss
        self.pv.append(move)

    def __lt__(self, other) -> bool:
        return self.threshold > other.threshold

    def __eq__(self, other) -> bool:
        return self.threshold == other.threshold

# Small wrapper class for communication between workers
class Work:
    def __init__(self, worker_count: int) -> None:
        self.worker_count = worker_count
        self.waiting = 0
        self.analyzed_lines = 0
        self.lines_in_queue = 1
        self.lock = threading.Lock()
        self.lock2 = threading.Lock()

    def wait(self) -> None:
        self.lock.acquire()
        self.waiting += 1
        self.lock.release()

    def stop_waiting(self) -> None:
        self.lock.acquire()
        self.waiting -= 1
        self.lock.release()

    def analysis_done(self) -> bool:
        self.lock.acquire()
        ret = self.waiting == self.worker_count
        self.lock.release()
        return ret

    def inc_queue(self) -> None:
        self.lock2.acquire()
        self.lines_in_queue += 1
        self.lock2.release()

    def dec_queue(self) -> None:
        self.lock2.acquire()
        self.lines_in_queue -= 1
        self.lock2.release()

    def inc_analysis(self) -> None:
        self.lock2.acquire()
        self.analyzed_lines += 1
        self.lock2.release()

    def show(self, start: float) -> None:
        self.lock2.acquire()
        A = self.analyzed_lines
        Q = self.lines_in_queue
        P = A * 100.0 / (A + Q)
        if A != 0:
            print("queue/total: %d/%d (%.2f %% done)   " % (Q, A + Q, P), end='\r', flush=True)
        self.lock2.release()

# Returns the MultiPV value to use for a given ply. Probably tweakable quite a bit.
def get_multipv(ply: int) -> int:
    return max(4, int(20.0 / ((ply + 1) ** 0.5)))

def generate_lines(analysis_queue: PriorityQueue, pgn_queue: Queue, work: Work, options, engine_path) -> None:
    engine = chess.engine.SimpleEngine.popen_uci(engine_path)
    if options.hash != 0:
        engine.configure({"Hash": options.hash})

    while True:
        cur_line = None

        try:
            cur_line = analysis_queue.get_nowait()

        except Empty:
            work.wait()
            while True:
                sleep(1.0)
                try:
                    cur_line = analysis_queue.get_nowait()
                    work.stop_waiting()
                    break

                except Empty:
                    if work.analysis_done():
                        engine.quit()
                        return

        work.dec_queue()
        work.inc_analysis()

        # If we already reached the max number of plies for this line, don't analyse further.
        if len(cur_line.pv) == options.max_depth:
            continue

        # Initialize board with the current line moves
        board = chess.Board()
        for move in cur_line.pv:
            board.push(move)

        info = engine.analyse(board, chess.engine.Limit(nodes=options.nodes), multipv=get_multipv(len(cur_line.pv)))

        # Save the bestmove score for adjusting line precision later
        base_score = info[0]["score"].relative.score(mate_score=40000)
        for pv_info in info:
            score_diff = base_score - pv_info["score"].relative.score(mate_score=40000)

            # If the current line play is too suspicious, don't add it to the list.
            if score_diff > cur_line.threshold:
                continue

            # Append the entry to the list of lines to analyse later, and adjust precision
            new_line = Line(cur_line.threshold, cur_line.pv)
            new_line.push_move(pv_info["pv"][0], score_diff)
            analysis_queue.put_nowait(new_line)
            pgn_queue.put_nowait(Line(new_line.threshold, new_line.pv))
            work.inc_queue()

def main() -> None:
    usage = "usage: %prog [options] engine_path pgn_output"
    parser = optparse.OptionParser(usage=usage)
    parser.add_option("-p", "--plies", type="int", dest="max_depth", default=8)
    parser.add_option("-t", "--threads", type="int", dest="threads", default=1)
    parser.add_option("-c", "--cp-threshold", type="int", dest="threshold", default=50)
    parser.add_option("-x", "--hash", type="int", dest="hash", default=0)
    parser.add_option("-n", "--nodes", type="int", dest="nodes", default=1000000)
    (options, args) = parser.parse_args()
    if len(args) != 2:
        print("Error: 2 positional arguments expected, got %d" % len(args))
        return

    analysis_queue = PriorityQueue()
    analysis_queue.put_nowait(Line(options.threshold))
    pgn_queue = Queue()
    work = Work(options.threads)
    start = perf_counter()

    thread_list = [threading.Thread(target=generate_lines, args=(analysis_queue, pgn_queue, work, options, args[0],)) for i in range(options.threads)]

    for thread in thread_list:
        thread.start()

    # Start reading the PGN queue and writing lines to the given file
    with open(args[1], 'w') as pgn_file:
        finished = False
        while not finished:
            sleep(5.0)
            while True:
                try:
                    line = pgn_queue.get_nowait()
                    pgn_file.write("[Result \"1/2-1/2\"]\n\n%s 1/2-1/2\n\n" % chess.Board().variation_san(line.pv))

                except Empty:
                    break
            work.show(start)

            finished = True
            for thread in thread_list:
                finished &= not thread.is_alive()

    print()


if __name__ == '__main__':
    main()
