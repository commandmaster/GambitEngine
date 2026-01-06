import gambit_engine
import time

print("Initializing Engine...")
bot = gambit_engine.GambitBot()

# Test FEN
print("Initial FEN:", bot.get_fen())

# Test loading book (optional, might fail if path is wrong, so skipping or handling error)
try:
    bot.load_opening_book("../GambitApp/assets/baron30.bin")
    print("Book loaded.")
except Exception as e:
    print(f"Book load warning: {e}")

# Test searching start pos
print("Searching for best move (depth 5, 1000ms)...")
move = bot.get_best_move(5, 1000)
print("Best move:", move)

# Test setting position (e2e4)
print("Making move e2e4...")
bot.set_position("startpos", ["e2e4"])
print("New FEN:", bot.get_fen())

print("Searching for response...")
move = bot.get_best_move(5, 1000)
print("Best response:", move)
