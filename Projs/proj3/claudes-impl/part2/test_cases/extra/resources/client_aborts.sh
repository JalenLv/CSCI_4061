#!/bin/bash
# Five clients abandon a 32 MiB download after 64 KiB, with the server's
# SIGPIPE at its default action, as when started from a terminal. Then the
# concurrent_open barrier checks that all 5 workers survived.
. test_cases/extra/resources/lib.sh
mkdir -p "$WORK/server_files"
cp server_files/* "$WORK/server_files/"
head -c $((32 << 20)) /dev/zero > "$WORK/server_files/big.txt"
start_server "$WORK/server_files" python3 "$RES/sigpipe_default.py" env LD_PRELOAD=./concurrent_open.so
pids=()
for i in 1 2 3 4 5; do
    $CLIENT abort "$PORT" /big.txt 65536 > "$WORK/abort$i" &
    pids+=($!)
done
wait "${pids[@]}"
cat "$WORK"/abort?
pids=()
for f in quote.txt index.html courses.txt gatsby.txt ocelot.jpg; do
    $CLIENT check "$PORT" "/$f" "server_files/$f" > "$WORK/after-$f" &
    pids+=($!)
done
wait "${pids[@]}"
for f in quote.txt index.html courses.txt gatsby.txt ocelot.jpg; do
    cat "$WORK/after-$f"
done
stop_server
rm -f "$WORK/server_files/big.txt"
